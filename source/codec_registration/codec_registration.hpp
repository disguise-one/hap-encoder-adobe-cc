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

// Placeholders for inputs, processing and outputs for encode process

struct EncodeInput
{
    std::vector<uint8_t> rgbaTopLeftOrigin;
};

struct EncodeScratchpad
{
    // not all of these are used by all codecs
    std::vector<std::vector<uint8_t> > buffers;
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

class Codec {
public:
    Codec(std::unique_ptr<CodecParametersBase> parameters)
        : parameters_(std::move(parameters))
    {};
    ~Codec() {};

    virtual VideoFormat subType() const { throw std::exception("not implemented"); }
    const CodecParametersBase& parameters() const { return *parameters_; }

    // encoding steps
    void copyExternalToLocal(
        const uint8_t *bgraBottomLeftOrigin,
        size_t stride,
        EncodeInput& local) const
    {
        doCopyExternalToLocal(
            bgraBottomLeftOrigin,
            stride,
            local);
    }
    void convert(const EncodeInput& in, EncodeScratchpad& scratchpad) const
    {
        doConvert(in, scratchpad);
    }
    void encode(const EncodeScratchpad& scratchpad, EncodeOutput& out) const
    {
        doEncode(scratchpad, out);
    }

private:
    // derived codec classes  must implement these
    virtual void doCopyExternalToLocal(
        const uint8_t *bgraBottomLeftOrigin,
        size_t stride,
        EncodeInput& local) const
    {
    }
    virtual void doConvert(const EncodeInput& in, EncodeScratchpad& scratchpad) const
    {
    }
    virtual void doEncode(const EncodeScratchpad& scratchpad, EncodeOutput& out) const
    {
    }


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