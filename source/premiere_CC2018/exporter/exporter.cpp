#include <chrono>
#include <thread>

#include "hapencode.hpp"
#include "codec/codec.hpp"
#include "configure.hpp"

using namespace std::chrono_literals;

Exporter::Exporter(
    std::unique_ptr<Codec> codec,
    std::unique_ptr<MovieWriter> writer,
    int64_t nFrames)
  : codec_(std::move(codec)), writer_(std::move(writer)), nFrames_(nFrames), nFramesDispatched_(0),
    nextFrameToWrite_(0),
    quit_(false)
{
    concurrentThreadsSupported_ = std::thread::hardware_concurrency();
    for (size_t i=0; i<concurrentThreadsSupported_; ++i)
    {
        workers_.push_back(
            std::thread(workerFunction,
                std::ref(quit_),
                std::ref(codec_),
                std::ref(freeListMutex_),
                std::ref(freeList_),
                std::ref(encodeQueueMutex_),
                std::ref(encodeQueue_),
                std::ref(writeQueueMutex_),
                std::ref(writeQueue_),
                std::ref(nextFrameToWrite_),
                std::ref(writer_),
                nFrames));
    }
}

Exporter::~Exporter()
{
    // if the number of frames dispatched was _all_ of them, wait for the final renders to exit the door
    if (nFramesDispatched_ == nFrames_)
    {
        // wait for completion - nonintrusive
        while (nextFrameToWrite_ < nFrames_)
        {
            std::this_thread::sleep_for(10ms);
        }
    }

    // workers can go home
    quit_ = true;
    for (auto it = workers_.begin(); it != workers_.end(); ++it)
    {
        it->join();
    }
}

void Exporter::dispatch(int64_t iFrame, const uint8_t* bgra_bottom_left_origin_data, size_t stride) const
{
    ExportJob job = getFreeJob();

    job->first = iFrame;

    // take a copy of the frame, in the codec preferred manner. we immediately return and let
    // the renderer get on with its job.
    //
    // TODO: may be able to use Adobe's addRef at a higher level and pipe it through for a minor
    //       performance gain
    // TODO: this is a little clumsy, fix
    job->second.input.bgraBottomLeftOrigin = bgra_bottom_left_origin_data;
    job->second.input.stride = stride;

    codec_->copyExternalToLocal(
        job->second.input, job->second.scratchpad, job->second.output);

    size_t nEncodeJobs;
    {
        std::lock_guard<std::mutex> guard(encodeQueueMutex_);
        encodeQueue_.push_back(std::move(job));
        nEncodeJobs = encodeQueue_.size();
        nFramesDispatched_++;
    }

    // throttle - if the queue is getting too long we should wait
    while (nEncodeJobs > concurrentThreadsSupported_)
    {
        std::this_thread::sleep_for(10ms);
        std::lock_guard<std::mutex> guard(encodeQueueMutex_);
        nEncodeJobs = encodeQueue_.size();
    }
}

void Exporter::workerFunction(
    bool& quit,
    std::unique_ptr<Codec>& codec,
    std::mutex& freeListMutex,
    ExportJobQueue& freeList,
    std::mutex& encodeQueueMutex,
    ExportJobQueue& encodeQueue,
    std::mutex& writeQueueMutex,
    ExportJobQueue& writeQueue,
    std::atomic<int64_t>& nextFrameToWrite,
    std::unique_ptr<MovieWriter>& writer,
    int64_t nFrames)
{
    while (!quit)
    {
        ExportJob job;

        // wait on work even
        {
            std::lock_guard<std::mutex> guard(encodeQueueMutex);
            if (!encodeQueue.empty())
            {
                job = std::move(encodeQueue.back());
                encodeQueue.pop_back();
            }
        }

        if (!job)
        {
            std::this_thread::sleep_for(2ms);
        }
        else
        {
            codec->encode(job->second.scratchpad, job->second.output);

            // submit it to be written (frame may be out of order)
            std::lock_guard<std::mutex> guard(writeQueueMutex);
            writeQueue.push_back(std::move(job));

            // dequeue any in-order writes
            // NOTE: other threads may be blocked here, even though they could get on with encoding
            //       this is ultimately not a problem as they'll be rate limited by how much can be
            //       written out - we don't want encoding to get too far ahead of writing, as that's
            //       a liveleak of the encode buffers
            bool written;
            do
            {
                written = false;
                for (auto i = writeQueue.begin(); i != writeQueue.end(); ++i)
                {
                    if ((*i)->first == nextFrameToWrite)
                    {
                        ExportJob job = std::move((*i));
                        writer->writeFrame(&job->second.output.buffer[0], job->second.output.buffer.size());
                        nextFrameToWrite++;
                        writeQueue.erase(i);
                        {
                            std::lock_guard<std::mutex> freeListGuard(freeListMutex);
                            freeList.push_back(std::move(job));
                        }
                        written = true;
                        break;
                    }
                }
            } while (written);

            if (nextFrameToWrite > nFrames)
            {
                writer.reset(nullptr);
            }
        }
    }
}

ExportJob Exporter::getFreeJob() const
{
    std::lock_guard<std::mutex> guard(freeListMutex_);
    if (!freeList_.empty())
    {
        ExportJob freeJob = std::move(freeList_.back());
        freeList_.pop_back();
        return freeJob;
    }
    else
        return std::make_unique<ExportFrameAndBuffers>();
}


csSDK_int32 getPixelFormatSize(const PrFourCC subtype)
{
    // !!! wrong.
	return 4;
}

