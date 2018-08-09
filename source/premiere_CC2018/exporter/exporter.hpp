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
    static void workerFunction(
        bool& quit,
        std::unique_ptr<Codec>& codec,
        std::mutex& freeListMutex,
        ExportJobQueue& freeList,
        std::mutex& encodeQueueMutex,
        ExportJobQueue& encodeQueue,
        std::mutex& writeQueueMutex,
        ExportJobQueue& writeQueue,
        std::atomic<int64_t> &nextFrameToWrite,
        std::unique_ptr<MovieWriter>& writer,
        int64_t nFrames);
    std::list<std::thread> workers_;

    ExportJob getFreeJob() const;

    std::unique_ptr<Codec> codec_;
    int64_t nFrames_;
    mutable int64_t nFramesDispatched_;

    mutable std::mutex freeListMutex_;
    mutable ExportJobQueue freeList_;

    mutable std::mutex encodeQueueMutex_;
    mutable ExportJobQueue encodeQueue_;

    mutable std::mutex writeQueueMutex_;
    mutable ExportJobQueue writeQueue_;
    mutable std::unique_ptr<MovieWriter> writer_;  // protected with writeQueueMutex_ too

    mutable std::atomic<int64_t>  nextFrameToWrite_;
};
