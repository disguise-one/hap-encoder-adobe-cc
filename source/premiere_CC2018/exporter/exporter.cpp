#include <chrono>
#include <thread>

#include "exporter.hpp"

using namespace std::chrono_literals;

ExporterJobEncoder::ExporterJobEncoder(Encoder& encoder)
    : encoder_(encoder), nEncodeJobs_(0)
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
        job->codecJob->convert();
        job->codecJob->encode(job->output);
    }

    return job;
}

ExporterJobWriter::ExporterJobWriter(std::unique_ptr<MovieWriter> writer)
  : writer_(std::move(writer)),
    utilisation_(1.),
    error_(false)
{
}

void ExporterJobWriter::setFirstFrame(int64_t iFrame)
{
    std::lock_guard<std::mutex> guard(mutex_);

    if (nextFrameToWrite_)
    {
        throw std::runtime_error("setting first frame twice");   // this should be an assertion
    }

    nextFrameToWrite_ = std::make_unique<int64_t>(iFrame);
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
        try {
            auto earliest = std::min_element(queue_.begin(), queue_.end(),
                [](const auto& lhs, const auto& rhs) { return (*lhs).iFrame < (*rhs).iFrame; });
            if (earliest != queue_.end() && nextFrameToWrite_ && (*earliest)->iFrame == *nextFrameToWrite_) {
                ExportJob job = std::move(*earliest);
                queue_.erase(earliest);

                // start idle timer first time we try to write to avoid false including setup time
                if (idleStart_ == std::chrono::high_resolution_clock::time_point())
                    idleStart_ = std::chrono::high_resolution_clock::now();

                writeStart_ = std::chrono::high_resolution_clock::now();
                writer_->writeFrame(&job->output.buffer[0], job->output.buffer.size());
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

                (*nextFrameToWrite_)++;

                return job;
            }
        }
        catch (...) {
            error_ = true;
            throw;
        }
    }

    return nullptr;
}

void ExporterJobWriter::close()
{
    if (!error_)
    {
        writer_->writeTrailer();
    }

    writer_->close();
}

ExporterWorker::ExporterWorker(std::atomic<bool>& error, ExporterJobFreeList& freeList, ExporterJobEncoder& encoder, ExporterJobWriter& writer)
    : quit_(false), error_(error), jobFreeList_(freeList), jobEncoder_(encoder), jobWriter_(writer)
{
    worker_ = std::thread(worker_start, std::ref(*this));
}

ExporterWorker::~ExporterWorker()
{
    quit_ = true;
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
        // if we hit an error, we shouldn't keep participating
        if (error_)
        {
            std::this_thread::sleep_for(1ms);
            continue;
        }

        try {
            // we assume the exporter delivers frames in sequence; we can't
            // buffer unlimited frames for writing until we're give frame 1
            // at the end, for example.
            ExportJob job = jobEncoder_.encode();

            if (!job)
            {
                std::this_thread::sleep_for(2ms);
            }
            else
            {
                // submit it to be written (frame may be out of order)
                jobWriter_.push(std::move(job));

                // dequeue any in-order writes
                // NOTE: other threads may be blocked here, even though they could get on with encoding
                //       this is ultimately not a problem as they'll be rate limited by how much can be
                //       written out - we don't want encoding to get too far ahead of writing, as that's
                //       a liveleak of the encode buffers
                //       each job that is written, we release one decode thread
                //       each job must do one write
                do
                {
                    ExportJob written = jobWriter_.write();

                    if (written)
                    {
                        jobFreeList_.free(std::move(written));
                        break;
                    }

                    std::this_thread::sleep_for(1ms);
                } while (!error_);  // an error in a different thread will abort this thread's need to write
            }
        }
        catch (...)
        {
            //!!! should copy the exception and rethrow in main thread when it joins
            error_ = true;
        }
    }
}



Exporter::Exporter(
    std::unique_ptr<Encoder> encoder,
    std::unique_ptr<MovieWriter> movieWriter)
  : encoder_(std::move(encoder)), jobEncoder_(*encoder_),
    jobFreeList_(std::function<ExportJob ()>([&](){
        return std::make_unique<ExportJob::element_type>(encoder_->create());
    })),
    closed_(false),
    jobWriter_(std::move(movieWriter)),
    error_(false)
{
    concurrentThreadsSupported_ = std::thread::hardware_concurrency() + 1;  // we assume at least 1 thread will be blocked by io write

    // assume 4 threads + 1 writing will not tax the system too much before we figure out its limits
    size_t startingThreads = std::min(std::size_t{ 5 }, concurrentThreadsSupported_);
    for (size_t i=0; i<startingThreads; ++i)
    {
        workers_.push_back(std::make_unique<ExporterWorker>(error_, jobFreeList_, jobEncoder_, jobWriter_));
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
    if (!closed_)
    {
        // we don't want to retry closing on destruction if we throw an exception
        closed_ = true;

        // wait for last jobs to complete. The last one does the last write. If something
        // fails it will abort the others.
        {
            ExportWorkers empty;  // this must be destructed before writer_.close()
            std::swap(workers_, empty);
        }

        // finalise and close the file.
        jobWriter_.close();

        if (error_)
            throw std::runtime_error("error writing");
    }
}


void Exporter::dispatch(int64_t iFrame, const uint8_t* bgra_bottom_left_origin_data, size_t stride) const
{
    // it is not clear from the docs whether or not frames are completed and passed in strict order
    // nevertheless we must make that assumption because
    //   -we don't know the start frame
    //   -we have finite space for buffering frames
    // so validate that here.
    // TODO: get confirmation from Adobe that this assumption cannot be violated
    if (!currentFrame_) {
        currentFrame_ = std::make_unique<int64_t>(iFrame);
        jobWriter_.setFirstFrame(iFrame);
    }
    else {
        ++(*currentFrame_);
        if (*currentFrame_ != iFrame)
        {
            // if this happens, this code needs revisiting
            throw std::runtime_error("plugin fault: frames delivered out of order");
        }
    }

    // throttle the caller - if the queue is getting too long we should wait
    while ( (jobEncoder_.nEncodeJobs() >= workers_.size()-1)
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

    ExportJob job = jobFreeList_.allocate();

    job->iFrame = iFrame;

    // take a copy of the frame, in the codec preferred manner. we immediately return and let
    // the renderer get on with its job.
    //
    // TODO: may be able to use Adobe's addRef at a higher level and pipe it through for a minor
    //       performance gain
    job->codecJob->copyExternalToLocal(bgra_bottom_left_origin_data, stride);

    jobEncoder_.push(std::move(job));
}

// returns true if pool was expanded
bool Exporter::expandWorkerPoolToCapacity() const
{
    bool isNotThreadLimited = workers_.size() < concurrentThreadsSupported_;
    bool isNotOutputLimited = jobWriter_.utilisation() < 0.99;
    bool isNotBufferLimited = true;  // TODO: get memoryUsed < maxMemoryCapacity from Adobe API

    if (isNotThreadLimited && isNotOutputLimited && isNotBufferLimited) {
        workers_.push_back(std::make_unique<ExporterWorker>(error_, jobFreeList_, jobEncoder_, jobWriter_));
        return true;
    }
    return false;
}
