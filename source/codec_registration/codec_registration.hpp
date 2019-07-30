#ifndef CODEC_REGISTRATION_H
#define CODEC_REGISTRATION_H

#include <array>
#include <functional>
#include <map>
#include <vector>
#include <string>

// Details of frame

// ChannelFormat, ChannelLayout and FrameOrigin are | combined into FrameFormat
typedef uint32_t FrameFormat;

#define ChannelFormatMask 0xFF
#define ChannelLayoutMask 0xFF00
#define FrameOriginMask 0xFF0000

enum ChannelFormat : uint32_t
{
    ChannelFormat_U8      = 0x01,      //  8 bits 0-255
    ChannelFormat_U16_32k = 0x02,      // 16 bits 0-32768  (not 32767; matches AEX)
    ChannelFormat_U16     = 0x03,      // 16 bits 0-65535
    ChannelFormat_F32     = 0x04       // 32 bits 0 - 1.0f typically
};

enum ChannelLayout : uint32_t
{
    ChannelLayout_ARGB = 0x0100,
    ChannelLayout_BGRA = 0x0200
};

enum FrameOrigin : uint32_t
{
    FrameOrigin_BottomLeft = 0x010000,
    FrameOrigin_TopLeft    = 0x020000,
};


struct FrameDef
{
    FrameDef(int width_, int height_,
             FrameFormat format_)
        : width(width_), height(height_), format(format_)
    { }

    int width;
    int height;
    FrameFormat format;

    ChannelFormat channelFormat() const { return (ChannelFormat) (format & ChannelFormatMask); }
    ChannelLayout channelLayout() const{ return (ChannelLayout) (format & ChannelLayoutMask); }
    FrameOrigin origin() const { return (FrameOrigin) (format & FrameOriginMask); };

    size_t bytesPerPixel() const {
        switch (channelFormat()) {
        case ChannelFormat_U16:
        case ChannelFormat_U16_32k:
            return 8;
        case ChannelFormat_F32:
            return 16;
        case ChannelFormat_U8:
        default:
            return 4;
        }
    }
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

typedef std::array<char, 4> CodecSubType;
typedef std::array<unsigned int, 2> HapChunkCounts;  //!!! move this

struct EncoderParametersBase {
    EncoderParametersBase(const FrameDef& frameDef_, CodecAlpha alpha_, bool hasSubType_, CodecSubType subType_,
                          bool hasChunkCount_, HapChunkCounts chunkCounts_, int quality_)
        : frameDef(frameDef_), alpha(alpha_), hasSubType(hasSubType_), subType(subType_),
          hasChunkCount(hasChunkCount_), chunkCounts(chunkCounts_), quality(quality_) {}
    virtual ~EncoderParametersBase() {}

    // ui-building
    // void describe(
    //    //const std:function<void (const std::string&, int, int, int &)>& slider
    // );

    FrameDef frameDef;
    CodecAlpha alpha;
    bool hasSubType;
    CodecSubType subType;
    bool hasChunkCount;
    HapChunkCounts chunkCounts; //!!! move this
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
    virtual ~EncoderJob() {};

    // respond to a new incoming frame in the main thread; returns as fast as possible
    void copyExternalToLocal(
        const uint8_t *data,
        size_t stride,
        FrameFormat format)
    {
        doCopyExternalToLocal(
            data,
            stride,
            format);
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
        const uint8_t *data,
        size_t stride,
        FrameFormat format) = 0;

    virtual void doConvert() = 0;
    virtual void doEncode(EncodeOutput& out) = 0;

    EncoderJob(const EncoderJob& rhs) = delete;
    EncoderJob& operator=(const EncoderJob& rhs) = delete;
};

class DecoderJob {
public:
    DecoderJob() {}
    virtual ~DecoderJob() {};

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
        uint8_t *data,
        size_t stride) const
    {
        doCopyLocalToExternal(
            data,
            stride);
    }

private:
    virtual void doDecode(std::vector<uint8_t>& in) = 0;
    virtual void doConvert() = 0;
    // derived EncoderJob classes  must implement these
    virtual void doCopyLocalToExternal(
        uint8_t *data,
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

    virtual VideoFormat subType() const { throw std::runtime_error("not implemented"); }
    virtual VideoEncoderName name() const { throw std::runtime_error("not implemented"); }
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

    virtual VideoFormat subType() const { throw std::runtime_error("not implemented"); }
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

typedef std::pair<CodecSubType, std::string> CodecNamedSubType;
typedef std::vector<CodecNamedSubType> CodecNamedSubTypes;

struct CodecDetails
{
    std::string productName;         // could be '<product> by <entity>'
    std::string fileFormatName;
    std::string fileFormatShortName;
    std::string videoFileExt;
    FileFormat fileFormat;
    VideoFormat videoFormat;
    CodecNamedSubTypes subtypes;      // leave empty for no subtypes
    CodecSubType defaultSubType;
    bool hasExplicitIncludeAlphaChannel;
    bool hasChunkCount;
    std::string premiereGroupName;               // Adobe Premiere group name for storage
    std::string premiereIncludeAlphaChannelName; // Adobe Premiere include alpha channel for storage(backwards compat)
    std::string premiereChunkCountName;          // Adobe Premiere chunk count name for storage (backwards compat)
    uint32_t afterEffectsSig;       // AfterEffects output module registration info - docs suggest letting Adobe know what you use here
    uint32_t afterEffectsCreator;   // other AEX reg info - _not_ exactly sure how this is is used by AEX
    uint32_t afterEffectsType;      // ditto
    uint32_t afterEffectsMacType;   // ditto
};

class CodecRegistry {
public:
    static std::shared_ptr<CodecRegistry>& codec();

    // opportunity to customise based on parameters
    std::function<UniqueEncoder (std::unique_ptr<EncoderParametersBase> parameters)> createEncoder;
    std::function<UniqueDecoder (std::unique_ptr<DecoderParametersBase> parameters)> createDecoder;


    // codec properties
    static const CodecDetails& details();
    static int getPixelFormatSize(bool hasSubType, CodecSubType subType); // !!! for bitrate calculation; should be moved to encoder

    static bool isHighBitDepth();       // should host expect high bit depth from this codec

    // as much information about the codec that will be doing the job as possible - eg gpu vs cpu, codebase etc
    // for output to log
    static std::string logName();

    // quality settings

    // qualities are ordered integers, not necessarily starting at 0 nor contiguous
    static bool hasQualityForAnySubType();
    static bool hasQuality(const CodecSubType& subtype);
    static std::map<int, std::string> qualityDescriptions();
    static int defaultQuality();

    //!!! should be private
    CodecRegistry();
private:
    friend std::shared_ptr<CodecRegistry>;

    static std::string logName_;  // !!! simplification; should be moved; also assert thread safety [depend on CC for this atm]
};

#endif