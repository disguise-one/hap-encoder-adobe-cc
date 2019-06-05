#ifndef FFMPEG_HELPERS_HPP
#define FFMPEG_HELPERS_HPP

#include <array>
#include <functional>
#include <memory>

extern"C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

typedef std::array<char, 4> FileFormat;
typedef std::array<char, 4> VideoFormat;
typedef std::array<char, 31> VideoEncoderName;
typedef std::function<void ()> MovieOpenCallback;                               // throws error on fail
typedef std::function<void ()> MovieOpenForWriteCallback;
typedef std::function<size_t(uint8_t*, int size)> MovieReadCallback;            // return 0 on success, -ve on failure
typedef std::function<size_t(const uint8_t*, int size)> MovieWriteCallback;     // return 0 on success, -ve on failure
typedef std::function<int (int64_t offset, int whence)> MovieSeekCallback;      // return 0 on success, -ve on failure
typedef std::function<int ()> MovieCloseCallback;                               // return 0 on success, -ve on failure
typedef std::function<void (const char *)> MovieErrorCallback;                  // must not throw

struct MovieFile
{
    MovieOpenForWriteCallback onOpenForWrite;
    MovieWriteCallback onWrite;
    MovieSeekCallback onSeek;
    MovieCloseCallback onClose;
};

// wrappers for libav-* objects

struct FormatContextDeleter {
    void operator()(AVFormatContext *context)
    {
        avformat_free_context(context);
    }
};
typedef std::unique_ptr<AVFormatContext, FormatContextDeleter> FormatContext;

struct IOContextDeleter {
    void operator()(AVIOContext *ioContext)
    {
        avio_flush(ioContext);
        av_freep(&ioContext->buffer);
        avio_context_free(&ioContext);
    }
};
typedef std::unique_ptr<AVIOContext, IOContextDeleter> IOContext;

struct DictDeleter {
    void operator()(AVDictionary **dict)
    {
        av_dict_free(dict);
    }
};
typedef std::unique_ptr<AVDictionary *, DictDeleter> Dictionary;

enum AudioEncoding
{
    AudioEncoding_Unsigned_PCM,
    AudioEncoding_Signed_PCM
};

inline void setAVCodecParams(
    int numChannels, int sampleRate, int bytesPerSample, AudioEncoding encoding,
    AVCodecParameters& codecpar)
{
    codecpar.codec_type = AVMEDIA_TYPE_AUDIO;

    auto &codec_id = codecpar.codec_id;
    auto &format = codecpar.format;

    if (encoding == AudioEncoding_Signed_PCM)
    {
        switch (bytesPerSample) {
        case 1:
            codec_id = AV_CODEC_ID_PCM_S8;
            format = AV_SAMPLE_FMT_U8;
            break;
        case 2:
            codec_id = AV_CODEC_ID_PCM_S16LE;
            format = AV_SAMPLE_FMT_S16;
            break;
        case 4:
            codec_id = AV_CODEC_ID_PCM_S32LE;
            format = AV_SAMPLE_FMT_S32;
            break;
        }
    }
    else if (encoding == AudioEncoding_Unsigned_PCM)
    {
        switch (bytesPerSample) {
        case 1:
            codec_id = AV_CODEC_ID_PCM_U8;
            format = AV_SAMPLE_FMT_U8;
            break;
        case 2:
            codec_id = AV_CODEC_ID_PCM_U16LE;
            format = AV_SAMPLE_FMT_S16;
            break;
        case 4:
            codec_id = AV_CODEC_ID_PCM_U32LE;
            format = AV_SAMPLE_FMT_S32;
            break;
        }
    }
    codecpar.channels = numChannels;
    codecpar.channel_layout = av_get_default_channel_layout(numChannels);
    codecpar.sample_rate = sampleRate;
}

inline void getAVCodecParams(
    const AVCodecParameters& codecpar,
    int &numChannels, int &sampleRate, int &bytesPerSample, AudioEncoding &encoding)
{
    auto &format = codecpar.format;

    numChannels = codecpar.channels;
    sampleRate = codecpar.sample_rate;

    switch (codecpar.codec_id)
    {
    case AV_CODEC_ID_PCM_S8:
        bytesPerSample = 1;
        encoding = AudioEncoding_Signed_PCM;
        break;
    case AV_CODEC_ID_PCM_S16LE:
        bytesPerSample = 2;
        encoding = AudioEncoding_Signed_PCM;
        break;
    case AV_CODEC_ID_PCM_S32LE:
        bytesPerSample = 4;
        encoding = AudioEncoding_Signed_PCM;
        break;
    case AV_CODEC_ID_PCM_U8:
        bytesPerSample = 1;
        encoding = AudioEncoding_Unsigned_PCM;
        break;
    case AV_CODEC_ID_PCM_U16LE:
        bytesPerSample = 2;
        encoding = AudioEncoding_Unsigned_PCM;
        break;
    case AV_CODEC_ID_PCM_U32LE:
        bytesPerSample = 4;
        encoding = AudioEncoding_Unsigned_PCM;
        break;
    }
}

#endif