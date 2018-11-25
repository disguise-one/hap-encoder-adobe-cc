#ifndef CODEC_REGISTRATION_H
#define CODEC_REGISTRATION_H

#include <array>
#include <functional>
#include <map>

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

struct DecodeOutput
{
    std::vector<uint8_t> buffer;
};

struct EncoderParametersBase {
    EncoderParametersBase(const FrameDef& frameDef_, int quality_)
        : frameDef(frameDef_), quality(quality_) {}
    virtual ~EncoderParametersBase() {}

    // ui-building
    // void describe(
    //    //const std:function<void (const std::string&, int, int, int &)>& slider
    // );

    FrameDef frameDef;
    int quality;

    EncoderParametersBase(const EncoderParametersBase&) = delete;
    EncoderParametersBase& operator=(const EncoderParametersBase&) = delete;
};

struct DecoderParametersBase {
    DecoderParametersBase(const FrameDef& frameDef_)
        : frameDef(frameDef_) {}
    virtual ~DecoderParametersBase() {}

    // ui-building
    // void describe(
    //    //const std:function<void (const std::string&, int, int, int &)>& slider
    // );

    FrameDef frameDef;

    DecoderParametersBase(const DecoderParametersBase&) = delete;
    DecoderParametersBase& operator=(const DecoderParametersBase&) = delete;
};

typedef std::array<char, 4> VideoFormat;

class EncoderJob {
public:
    EncoderJob() {}

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
    // derived EncoderJob classes  must implement these
    virtual void doCopyExternalToLocal(
        const uint8_t *bgraBottomLeftOrigin,
        size_t stride) = 0;

    virtual void doConvert() = 0;
    virtual void doEncode(EncodeOutput& out) = 0;

    EncoderJob(const EncoderJob& rhs) = delete;
    EncoderJob& operator=(const EncoderJob& rhs) = delete;
};

class Encoder {
public:
    Encoder(std::unique_ptr<EncoderParametersBase> parameters)
        : parameters_(std::move(parameters))
    {};
    virtual ~Encoder() {};

    virtual VideoFormat subType() const { throw std::exception("not implemented"); }
    const EncoderParametersBase& parameters() const { return *parameters_; }

    virtual std::unique_ptr<EncoderJob> create()=0;

private:
    Encoder(const Encoder&) = delete;
    Encoder& operator=(const Encoder&) = delete;

    std::unique_ptr<EncoderParametersBase> parameters_;
};

class Decoder {
public:
    Decoder(std::unique_ptr<EncoderParametersBase> parameters)
        : parameters_(std::move(parameters))
    {};
    virtual ~Decoder() {};

    virtual VideoFormat subType() const { throw std::exception("not implemented"); }
    const EncoderParametersBase& parameters() const { return *parameters_; }

    virtual std::unique_ptr<EncoderJob> create()=0;

    // decoding steps

    // CPU-side decompression, performed in a job-thread
    void decode(DecodeOutput& out)
    {
        doDecode(out);
    }

    // post-process after decompression, ready for copying back
    void convert()
    {
        doConvert();
    }

private:
    // derived EncoderJob classes  must implement these
    virtual void doCopyLocalToExtermal(
        uint8_t *bgraBottomLeftOrigin,
        size_t stride) = 0;

    virtual void doDecode(DecodeOutput& out) = 0;
    virtual void doConvert() = 0;

private:
    Decoder(const Decoder&) = delete;
    Decoder& operator=(const Decoder&) = delete;

    std::unique_ptr<EncoderParametersBase> parameters_;
};

// this class is instantiated by the exporter plugin as a singleton

// library clients must define
//   ctor / dtor - can wrap singleton initialisation / shutdown here
//   create factory function

class CodecRegistry {
public:
    static const CodecRegistry& codec();

    // opportunity to customise based on parameters
    std::function<std::unique_ptr<Encoder> (std::unique_ptr<EncoderParametersBase> parameters)> createEncoder;
    std::function<std::unique_ptr<Decoder> (std::unique_ptr<DecoderParametersBase> parameters)> createDecoder;

    // quality settings
    static bool hasQuality();
    static std::map<int, std::string> qualityDescriptions();

private:
    CodecRegistry();
};

#endif