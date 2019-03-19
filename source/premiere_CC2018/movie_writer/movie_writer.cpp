
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

extern "C" {
    extern AVOutputFormat ff_mov_muxer;
}
// =======================================================
MovieWriter::MovieWriter(VideoFormat videoFormat, VideoEncoderName encoderName,
    int width, int height, int encodedBitDepth,
    int64_t frameRateNumerator, int64_t frameRateDenominator,
    int32_t maxFrames, int32_t reserveMetadataSpace,
    MovieFile file, MovieErrorCallback onError,
    bool writeMoovTagEarly)
    : maxFrames_(maxFrames), reserveMetadataSpace_(reserveMetadataSpace),
      onWrite_(file.onWrite), onSeek_(file.onSeek), onClose_(file.onClose), onError_(onError),
      audioStream_(nullptr),
      iFrame_(0), closed_(false), error_(false),
      writeMoovTagEarly_(writeMoovTagEarly)
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
    videoStream_->codecpar->codec_id = AV_CODEC_ID_WRAPPED_AVFRAME;
    videoStream_->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    videoStream_->codecpar->width = width;
    videoStream_->codecpar->height = height;
    videoStream_->codecpar->bits_per_coded_sample = encodedBitDepth;
    av_dict_set(&videoStream_->metadata, "encoder", &encoderName[0], 0);

    /* timebase: This is the fundamental unit of time (in seconds) in terms
    * of which frame timestamps are represented. For fixed-fps content,
    * timebase should be 1/framerate and timestamp increments should be
    * identical to 1. */
    // which doesn't work for 29.97fps?
    // videoStream_->time_base will later get trashed by mov file format
    videoStream_->avg_frame_rate = streamTimebase_;
    videoStream_->time_base.den = streamTimebase_.den;
    videoStream_->time_base.num = 1;
    //videoStream_->codec->time_base = streamTimebase_; // deprecation warning, but it work...

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
    Dictionary movOptions;
    AVDictionary* movOptionsDictptr(nullptr);
    movOptions.reset(&movOptionsDictptr);

    if (writeMoovTagEarly_)
    {
        // avoid Adobe CC's post export copy step by giving ffmpeg enough info to put the moov header at
        // the start, including metadata
        auto predictedMoovSize = guessMoovSize();

        av_dict_set(&formatContext_->metadata, "xmp", std::string(reserveMetadataSpace_, ' ').c_str(), 0);
        av_dict_set(&movOptionsDictptr, "moov_size", std::to_string((int)(predictedMoovSize * 2)).c_str(), 0);
    }

    int ret = avformat_write_header(formatContext_.get(), movOptions.get()); // this is where the mov file format trashes the videoStream_ timebase
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
    pkt.dts = pkt.pts;
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
    pkt.dts = pkt.pts;
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
    if (ret == AVERROR(EINVAL)) {
        // this is the closest to knowing we didn't write enough header space
        throw MovieWriterInvalidData();
    } 
    else if (ret < 0)
        throw std::runtime_error(std::string("Error writing trailer: ") + av_err2str(ret).c_str());
}

int64_t MovieWriter::guessMoovSize()
{
    // structure of moov with audio will look something like this:
    //
    // moov 180757
    //   mvhd 108
    //   trak 171123               // video
    //     tkhd 92
    //     edts 36
    //     mdia 170987
    //       mdhd 32
    //       hdlr 45
    //       minf 170902
    //         vmhd 20
    //         hdlr 44
    //         dinf 36
    //         stbl 170794
    //           stsd 102
    //           stts 32
    //           stsc 868
    //           stsz 57280
    //           co64 112504
    //   trak 4429                 // audio
    //     tkhd 92
    //     edts 36
    //     mdia 4293
    //       mdhd 32
    //       hdlr 45
    //       minf 4208
    //         smhd 16
    //         hdlr 44
    //         dinf 36
    //         stbl 4104
    //           stsd 76
    //           stts 24
    //           stsc 1240
    //           stsz 20
    //           co64 2736
    //   udta 5089                 // user data
    //     ⌐swr 25
    //     ⌐TIM 23
    //     ⌐TSC 16
    //     ⌐TSZ 15
    //     XMP_ 5002

    // Quicktime terms
    //   a 'sample' is a video frame or an audio PCM multichannel datum point.
    //   'sample' size varies for video, will be uniform for audio
    //   'chunks' are groups of samples; used to create indexes for fast lookup of samples

    // the following calculations obtained by inspection of movenc.c in libavformat,
    // erring on side of larger. Atom sizes for / in the moov atom are 32-bit in movenc,
    // although the tables can use 64-bit variants in the event of large mdat body later
    // in the file 

    //   the number of chunks is optimisation-dependent, and the number of chunk groups is at most
    //   the number of chunks
    //   there could conceivably be maxFrames+1 entries here, and we don't know until all frames are written

    // number of video chunks is usually not a lot smaller than the number of frames
    // audio depends on interleaving, ie whether the muxer dumps whole blobs of contiguous
    // audio in places - number of samples in a chunk varied by a factor of 261 in one test case.
    // assume worst case - number of chunks = number of frames, based on the idea that more audio
    // chunks than video frames wouldn't make sense (adjacent audio chunks could be combined without
    // problems, as the sample size is uniform).

    auto n_video_chunks_guess = maxFrames_;
    auto n_audio_chunks_guess = maxFrames_;

    // stsd
    //   sample description
    auto video_stsd = 8 + (8 + 4 + 2 + 2 + 2 + 2 + 4 + 4 + 4 + 2 + 2 + 4 + 4 + 4 + 2 + 1 + 31 + 2 + 2 + 8);
    auto audio_stsd = 8 + 68;  // !!! from inspection above, needs verification, on cursory inspection is not varying size

    // stts
    //   time-to-sample, for looking up sample indices from time (on a timeline say).
    //   for the files we're writing this is completely uniform, although samples may be missing
    //   checked this for a file with frames missing from the middle - stts was still 24/32
    auto video_stts = 24;
    auto audio_stts = 32;

    // stss
    //   we're not writing this

    // stsc
    //   lists the number of samples in each group of chunks
    //   number of samples 

    //   based on a test encoding the number of entries was approx
    //     maxFrames / 250 for video,
    //     maxFrames / 180 for audio   <-- this seems determined by the granularity of the data pushed from the client
    auto video_entries_guess = n_video_chunks_guess / 60;
    auto audio_entries_guess = n_audio_chunks_guess / 60;
    auto video_stsc = 8 + 4 + 4 + 12 * video_entries_guess;
    auto audio_stsc = 8 + 4 + 4 + 12 * audio_entries_guess;

    // stsz
    //   sample sizes. Frame sizes for video. Uniform for audio.
    auto video_stsz = 8 + 4 + 4 + maxFrames_ * 4;
    auto audio_stsz = 8 + 4 + 4 + 4;

    // co64, stco
    //   chunk offset tables
    //   co64 uses 64-bit offsets; assume file might get big enough that this will be used
    auto video_co64_or_stco = 8 + 4 + 4 + n_video_chunks_guess * 8;
    auto audio_co64_or_stco = 8 + 4 + 4 + n_audio_chunks_guess * 8;

    // stbl
    //   sample table. for looking up samples
    auto video_stbl = 8 + video_stsd + video_stts + video_stsc + video_stsz + video_co64_or_stco; 
    auto audio_stbl = 8 + audio_stsd + audio_stts + audio_stsc + audio_stsz + audio_co64_or_stco;

    // minf
    auto vmhd = 20;
    auto smhd = 16;
    auto minf_hdlr = 44;
    auto dinf = 36;
    
    auto video_minf = 8 + vmhd + minf_hdlr + dinf + video_stbl;
    auto audio_minf = 8 + smhd + minf_hdlr + dinf + audio_stbl;

    // mdia
    auto mdhd = 44;
    auto mdia_hdlr = 45;

    auto video_mdia = 8 + mdhd + mdia_hdlr + video_minf;
    auto audio_mdia = 8 + mdhd + mdia_hdlr + audio_minf;

    // trak
    auto tkhd = 104;
    auto edts = 64;

    auto video_trak = tkhd + edts + video_mdia;
    auto audio_trak = tkhd + edts + audio_mdia;

    // udta
    auto udta_0x9swr = 25;
    auto udta_0x9TIM = 23;
    auto udta_0x9TSC = 16;
    auto udta_0x9TSZ = 15;
    auto udta_XMP_ = 8 + reserveMetadataSpace_;

    auto udta = 8 + udta_0x9swr + udta_0x9TIM + udta_0x9TSC + udta_0x9TSZ + udta_XMP_;

    // moov
    auto mvhd = 120;
    auto moov = 8 + mvhd + video_trak + ((audioStream_) ? audio_trak : 0) + udta;
    
    return moov;
}