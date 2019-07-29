#pragma once

#include <atomic>
#include <list>
#include <mutex>
#include <queue>
#include <thread>

#include "../movie_writer/movie_writer.hpp"
#include "codec_registration.hpp"
#include "../freelist.hpp"

struct ExportJobImpl
{
    ExportJobImpl(std::unique_ptr<EncoderJob> codecJob_) : codecJob(std::move(codecJob_)) {}

    int64_t iFrame{ -1 };
    std::unique_ptr<EncoderJob> codecJob;
    EncodeOutput output;
};

class ExporterWorker;

typedef std::unique_ptr<ExportJobImpl> ExportJob;  // either encode or write, depending on the queue its in
typedef std::vector<ExportJob> ExportJobQueue;
typedef std::list<std::unique_ptr<ExporterWorker> > ExportWorkers;

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

    // frames may arrive out of order due to encoding taking varied lengths of time
    // we don't know the index of the first frame until the first frame is dispatched
    // there can be jumps in sequence where there are no source frames (Premiere Pro can skip frames)
    // so when the external host dispatches frames to us, we store the order they arrived, and write
    // them out in that order
    void enqueueFrameWrite(int64_t iFrame);

    // !!! temporary hack for aex audio
    void dispatch_audio_at_end(const uint8_t *audio, size_t size);

    void close();  // call ahead of destruction in order to recognise errors

    void push(ExportJob job);
    ExportJob write();  // returns the job that was written

    double utilisation() { return utilisation_; }

private:

    std::mutex mutex_;
    bool error_;
    ExportJobQueue queue_;
    std::unique_ptr<MovieWriter> writer_;
    std::chrono::high_resolution_clock::time_point idleStart_;
    std::chrono::high_resolution_clock::time_point writeStart_;

    std::atomic<double> utilisation_;

    std::mutex frameOrderMutex_;
    std::queue<int64_t> frameOrderQueue_;

    // !!! temporary hack for aex audio to be written just prior to footer
    std::vector<uint8_t> audioBuffer_;
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
        UniqueEncoder encoder,
        std::unique_ptr<MovieWriter> writer);
    ~Exporter();

    // users should call close if they wish to handle errors on shutdown - destructors cannot
    // throw, 'close' can and will
    void close();
    
    // thread safe to be called 'on frame rendered'
    void dispatch(int64_t iFrame, const uint8_t* data, size_t stride, FrameFormat format) const;

    // !!! temporary hack for aex audio
    void dispatch_audio_at_end(const uint8_t *audio, size_t size);

private:
    bool closed_;
    mutable std::unique_ptr<int64_t> currentFrame_;

    bool expandWorkerPoolToCapacity() const;
    size_t concurrentThreadsSupported_;

    mutable std::atomic<bool> error_;
    UniqueEncoder encoder_;

    mutable ExporterJobFreeList jobFreeList_;
    mutable ExporterJobEncoder jobEncoder_;
    mutable ExporterJobWriter jobWriter_;

    // must be last to ensure they're joined before their dependents are destructed
    mutable ExportWorkers workers_;
};
