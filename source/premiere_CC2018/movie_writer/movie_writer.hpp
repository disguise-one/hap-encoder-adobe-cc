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

// wrappers for libav-* objects
#include "../movie/ffmpeg_helpers.hpp"

class MovieWriterInvalidData : public std::runtime_error
{
public:
    MovieWriterInvalidData() : std::runtime_error("invalid data")
    {
    }
};

// ffmpeg libavformat-based file writing
class MovieWriter
{
public:
    MovieWriter(VideoFormat videoFormat, VideoEncoderName encoderName,
                int width, int height,
                int encodedBitDepth,  // rgb=24, rgba=32 etc. Needs to be set correctly for some playback importers (eg After Effects) 
                int64_t frameRateNumerator, int64_t frameRateDenominator,
                int32_t maxFrames, int32_t reserveMetadataSpace,
                MovieFile file, MovieErrorCallback onError,
                bool writeMoovTagEarly
    );
    ~MovieWriter();

    //void addVideoStream(VideoFormat videoFormat, int width, int height, int64_t frameRateNumerator, int64_t frameRateDenominator);
    void addAudioStream(int numChannels, int sampleRate);
    void writeFrame(const uint8_t *data, size_t size);
    void writeAudioFrame(const uint8_t *data, size_t size, int64_t pts);
    void writeHeader();
    void writeTrailer();

    void close(); // can throw. Call ahead of destruction if onClose errors must be caught externally.

private:
    int64_t guessMoovSize();

    // need enough information to calculate space to reserve for moov atom
    int32_t maxFrames_;               // maximum number of frames that will be written
    int32_t reserveMetadataSpace_;    // space to reserve for XMP_ atom

    MovieWriteCallback onWrite_;
    MovieSeekCallback onSeek_;
    MovieCloseCallback onClose_;
    MovieErrorCallback onError_;

    bool writeMoovTagEarly_;

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
    AVStream *audioStream_;     // nullptr on audio not present
    AVRational streamTimebase_;
    int64_t iFrame_;

    bool closed_;
    bool error_;
};


#endif   // MOVIE_WRITER_HPP