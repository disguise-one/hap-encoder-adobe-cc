#include "hapencode.hpp"
#include "codec/codec.hpp"
#include "configure.hpp"

Exporter::Exporter(
    std::unique_ptr<Codec> codec,
    std::unique_ptr<MovieWriter> writer,
    int64_t nFrames)
  : codec_(std::move(codec)), writer_(std::move(writer)), nFrames_(nFrames),
    nextFrameToWrite_(0)

{
}

Exporter::~Exporter()
{
}

void Exporter::dispatch(int64_t iFrame, const uint8_t* bgra_bottom_left_origin_data, size_t stride) const
{
    std::unique_ptr<ExporterEncodeBuffers> buffers = getEncodeBuffers();

    // convert Premiere frame to HAP frame
    buffers->input.bgraBottomLeftOrigin = (uint8_t *)bgra_bottom_left_origin_data;
    buffers->input.stride = stride;

    codec_->encode(buffers->input, buffers->scratchpad, buffers->output);

    // write it out
    writeInSequence(iFrame, std::move(buffers));
}

std::unique_ptr<ExporterEncodeBuffers> Exporter::getEncodeBuffers() const
{
    std::lock_guard<std::mutex> guard(freeListMutex_);
    if (!freeList_.empty())
    {
        auto ret = std::move(freeList_.back());
        freeList_.pop_back();
        return ret;
    }
    else
        return std::make_unique<ExporterEncodeBuffers>();

}

void Exporter::recycleEncodeBuffers(std::unique_ptr<ExporterEncodeBuffers> toRecycle) const
{
    std::lock_guard<std::mutex> guard(freeListMutex_);
    freeList_.push_back(std::move(toRecycle));
}

void Exporter::writeInSequence(int64_t iFrame, std::unique_ptr<ExporterEncodeBuffers> buffers) const
{
    std::lock_guard<std::mutex> writeQueueGuard(writeQueueMutex_);
    writeQueue_.push_back(std::pair<int64_t, std::unique_ptr<ExporterEncodeBuffers> >(iFrame, std::move(buffers)));

    bool written;
    do
    {
        written = false;
        for (auto i = writeQueue_.begin(); i != writeQueue_.end(); ++i)
        {
            if (i->first == nextFrameToWrite_)
            {
                std::unique_ptr<ExporterEncodeBuffers> buffers = std::move(i->second);
                writer_->writeFrame(&buffers->output.buffer[0], buffers->output.buffer.size());
                nextFrameToWrite_++;
                writeQueue_.erase(i);
                recycleEncodeBuffers(std::move(buffers));
                written = true;
                break;
            }
        }
    } while (written);

    if (nextFrameToWrite_ > nFrames_)
    {
        writer_.reset(nullptr);
    }
}

csSDK_int32 getPixelFormatSize(const PrFourCC subtype)
{
	return 4;
}

