#ifndef MOVIE_READER_HPP
#define MOVIE_READER_HPP

#include <Windows.h>

#include "../movie/ffmpeg_helpers.hpp"


// ffmpeg libavformat-based file writing
class MovieReader
{
public:
    MovieReader(
        VideoFormat videoFormat,
        int64_t fileSize,
        MovieReadCallback onRead,
        MovieSeekCallback onSeek,
        MovieErrorCallback onError,
        MovieCloseCallback onClose
    );
    ~MovieReader();

    void readVideoFrame(int iFrame, std::vector<uint8_t>& frame);
    bool hasAudio() const { return audioStreamIdx_ != -1;  }

    int64_t fileSize() const { return fileSize_; }  // this is used by avio seek :(

    int width() const { return width_; }
    int height() const { return height_;  }
    int frameRateNumerator() const { return frameRateNumerator_; }
    int frameRateDenominator() const { return frameRateDenominator_; }
    int64_t numFrames() const { return numFrames_; }

private:
    std::string filespec_; // path + filename
    int64_t fileSize_;

    //CodecContext videoCodecContext_;
    FormatContext formatContext_;
    IOContext ioContext_;
    int videoStreamIdx_;
    int audioStreamIdx_;

    int width_;
    int height_;
    int frameRateNumerator_;
    int frameRateDenominator_;
    int64_t numFrames_;

    // adapt writers that throw exceptions
    static int c_onRead(void *context, uint8_t *data, int size);
    static int64_t c_onSeek(void *context, int64_t offset, int whence);
    // we're forced to allocate a buffer for AVIO.
    // TODO: get an no idea what an appropriate size is
    // TODO: hook libav allocation system so it can use externally cached memory management
    const size_t cAVIOBufferSize = 2 << 20;

    MovieReadCallback onRead_;
    MovieSeekCallback onSeek_;
    MovieErrorCallback onError_;
    MovieCloseCallback onClose_;
};

#endif   // MOVIE_WRITER_HPP