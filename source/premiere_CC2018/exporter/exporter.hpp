#pragma once

#include <atomic>
#include <list>
#include <mutex>
#include <thread>

#include "../movie_writer/movie_writer.hpp"
#include "codec_registration.hpp"

struct ExportJobImpl
{
    ExportJobImpl(std::unique_ptr<EncoderJob>& codecJob_) : codecJob(std::move(codecJob_)) {}

    int64_t iFrame;
    std::unique_ptr<EncoderJob> codecJob;
    EncodeOutput output;
};

class ExporterWorker;

typedef std::unique_ptr<ExportJobImpl> ExportJob;  // either encode or write, depending on the queue its in
typedef std::vector<ExportJob> ExportJobQueue;
typedef std::list<std::unique_ptr<ExporterWorker> > ExportWorkers;

// thread-safe freelist. T must be unique_ptr<>
template<class T>
class FreeList
{
public:
    FreeList(std::function<T ()> factory) : factory_(factory) {}

    T allocate()
    {
        std::lock_guard<std::mutex> guard(mutex_);
        if (!freeList_.empty())
        {
            T t = std::move(freeList_.back());
            freeList_.pop_back();
            return t;
        }
        else
            return factory_();
    }
    void free(T t)
    {
        std::lock_guard<std::mutex> guard(mutex_);
        freeList_.push_back(std::move(t));
    }

private:
    std::mutex mutex_;
    std::vector<T> freeList_;
    std::function<T ()> factory_;
};

typedef FreeList<ExportJob> ExporterJobFreeList;

// thread-safe encoder of ExportJob
class ExporterJobEncoder
{
public:
    ExporterJobEncoder(Encoder& encoder);

    void push(ExportJob job);
    ExportJob encode();

    uint64_t nEncodeJobs() const { return nEncodeJobs_;  }

private:
    Encoder& encoder_;  // must have thread-safe processing functions

    std::mutex mutex_;
    ExportJobQueue queue_;

    std::atomic<uint64_t> nEncodeJobs_;
};

// thread-safe writer of ExportJob
class ExporterJobWriter
{
public:
    ExporterJobWriter(std::unique_ptr<MovieWriter> writer);

    void setFirstFrame(int64_t iFrame);
    // we don't know the index of the first frame until the first frame is dispatched
    // but the writer is created before then
    // and the first frame to hit the writer may not be the first frame that was dispatched
    // (it might take longer to encode)
    // TODO: remove this hack.

    void close();  // call ahead of destruction in order to recognise errors

    void push(ExportJob job);
    ExportJob write();  // returns the job that was written

    double utilisation() { return utilisation_; }

private:

    std::mutex mutex_;
    bool error_;
    ExportJobQueue queue_;
    std::unique_ptr<int64_t> nextFrameToWrite_;
    std::unique_ptr<MovieWriter> writer_;
    std::chrono::high_resolution_clock::time_point idleStart_;
    std::chrono::high_resolution_clock::time_point writeStart_;

    std::atomic<double> utilisation_;
};

class ExporterWorker
{
public:
    ExporterWorker(std::atomic<bool>& error, ExporterJobFreeList& freeList, ExporterJobEncoder& encoder, ExporterJobWriter& writer);
    ~ExporterWorker();

    static void worker_start(ExporterWorker& worker);

private:
    std::thread worker_;
    void run();

    std::atomic<bool> quit_;
    std::atomic<bool>& error_;
    ExporterJobFreeList& jobFreeList_;
    ExporterJobEncoder& jobEncoder_;
    ExporterJobWriter& jobWriter_;
};


class Exporter
{
public:
    Exporter(
        std::unique_ptr<Encoder> encoder,
        std::unique_ptr<MovieWriter> writer);
    ~Exporter();

    // users should call close if they wish to handle errors on shutdown
    void close();
    
    // thread safe to be called 'on frame rendered'
    void dispatch(int64_t iFrame, const uint8_t* bgra_bottom_left_origin_data, size_t stride) const;

private:
    bool closed_;
    mutable std::unique_ptr<int64_t> currentFrame_;

    bool expandWorkerPoolToCapacity() const;
    size_t concurrentThreadsSupported_;

    mutable std::atomic<bool> error_;
    std::unique_ptr<Encoder> encoder_;

    mutable ExporterJobFreeList jobFreeList_;
    mutable ExporterJobEncoder jobEncoder_;
    mutable ExporterJobWriter jobWriter_;

    // must be last to ensure they're joined before their dependents are destructed
    mutable ExportWorkers workers_;
};
