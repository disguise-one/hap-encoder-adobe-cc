#ifndef CODEC_REGISTRATION_H
#define CODEC_REGISTRATION_H

#include <array>
#include <functional>

// Details of frame

struct FrameDef
{
    FrameDef(int width_, int height_)
        : width(width_), height(height_)
    { }

    int width;
    int height;
};

struct EncodeOutput
{
    std::vector<uint8_t> buffer;
};

struct CodecParametersBase {
    CodecParametersBase(const FrameDef& frameDef_)
        : frameDef(frameDef_) {}
    virtual ~CodecParametersBase() {}

    // ui-building
    // void describe(
    //    //const std:function<void (const std::string&, int, int, int &)>& slider
    // );

    FrameDef frameDef;

    CodecParametersBase(const CodecParametersBase&) = delete;
    CodecParametersBase& operator=(const CodecParametersBase&) = delete;
};

typedef std::array<char, 4> VideoFormat;

class CodecJob {
public:
    CodecJob() {}

    // respond to a new incoming frame in the main thread; returns as fast as possible
    void copyExternalToLocal(
        const uint8_t *bgraBottomLeftOrigin,
        size_t stride)
    {
        doCopyExternalToLocal(
            bgraBottomLeftOrigin,
            stride);
    }

    // encoding steps

    // prepare for CPU-side compression, performed in a job-thread
    void convert()
    {
        doConvert();
    }

    // CPU-side compression, performed in a job-thread
    void encode(EncodeOutput& out)
    {
        doEncode(out);
    }

private:
    // derived CodecJob classes  must implement these
    virtual void doCopyExternalToLocal(
        const uint8_t *bgraBottomLeftOrigin,
        size_t stride) = 0;

    virtual void doConvert() = 0;
    virtual void doEncode(EncodeOutput& out) = 0;

    CodecJob(const CodecJob& rhs) = delete;
    CodecJob& operator=(const CodecJob& rhs) = delete;
};

class Codec {
public:
    Codec(std::unique_ptr<CodecParametersBase> parameters)
        : parameters_(std::move(parameters))
    {};
    virtual ~Codec() {};

    virtual VideoFormat subType() const { throw std::exception("not implemented"); }
    const CodecParametersBase& parameters() const { return *parameters_; }

    virtual std::unique_ptr<CodecJob> create()=0;

private:
    Codec(const Codec&) = delete;
    Codec& operator=(const Codec&) = delete;

    std::unique_ptr<CodecParametersBase> parameters_;
};

// this class is instantiated by the exporter plugin

// library clients must define
//   ctor / dtor - can wrap singleton initialisation / shutdown here
//   create factory function

class CodecRegistry {
public:
    CodecRegistry();

    // opportunity to customise based on parameters
    static std::unique_ptr<Codec> create(std::unique_ptr<CodecParametersBase> parameters);

private:
};

#endif