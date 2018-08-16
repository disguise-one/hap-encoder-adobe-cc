#pragma once

#include <atomic>
#include <list>
#include <mutex>
#include <thread>

#include "../movie_writer/movie_writer.hpp"
#include "../codec/codec.hpp"

struct ExporterEncodeBuffers
{
    EncodeInput input;
    EncodeScratchpad scratchpad;
    EncodeOutput output;
};

typedef std::pair<int64_t, ExporterEncodeBuffers> ExportFrameAndBuffers;
typedef std::unique_ptr<ExportFrameAndBuffers> ExportJob;  // either encode or write, depending on the queue its in
typedef std::vector<ExportJob> ExportJobQueue;

// thread-safe freelist of ExportJob
class ExporterJobFreeList
{
public:
    ExportJob allocate_job();
    void free_job(ExportJob job);

private:
    std::mutex mutex_;
    ExportJobQueue jobs_;
};

// thread-safe encoder of ExportJob
class ExporterJobEncoder
{
public:
    ExporterJobEncoder(Codec& codec);

    void push(ExportJob job);
    ExportJob encode();

    int64_t nEncodeJobs() const { return nEncodeJobs_;  }

private:
    Codec& codec_;  // must have thread-safe processing functions

    std::mutex mutex_;
    ExportJobQueue queue_;

    std::atomic<int64_t> nEncodeJobs_;
};

// thread-safe writer of ExportJob
class ExporterJobWriter
{
public:
    ExporterJobWriter(std::unique_ptr<MovieWriter> writer, int64_t nFrames);

    void push(ExportJob job);
    ExportJob write();  // returns the job that was written

    void waitForLastWrite(); // finish up

private:
    std::unique_ptr<MovieWriter> writer_;
    int64_t nFrames_;

    std::mutex mutex_;
    ExportJobQueue queue_;
    std::atomic<int64_t> nextFrameToWrite_;
    std::unique_ptr<MovieWriter> writer_;
};

class ExporterWorker
{
public:
    ExporterWorker(bool& quit, ExporterJobFreeList& freeList, ExporterJobEncoder& encoder, ExporterJobWriter& writer);
    ~ExporterWorker();

    static void worker_start(ExporterWorker& worker);

private:
    std::thread worker_;
    void run();

    bool& quit_;
    ExporterJobFreeList& freeList_;
    ExporterJobEncoder& encoder_;
    ExporterJobWriter& writer_;
};


class Exporter
{
public:
    Exporter(
        std::unique_ptr<Codec> codec,
        std::unique_ptr<MovieWriter> writer,
        int64_t nFrames);
    ~Exporter();
    
    // thread safe to be called 'on frame rendered'
    void dispatch(int64_t iFrame, const uint8_t* bgra_bottom_left_origin_data, size_t stride) const;

private:
    size_t concurrentThreadsSupported_;
    bool quit_;
    std::list<ExporterWorker> workers_;

    std::unique_ptr<Codec> codec_;
    int64_t nFrames_;
    mutable int64_t nFramesDispatched_;

    mutable ExporterJobFreeList freeList_;
    mutable ExporterJobEncoder encoder_;
    mutable ExporterJobWriter writer_;
};
