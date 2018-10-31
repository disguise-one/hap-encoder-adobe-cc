#ifndef MOVIE_WRITER_HPP
#define MOVIE_WRITER_HPP

#include <array>
#include <functional>
#include <memory>

extern"C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

typedef std::array<char, 4> VideoFormat;
typedef std::function<size_t (const uint8_t*, int size)> MovieWriteCallback;    // return 0 on success, -ve on failure
typedef std::function<int (int64_t offset, int whence)> MovieSeekCallback;      // return 0 on success, -ve on failure
typedef std::function<int ()> MovieCloseCallback;                               // return 0 on success, -ve on failure
typedef std::function<void (const char *)> MovieErrorCallback;                  // must not throw

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


// ffmpeg libavformat-based file writing
class MovieWriter
{
public:
    MovieWriter(VideoFormat videoFormat,
                int width, int height,
                int64_t frameRateNumerator, int64_t frameRateDenominator,
                MovieWriteCallback onWrite,
                MovieSeekCallback onSeek,
                MovieCloseCallback onClose,
                MovieErrorCallback onError);
    ~MovieWriter();

    //void addVideoStream(VideoFormat videoFormat, int width, int height, int64_t frameRateNumerator, int64_t frameRateDenominator);
    void addAudioStream(int numChannels, int sampleRate);
    void writeFrame(const uint8_t *data, size_t size);
    void writeAudioFrame(const uint8_t *data, size_t size, int64_t pts);
    void writeHeader();
    void writeTrailer();

    void close(); // can throw. Call ahead of destruction if onClose errors must be caught externally.

private:
    MovieWriteCallback onWrite_;
    MovieSeekCallback onSeek_;
    MovieCloseCallback onClose_;
    MovieErrorCallback onError_;

    // adapt writers that throw exceptions
    static int c_onWrite(void *context, uint8_t *data, int size);
    static int64_t c_onSeek(void *context, int64_t offset, int whence);

    // we're forced to allocate a buffer for AVIO.
    // TODO: get an no idea what an appropriate size is
    // TODO: hook libav allocation system so it can use externally cached memory management
    const size_t cAVIOBufferSize = 2 << 20;

    //CodecContext videoCodecContext_;
    FormatContext formatContext_;
    IOContext ioContext_;
    AVStream *videoStream_;
    AVStream *audioStream_;
    AVRational streamTimebase_;
    int64_t iFrame_;

    bool closed_;
};


#endif   // MOVIE_WRITER_HPP