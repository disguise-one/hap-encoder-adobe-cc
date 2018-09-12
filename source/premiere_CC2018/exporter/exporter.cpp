#include <chrono>
#include <thread>

#include "exporter.hpp"

using namespace std::chrono_literals;

ExporterJobEncoder::ExporterJobEncoder(Codec& codec)
    : codec_(codec), nEncodeJobs_(0)
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

    {
        std::lock_guard<std::mutex> guard(mutex_);
        auto earliest = std::min_element(queue_.begin(), queue_.end(),
                                         [](const auto& lhs, const auto& rhs) { return (*lhs).iFrame < (*rhs).iFrame; });
        if (earliest != queue_.end()) {
            job = std::move(*earliest);
            queue_.erase(earliest);
            nEncodeJobs_--;
        }
    }

    if (job)
    {
        codec_.convert(job->buffers.input, job->buffers.scratchpad);
        codec_.encode(job->buffers.scratchpad, job->buffers.output);
    }

    return job;
}

ExporterJobWriter::ExporterJobWriter(std::unique_ptr<MovieWriter> writer, int64_t nFrames)
    : writer_(std::move(writer)), nFrames_(nFrames), nextFrameToWrite_(0),
      utilisation_(1.)
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
                                         [](const auto& lhs, const auto& rhs) { return (*lhs).iFrame < (*rhs).iFrame; });
        if (earliest != queue_.end() && (*earliest)->iFrame == nextFrameToWrite_) {
            ExportJob job = std::move(*earliest);
            queue_.erase(earliest);

            // start idle timer first time we try to write to avoid false including setup time
            if (idleStart_ == std::chrono::high_resolution_clock::time_point())
                idleStart_ = std::chrono::high_resolution_clock::now();

            writeStart_ = std::chrono::high_resolution_clock::now();
            writer_->writeFrame(&job->buffers.output.buffer[0], job->buffers.output.buffer.size());
            auto writeEnd = std::chrono::high_resolution_clock::now();
 
            // filtered update of utilisation_
            if (writeEnd != idleStart_)
            {
                auto totalTime = (writeEnd - idleStart_).count();
                auto writeTime = (writeEnd - writeStart_).count();
                const double alpha = 0.9;
                utilisation_ = (1.0 - alpha) * utilisation_ + alpha * ((double)writeTime / totalTime);
            }
            idleStart_ = writeEnd;

            nextFrameToWrite_++;

            // last frame triggers writing trailer
            if (nextFrameToWrite_ == nFrames_)
            {
                writer_->writeTrailer();
            }

            return job;
        }

        try_guard.unlock();
    }

    return nullptr;
}

void ExporterJobWriter::waitForLastWrite(const std::atomic<bool>& abort)
{
    // wait for completion - nonintrusive
    while (nextFrameToWrite_ < nFrames_ &&
          !abort)
    {
        std::this_thread::sleep_for(10ms);
    }
}

void ExporterJobWriter::close()
{
    writer_->close();
}

ExporterWorker::ExporterWorker(bool& quit, std::atomic<bool>& error, ExporterJobFreeList& freeList, ExporterJobEncoder& encoder, ExporterJobWriter& writer)
    : quit_(quit), error_(error), freeList_(freeList), encoder_(encoder), writer_(writer)
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
    try
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
                        freeList_.free(std::move(written));
                        break;
                    }

                    std::this_thread::sleep_for(1ms);
                } while (!quit_);
            }
        }
    }
    catch (...)
    {
        //!!! should copy the exception and rethrow in main thread when it joins
        error_ = true;
    }
}



Exporter::Exporter(
    std::unique_ptr<Codec> codec,
    std::unique_ptr<MovieWriter> movieWriter,
    int64_t nFrames)
  : codec_(std::move(codec)), encoder_(*codec_), nFrames_(nFrames),
            writer_(std::move(movieWriter), nFrames), nFramesDispatched_(0),
    quit_(false), error_(false)
{
    concurrentThreadsSupported_ = std::thread::hardware_concurrency() + 1;  // we assume at least 1 thread will be blocked by io write

    // assume 4 threads + 1 writing will not tax the system too much before we figure out its limits
    size_t startingThreads = std::min(std::size_t{ 5 }, concurrentThreadsSupported_);
    for (size_t i=0; i<startingThreads; ++i)
    {
        workers_.push_back(std::make_unique<ExporterWorker>(quit_, error_, freeList_, encoder_, writer_));
    }
}

Exporter::~Exporter()
{
    try
    {
        close();
    }
    catch (...)
    {
        //!!! not much we can do now;
        //!!! users should call 'close' themselves if they need to catch errors
    }
}

void Exporter::close()
{
    // if the number of frames dispatched was _all_ of them, wait for the final renders to exit the door
    if (nFramesDispatched_ == nFrames_)
    {
        writer_.waitForLastWrite(error_);
    }

    // workers can go home
    quit_ = true;

    // wait for them to exit
    std::swap(workers_, ExportWorkers());

    writer_.close();

    if (error_)
        throw std::runtime_error("error writing");
}


void Exporter::dispatch(int64_t iFrame, const uint8_t* bgra_bottom_left_origin_data, size_t stride) const
{
    // throttle the caller - if the queue is getting too long we should wait
    while ( (encoder_.nEncodeJobs() >= workers_.size()-1)
            && !expandWorkerPoolToCapacity()  // if we can, expand the pool
            && !error_)
    {
        // otherwise wait for an opening
        std::this_thread::sleep_for(2ms);
    }

    // worker threads can die while encoding or writing (eg full disk)
    // this is the most likely spot where the error can be noted by the main thread
    // TODO: should intercept and alert with the correct error reason
    if (error_)
        throw std::runtime_error("error while exporting");

    ExportJob job = freeList_.allocate();

    job->iFrame = iFrame;

    // take a copy of the frame, in the codec preferred manner. we immediately return and let
    // the renderer get on with its job.
    //
    // TODO: may be able to use Adobe's addRef at a higher level and pipe it through for a minor
    //       performance gain
    codec_->copyExternalToLocal(
        bgra_bottom_left_origin_data, stride,
        job->buffers.input);

    encoder_.push(std::move(job));
    nFramesDispatched_++;
}

// returns true if pool was expanded
bool Exporter::expandWorkerPoolToCapacity() const
{
    bool isNotThreadLimited = workers_.size() < concurrentThreadsSupported_;
    bool isNotOutputLimited = writer_.utilisation() < 0.99;
    bool isNotBufferLimited = true;  // TODO: get memoryUsed < maxMemoryCapacity from Adobe API

    if (isNotThreadLimited && isNotOutputLimited && isNotBufferLimited) {
        workers_.push_back(std::make_unique<ExporterWorker>(quit_, error_, freeList_, encoder_, writer_));
        return true;
    }
    return false;
}
