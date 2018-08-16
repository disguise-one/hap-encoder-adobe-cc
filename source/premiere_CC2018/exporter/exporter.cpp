#include <chrono>
#include <thread>

#include "exporter.hpp"

using namespace std::chrono_literals;

ExportJob ExporterJobFreeList::allocate_job()
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (!jobs_.empty())
    {
        ExportJob job = std::move(jobs_.back());
        jobs_.pop_back();
        return job;
    }
    else
        return std::make_unique<ExportFrameAndBuffers>();
}

void ExporterJobFreeList::free_job(ExportJob job)
{
    std::lock_guard<std::mutex> guard(mutex_);
    jobs_.push_back(std::move(job));
}



ExporterJobEncoder::ExporterJobEncoder(Codec& codec)
    : codec_(codec)
{
}

void ExporterJobEncoder::push(ExportJob job)
{
    std::lock_guard<std::mutex> guard(mutex_);
    queue_.push_back(std::move(job));
    nEncodeJobs_++;
}

ExportJob ExporterJobEncoder::encode()
{
    ExportJob job;
    std::lock_guard<std::mutex> guard(mutex_);
    auto earliest = std::min_element(queue_.begin(), queue_.end(),
                                     [](const auto& lhs, const auto& rhs) { return (*lhs).first < (*rhs).first; });
    if (earliest != queue_.end()) {
        job = std::move(*earliest);
        queue_.erase(earliest);

        codec_.encode(job->second.scratchpad, job->second.output);

        nEncodeJobs_--;
    }
    return job;
}

ExporterJobWriter::ExporterJobWriter(std::unique_ptr<MovieWriter> writer, int64_t nFrames)
    : writer_(std::move(writer)), nFrames_(nFrames), nextFrameToWrite_(0)
{
}

void ExporterJobWriter::push(ExportJob job)
{
    std::lock_guard<std::mutex> guard(mutex_);
    queue_.push_back(std::move(job));
}

ExportJob ExporterJobWriter::write()
{
    std::unique_lock<std::mutex> try_guard(mutex_, std::try_to_lock);

    if (try_guard.owns_lock())
    {
        auto earliest = std::min_element(queue_.begin(), queue_.end(),
                                         [](const auto& lhs, const auto& rhs) { return (*lhs).first < (*rhs).first; });
        if (earliest != queue_.end() && (*earliest)->first == nextFrameToWrite_) {
            ExportJob job = std::move(*earliest);
            queue_.erase(earliest);

            writer_->writeFrame(&job->second.output.buffer[0], job->second.output.buffer.size());
            nextFrameToWrite_++;

            // last frame triggers close etc
            if (nextFrameToWrite_ == nFrames_)
            {
                writer_.reset(nullptr);
            }

            return job;
        }

        try_guard.unlock();
    }

    return nullptr;
}

void ExporterJobWriter::waitForLastWrite()
{
    // wait for completion - nonintrusive
    while (nextFrameToWrite_ < nFrames_)
    {
        std::this_thread::sleep_for(10ms);
    }
}

ExporterWorker::ExporterWorker(bool& quit, ExporterJobFreeList& freeList, ExporterJobEncoder& encoder, ExporterJobWriter& writer)
    : quit_(quit), freeList_(freeList), encoder_(encoder), writer_(writer)
{
    worker_ = std::thread(worker_start, std::ref(*this));
}

ExporterWorker::~ExporterWorker()
{
    worker_.join();
}

// static public interface for std::thread
void ExporterWorker::worker_start(ExporterWorker& worker)
{
    worker.run();
}

// private
void ExporterWorker::run()
{
    while (!quit_)
    {
        // we assume the exporter delivers frames in sequence; we can't
        // buffer unlimited frames for writing until we're give frame 1
        // at the end, for example.
        ExportJob job = encoder_.encode();

        if (!job)
        {
            std::this_thread::sleep_for(2ms);
        }
        else
        {
            // submit it to be written (frame may be out of order)
            writer_.push(std::move(job));

            // dequeue any in-order writes
            // NOTE: other threads may be blocked here, even though they could get on with encoding
            //       this is ultimately not a problem as they'll be rate limited by how much can be
            //       written out - we don't want encoding to get too far ahead of writing, as that's
            //       a liveleak of the encode buffers
            //       each job that is written, we release one decode thread
            //       each job must do one write
            do
            {
                ExportJob written = writer_.write();

                if (written)
                {
                    freeList_.free_job(std::move(written));
                    break;
                }

                std::this_thread::sleep_for(1ms);
            } while (!quit_);
        }
    }
}



Exporter::Exporter(
    std::unique_ptr<Codec> codec,
    std::unique_ptr<MovieWriter> movieWriter,
    int64_t nFrames)
  : codec_(std::move(codec)), encoder_(*codec_), writer_(std::move(movieWriter), nFrames), nFrames_(nFrames), nFramesDispatched_(0),
    quit_(false)
{
    concurrentThreadsSupported_ = std::thread::hardware_concurrency();
    for (size_t i=0; i<concurrentThreadsSupported_; ++i)
    {
        workers_.push_back(ExporterWorker(quit_, freeList_, encoder_, writer_));
    }
}

Exporter::~Exporter()
{
    // if the number of frames dispatched was _all_ of them, wait for the final renders to exit the door
    if (nFramesDispatched_ == nFrames_)
    {
        writer_.waitForLastWrite();
    }

    // workers can go home
    quit_ = true;
}

void Exporter::dispatch(int64_t iFrame, const uint8_t* bgra_bottom_left_origin_data, size_t stride) const
{
    ExportJob job = freeList_.allocate_job();

    job->first = iFrame;

    // take a copy of the frame, in the codec preferred manner. we immediately return and let
    // the renderer get on with its job.
    //
    // TODO: may be able to use Adobe's addRef at a higher level and pipe it through for a minor
    //       performance gain
    job->second.input.bgraBottomLeftOrigin = bgra_bottom_left_origin_data;
    job->second.input.stride = stride;

    codec_->copyExternalToLocal(
        job->second.input, job->second.scratchpad, job->second.output);

    encoder_.push(std::move(job));
    nFramesDispatched_++;

    // throttle - if the queue is getting too long we should wait
    while (encoder_.nEncodeJobs() > concurrentThreadsSupported_)
    {
        std::this_thread::sleep_for(2ms);
    }
}