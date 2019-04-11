#ifndef CODEC_REGISTRATION_H
#define CODEC_REGISTRATION_H

#include <array>
#include <functional>
#include <map>

// Details of frame
enum FrameHostFormat
{
    frameHostFormat_u8,      // host is argb with 8-bits unsigned per channel
    frameHostFormat_u16_32k, //         argb with 16-bits unsigned per channel normalised 0-1 -> 0->32768
    frameHostFormat_f32      //         argb with float channel normalised ? - ? -> ? - ?
};

struct FrameDef
{
    FrameDef(int width_, int height_, bool isHighBitDepth_)
        : width(width_), height(height_), isHighBitDepth(isHighBitDepth_),
          hostFormat(isHighBitDepth_ ? frameHostFormat_u16_32k : frameHostFormat_u8)
    { }

    int width;
    int height;
    FrameHostFormat hostFormat;
    bool isHighBitDepth;    // !!! enum would be better; moving to this (hostFormat)

    size_t bytesPerPixel() const {
        switch (hostFormat) {
        case frameHostFormat_u16_32k:
            return 8;
        case frameHostFormat_f32:
            return 16;
        case frameHostFormat_u8:
        default:
            return 4;
        }
    }
    bool hostFormat_u16_32k() const { return hostFormat == frameHostFormat_u16_32k; }
    bool hostFormat_f32() const { return hostFormat == frameHostFormat_f32;  }
    bool hostFormat_u8() const { return hostFormat == frameHostFormat_u8; }
};

struct EncodeOutput
{
    std::vector<uint8_t> buffer;
};

struct DecodeInput
{
    std::vector<uint8_t> buffer;
};

enum CodecAlpha
{
    withoutAlpha = 0,
    withAlpha = 1
};

struct EncoderParametersBase {
    EncoderParametersBase(const FrameDef& frameDef_, CodecAlpha alpha_, int quality_)
        : frameDef(frameDef_), alpha(alpha_), quality(quality_) {}
    virtual ~EncoderParametersBase() {}

    // ui-building
    // void describe(
    //    //const std:function<void (const std::string&, int, int, int &)>& slider
    // );

    FrameDef frameDef;
    CodecAlpha alpha;
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

typedef std::array<char, 4> FileFormat;
typedef std::array<char, 4> VideoFormat;
typedef std::array<char, 31> VideoEncoderName;

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

class DecoderJob {
public:
    DecoderJob() {}

    // encoding steps

    // CPU-side decompression, performed in a job-thread
    void decode(std::vector<uint8_t>& in)  //!!! 'in' buffer is swapped with local
    {
        doDecode(in);
    }

    // convert to texture ready for delivery
    void convert()
    {
        doConvert();
    }

    // deliver the texture
    void copyLocalToExternal(
        uint8_t *bgraBottomLeftOrigin,
        size_t stride) const
    {
        doCopyLocalToExternal(
            bgraBottomLeftOrigin,
            stride);
    }

private:
    virtual void doDecode(std::vector<uint8_t>& in) = 0;
    virtual void doConvert() = 0;
    // derived EncoderJob classes  must implement these
    virtual void doCopyLocalToExternal(
        uint8_t *bgraBottomLeftOrigin,
        size_t stride) const = 0;

    DecoderJob(const DecoderJob& rhs) = delete;
    DecoderJob& operator=(const DecoderJob& rhs) = delete;
};

class Encoder {
public:
    Encoder(std::unique_ptr<EncoderParametersBase> parameters)
        : parameters_(std::move(parameters))
    {};
    virtual ~Encoder() {};

    virtual VideoFormat subType() const { throw std::exception("not implemented"); }
    virtual VideoEncoderName name() const { throw std::exception("not implemented"); }
    const EncoderParametersBase& parameters() const { return *parameters_; }
    int encodedBitDepth() const { return (parameters_->alpha == withoutAlpha) ? 24 : 32; }

    virtual std::unique_ptr<EncoderJob> create()=0;

private:
    Encoder(const Encoder&) = delete;
    Encoder& operator=(const Encoder&) = delete;

    std::unique_ptr<EncoderParametersBase> parameters_;
};

class Decoder {
public:
    Decoder(std::unique_ptr<DecoderParametersBase> parameters)
        : parameters_(std::move(parameters))
    {};
    virtual ~Decoder() {};

    virtual VideoFormat subType() const { throw std::exception("not implemented"); }
    const DecoderParametersBase& parameters() const { return *parameters_; }

    virtual std::unique_ptr<DecoderJob> create() = 0;

private:
    Decoder(const Decoder&) = delete;
    Decoder& operator=(const Decoder&) = delete;

    std::unique_ptr<DecoderParametersBase> parameters_;
};

// this class is instantiated by the exporter plugin as a singleton

// library clients must define
//   ctor / dtor - can wrap singleton initialisation / shutdown here
//   create factory function

typedef std::unique_ptr<Encoder, std::function<void(Encoder *)>> UniqueEncoder;
typedef std::unique_ptr<Decoder, std::function<void(Decoder *)>> UniqueDecoder;

class CodecRegistry {
public:
    static std::shared_ptr<CodecRegistry>& codec();

    // opportunity to customise based on parameters
    std::function<UniqueEncoder (std::unique_ptr<EncoderParametersBase> parameters)> createEncoder;
    std::function<UniqueDecoder (std::unique_ptr<DecoderParametersBase> parameters)> createDecoder;


    // codec properties
    static std::string fileFormatName();
    static std::string fileFormatShortName();
    static FileFormat fileFormat();
    //!!! these need to be broken out per codec subtype
    static VideoFormat videoFormat();
    static bool isHighBitDepth();       // should host expect high bit depth from this codec

    // as much information about the codec that will be doing the job as possible - eg gpu vs cpu, codebase etc
    // for output to log
    static std::string logName();

    // quality settings
    static bool hasQuality();
    static std::map<int, std::string> qualityDescriptions();

    //!!! should be private
    CodecRegistry();
private:
    friend std::shared_ptr<CodecRegistry>;

    static std::string logName_;  // !!! simplification; should be moved; also assert thread safety [depend on CC for this atm]
};

#endif