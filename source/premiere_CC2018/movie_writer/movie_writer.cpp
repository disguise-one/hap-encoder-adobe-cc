
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <string>
#include <numeric>

#define __STDC_CONSTANT_MACROS

extern "C" {
#include <libavutil/channel_layout.h>
}


#include "movie_writer.hpp"


#undef av_err2str
std::string av_err2str(int errnum)
{
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, errnum);
    return buffer;
}

const char *kVideoCodecName = "hap";
const char *kVideoCodecLongName = "Vidvox Hap";
extern "C" {
    extern AVOutputFormat ff_mov_muxer;
}
// =======================================================
MovieWriter::MovieWriter(VideoFormat videoFormat,
    int width, int height,
    int64_t frameRateNumerator, int64_t frameRateDenominator,
    MovieWriteCallback onWrite,
    MovieSeekCallback onSeek,
    MovieCloseCallback onClose,
    MovieErrorCallback onError)
    : onWrite_(onWrite), onSeek_(onSeek), onClose_(onClose), onError_(onError), iFrame_(0), closed_(false)
{
    /* allocate the output media context */
    AVFormatContext *formatContext = avformat_alloc_context();
    formatContext->oformat = &ff_mov_muxer;
    if (!formatContext)
        throw std::runtime_error("Could not allocate format context");
    formatContext_.reset(formatContext); // and own it

    int64_t frameRateGCD = std::gcd(frameRateNumerator, frameRateDenominator);
    streamTimebase_.num = static_cast<int>(frameRateDenominator / frameRateGCD);
    streamTimebase_.den = static_cast<int>(frameRateNumerator / frameRateGCD);
    // TODO: rename streamTimebase_ to videoTimebase_ to differ video and audio streams

    /* Add the video stream */
    // becomes owned by formatContext
    videoStream_ = avformat_new_stream(formatContext_.get(), NULL);
    if (!videoStream_) {
        throw std::runtime_error("Could not allocate stream");
    }
    videoStream_->id = formatContext_->nb_streams - 1;
    videoStream_->codecpar->codec_tag = MKTAG(videoFormat[0], videoFormat[1], videoFormat[2], videoFormat[3]);
    videoStream_->codecpar->codec_id = AV_CODEC_ID_HAP;
    videoStream_->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    videoStream_->codecpar->width = width;
    videoStream_->codecpar->height = height;

    /* timebase: This is the fundamental unit of time (in seconds) in terms
    * of which frame timestamps are represented. For fixed-fps content,
    * timebase should be 1/framerate and timestamp increments should be
    * identical to 1. */
    // which doesn't work for 29.97fps?
    // videoStream_->time_base will later get trashed by mov file format
    videoStream_->time_base = streamTimebase_;
    videoStream_->codec->time_base = streamTimebase_; // deprecation warning, but it work...

    uint8_t* buffer = (uint8_t*)av_malloc(cAVIOBufferSize);
    if (!buffer)
        throw std::runtime_error("couldn't allocate write buffer");

    AVIOContext *ioContext = avio_alloc_context(
        buffer,               // unsigned char *buffer,
        (int)cAVIOBufferSize, // int buffer_size,
        1,                    // int write_flag,
        this,                 // void *opaque,
        nullptr,              // int(*read_packet)(void *opaque, uint8_t *buf, int buf_size),
        c_onWrite,            // int(*write_packet)(void *opaque, uint8_t *buf, int buf_size),
        c_onSeek);            // int64_t(*seek)(void *opaque, int64_t offset, int whence));
    if (!ioContext)
    {
        av_free(buffer);  // not owned by anyone yet :(
        throw std::runtime_error("couldn't allocate io context");
    }
    ioContext_.reset(ioContext);

    formatContext->pb = ioContext_.get();
    //writeHeader();
}

void MovieWriter::writeHeader()
{
    /* Write the stream header, if any. */
    int ret = avformat_write_header(formatContext_.get(), nullptr); // this is where the mov file format trashes the videoStream_ timebase
    if (ret < 0) {
        throw std::runtime_error(std::string("Error occurred when writing header: ") + av_err2str(ret).c_str());
    }
}

MovieWriter::~MovieWriter()
{
    try
    {
        close();
    }
    catch (const std::exception& ex)
    {
        onError_(ex.what());
    }
    catch (...)
    {
        onError_("unhandled error on closing");
    }
}

void MovieWriter::close()
{
    if (!closed_)
    {
        closed_ = true;

        if (onClose_() < 0)
        {
            throw std::runtime_error("error while closing");
        }
    }
}

int MovieWriter::c_onWrite(void *context, uint8_t *data, int size)
{
    MovieWriter *writer = reinterpret_cast<MovieWriter*>(context);
    try
    {
        return (int)writer->onWrite_(data, size);
    }
    catch (const std::exception &ex)
    {
        writer->onError_(ex.what());
        return -1;
    }
    catch (...)
    {
        writer->onError_("unhandled exception while writing");
        return -1;
    }
    return size;
}

int64_t MovieWriter::c_onSeek(void *context, int64_t seekPos, int whence)
{
    MovieWriter *writer = reinterpret_cast<MovieWriter*>(context);
    try
    {
        return writer->onSeek_(seekPos, whence);
    }
    catch (const std::exception &ex)
    {
        writer->onError_(ex.what());
        return -1;
    }
    catch (...)
    {
        writer->onError_("unhandled exception while seeking");
        return -1;
    }
}

void MovieWriter::addAudioStream(int numChannels, int sampleRate)
{
    audioStream_ = avformat_new_stream(formatContext_.get(), NULL);
    if (!audioStream_)
        throw std::runtime_error("Could not allocate audio stream");

    audioStream_->id = formatContext_->nb_streams - 1;
    audioStream_->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    audioStream_->codecpar->codec_id = AV_CODEC_ID_PCM_S16LE;
    audioStream_->codecpar->format = AV_SAMPLE_FMT_S16;
    audioStream_->codecpar->channels = numChannels;
    audioStream_->codecpar->channel_layout = av_get_default_channel_layout(numChannels);
    audioStream_->codecpar->sample_rate = sampleRate;
}

void MovieWriter::writeFrame(const uint8_t *data, size_t size)
{
    AVPacket pkt = { 0 };

    av_init_packet(&pkt);
    pkt.data = const_cast<uint8_t *>(data);
    pkt.size = (int)size;
    pkt.stream_index = videoStream_->index;
    pkt.pts = iFrame_++;
    av_packet_rescale_ts(&pkt, streamTimebase_, videoStream_->time_base);
    pkt.flags = AV_PKT_FLAG_KEY;

    /* Write the compressed frame to the media file. */
    int ret = av_interleaved_write_frame(formatContext_.get(), &pkt);
    if (ret < 0)
    {
        throw std::runtime_error(std::string("Error while writing video frame: ") + av_err2str(ret).c_str());
    }
}

void MovieWriter::writeAudioFrame(const uint8_t *data, size_t size, int64_t pts)
{
    AVPacket pkt = { 0 };

    av_init_packet(&pkt);
    pkt.data = const_cast<uint8_t *>(data);
    pkt.size = (int)size;
    pkt.stream_index = audioStream_->index;
    pkt.pts = pts;
    //av_packet_rescale_ts(&pkt, streamTimebase_, audioStream_->time_base);
    pkt.flags = AV_PKT_FLAG_KEY;

    /* Write the compressed frame to the media file. */
    int ret = av_interleaved_write_frame(formatContext_.get(), &pkt);
    if (ret < 0)
    {
        throw std::runtime_error(std::string("Error while writing audio frame: ") + av_err2str(ret).c_str());
    }
}

void MovieWriter::writeTrailer()
{
    /* Write the trailer, if any. The trailer must be written before you
    * close the CodecContexts open when you wrote the header; otherwise
    * av_write_trailer() may try to use memory that was freed on
    * av_codec_close(). */
    int ret = av_write_trailer(formatContext_.get());
    if (ret < 0)
        throw std::runtime_error(std::string("Error writing trailer: ") + av_err2str(ret).c_str());
}
