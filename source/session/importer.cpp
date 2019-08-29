#include <codecvt>
#include <new>

#include "codec_registration.hpp"
#include "importer.hpp"

#ifdef PRMAC_ENV
#include <os/log.h>
#endif

using namespace std::chrono_literals;

ImporterJobReader::ImporterJobReader(std::unique_ptr<MovieReader> reader)
    : reader_(std::move(reader)),
    utilisation_(1.)
{
}

void ImporterJobReader::push(ImportJob job)
{
    std::lock_guard<std::mutex> guard(mutex_);
    queue_.push_back(std::move(job));
}

ImportJob ImporterJobReader::read()
{
    std::unique_lock<std::mutex> try_guard(mutex_, std::try_to_lock);

    if (try_guard.owns_lock())
    {
        try {
            // attempt to read in frame order
            auto earliest = std::min_element(queue_.begin(), queue_.end(),
                [](const auto& lhs, const auto& rhs) { return (*lhs).iFrame < (*rhs).iFrame; });
            if (earliest != queue_.end()) {
                ImportJob job = std::move(*earliest);
                queue_.erase(earliest);

                // start idle timer first time we try to read to avoid falsely including setup time
                if (idleStart_ == std::chrono::high_resolution_clock::time_point())
                    idleStart_ = std::chrono::high_resolution_clock::now();

                readStart_ = std::chrono::high_resolution_clock::now();
                
                try {
                    reader_->readVideoFrame(job->iFrame, job->input.buffer);
                    auto readEnd = std::chrono::high_resolution_clock::now();

                    // filtered update of utilisation_
                    if (readEnd != idleStart_)
                    {
                        auto totalTime = (readEnd - idleStart_).count();
                        auto readTime = (readEnd - readStart_).count();
                        const double alpha = 0.9;
                        utilisation_ = (1.0 - alpha) * utilisation_ + alpha * ((double)readTime / totalTime);
                    }
                    idleStart_ = readEnd;
                }
                catch (...)
                {
                    job->failed = true;
                }
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

ImporterJobDecoder::ImporterJobDecoder(Decoder& decoder)
    : decoder_(decoder), nDecodeJobs_(0)
{
}

void ImporterJobDecoder::push(ImportJob job)
{
    std::lock_guard<std::mutex> guard(mutex_);
    queue_.push_back(std::move(job));
    nDecodeJobs_++;
}

ImportJob ImporterJobDecoder::decode()
{
    ImportJob job;

    {
        std::lock_guard<std::mutex> guard(mutex_);
        auto earliest = std::min_element(queue_.begin(), queue_.end(),
            [](const auto& lhs, const auto& rhs) { return (*lhs).iFrame < (*rhs).iFrame; });
        if (earliest != queue_.end()) {
            job = std::move(*earliest);
            queue_.erase(earliest);
            nDecodeJobs_--;
        }
    }

    if (job && !job->failed)
    {
        try {
            job->codecJob->decode(job->input.buffer);
        }
        catch (...)
        {
            job->failed = true;
        }
    }

    return job;
}

void ImporterJobReader::close()
{
    reader_.reset(0);
}

ImporterWorker::ImporterWorker(std::atomic<bool>& error, ImporterJobFreeList& freeList, ImporterJobReader& reader, ImporterJobDecoder& decoder)
    : error_(error), jobFreeList_(freeList), jobDecoder_(decoder), jobReader_(reader)
{
    worker_ = std::thread(worker_start, std::ref(*this));
}

ImporterWorker::~ImporterWorker()
{
    quit_ = true;
    worker_.join();
}

// static public interface for std::thread
void ImporterWorker::worker_start(ImporterWorker& worker)
{
    worker.run();
}

// private
void ImporterWorker::run()
{
    while (!quit_)
    {
        try {
            ImportJob job = jobReader_.read();

            if (!job)
            {
                std::this_thread::sleep_for(2ms);
            }
            else
            {
                // submit it to be decoded (frame may be out of order)
                jobDecoder_.push(std::move(job));

                // dequeue any decodes
                do
                {
                    ImportJob decoded = jobDecoder_.decode();

                    if (decoded)
                    {
                        if (decoded->failed)
                        {
                            decoded->onFail(*decoded->codecJob);
                        }
                        else
                        {
                            decoded->onSuccess(*decoded->codecJob);
                        }
                        jobFreeList_.free(std::move(decoded));
                        break;
                    }

                    std::this_thread::sleep_for(1ms);
                } while (!error_);  // an error in a different thread will abort this thread's need to read
            }
        }
        catch (...)
        {
            //!!! should copy the exception and rethrow in main thread when it joins
            error_ = true;
            throw;
        }
    }
}



Importer::Importer(
    std::unique_ptr<MovieReader> movieReader,
    UniqueDecoder decoder)
    : decoder_(std::move(decoder)),
      jobFreeList_(std::function<ImportJob()>([&]() {
        return std::make_unique<ImportJob::element_type>(decoder_->create());
      })),
      jobReader_(std::move(movieReader)),
      jobDecoder_(*decoder_)
{
    concurrentThreadsSupported_ = std::thread::hardware_concurrency() + 1;  // we assume at least 1 thread will be blocked by io read

    // assume 4 threads + 1 serialising will not tax the system too much before we figure out its limits
    int startingThreads = std::min(5, concurrentThreadsSupported_);
    for (size_t i = 0; i < startingThreads; ++i)
    {
        workers_.push_back(std::make_unique<ImporterWorker>(error_, jobFreeList_, jobReader_, jobDecoder_));
    }
}

Importer::~Importer()
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

void Importer::close()
{
    if (!closed_)
    {
        // we don't want to retry closing on destruction if we throw an exception
        closed_ = true;

        // close the file.
        jobReader_.close();

        // wait for last jobs to complete. The last one does the last read. If something
        // fails it will abort the others.
        {
            ImportWorkers empty;  // this must be destructed before reader_.close()
            std::swap(workers_, empty);
        }

        if (error_)
            throw std::runtime_error("error on close");
    }
}

void Importer::requestFrame(int32_t iFrame,
                            std::function<void(const DecoderJob&)> onSuccess,
                            std::function<void(const DecoderJob&)> onFail)
{
    // throttle the caller - if the queue is getting too long we should wait
    while ((jobDecoder_.nDecodeJobs() >= workers_.size() - 1)
        && !expandWorkerPoolToCapacity()  // if we can, expand the pool
        && !error_)
    {
        // otherwise wait for an opening
        std::this_thread::sleep_for(2ms);
    }

    // worker threads can die while reading or encoding (eg dud file)
    // this is the most likely spot where the error can be noted by the main thread
    // TODO: should intercept and alert with the correct error reason
    if (error_)
        throw std::runtime_error("error while Importing");

    ImportJob job = jobFreeList_.allocate();

    job->iFrame = iFrame;
    job->onSuccess = onSuccess;
    job->onFail = onFail;
    job->failed = false;

    jobReader_.push(std::move(job));
}

// returns true if pool was expanded
bool Importer::expandWorkerPoolToCapacity() const
{
    bool isNotThreadLimited = workers_.size() < concurrentThreadsSupported_;
    bool isNotInputLimited = jobReader_.utilisation() < 0.99;
    bool isNotBufferLimited = true;  // TODO: get memoryUsed < maxMemoryCapacity from Adobe API

    if (isNotThreadLimited && isNotInputLimited && isNotBufferLimited) {
        workers_.push_back(std::make_unique<ImporterWorker>(error_, jobFreeList_, jobReader_, jobDecoder_));
        return true;
    }
    return false;
}
