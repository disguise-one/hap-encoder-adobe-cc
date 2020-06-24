#include <stdexcept>
#include <string>
#include <tmmintrin.h>

#include "hap.h"

#include "codec.hpp"
#include "util.hpp"

int roundUpToMultipleOf4(int n)
{
    return (n + 3) & ~3;
}

const Codec4CC kHapCodecSubType{ 'H' , 'a', 'p', '1' };
const Codec4CC kHapAlphaCodecSubType{ 'H', 'a', 'p', '5' };
const Codec4CC kHapYCoCgCodecSubType{ 'H', 'a', 'p', 'Y' };
const Codec4CC kHapYCoCgACodecSubType{ 'H', 'a', 'p', 'M' };
const Codec4CC kHapAOnlyCodecSubType{ 'H', 'a', 'p', 'A' };

const CodecDetails& CodecRegistry::details()
{
    CodecNamedSubTypes hapCodecSubtypes{
        CodecNamedSubType{kHapCodecSubType, "Hap"},
        CodecNamedSubType{kHapAlphaCodecSubType, "Hap Alpha"},
        CodecNamedSubType{kHapYCoCgCodecSubType,"Hap Q"},
        CodecNamedSubType{kHapYCoCgACodecSubType,"Hap Q Alpha"},
        CodecNamedSubType{kHapAOnlyCodecSubType, "Hap Alpha-Only"} };
    
    static CodecDetails details{
        "HAP", // productName
        "HAP Movie", // fileFormatName;
        "HAP", // fileFormatShortName;
        "mov",  // videoFileExt
        FileFormat{'p', 'a', 'h', '\0'}, // fileFormat
        VideoFormat{'Y', 'P', 'A', 'H'}, // videoFormat
        hapCodecSubtypes, // codecSubTypes
        kHapAlphaCodecSubType, // defaultSubType
        false,  // isHighBitDepth
        false,  // hasExplicitIncludeAlphaChannel
        true,   // hasChunkCount
        QualityCodecDetails{
            true,
            { { kHapCodecSubType, true },
              { kHapAlphaCodecSubType, true },
              { kHapYCoCgCodecSubType, false },
              { kHapYCoCgACodecSubType, false },
              { kHapAOnlyCodecSubType, false } },  // presentForSubtype
            { { kSquishEncoderFastQuality, "Fast" },
              {kSquishEncoderNormalQuality, "Normal" } }, // descriptions
            kSquishEncoderNormalQuality},  // defaultQuality
        5,                        // premiereParamsVersion
        "HAPSpecificCodecGroup",  // premiereGroupName
        std::string(),            // premiereIncludeAlphaChannelNmae
        "HAPChunkCount",          // premiereChunkCountName
        'HAPP',                   // premiereSig [for afterEffects, must differ from afterEffectsSig]
        'HAPA',                   // afterEffectsSig
        'DTEK',                   // afterEffectsCreator
        'HAP_',                   // afterEffectsType
        'HAP_'                    // afterEffectsMacType
    };

    return details;
}

CodecRegistry::CodecRegistry()
{
    logName_ = "HAP";

    createEncoder = [=](std::unique_ptr<EncoderParametersBase> parameters) -> UniqueEncoder
    {
        return UniqueEncoder(new HapEncoder(parameters),
            [](Encoder* encoder) { delete encoder; });
    };

    // createDecoder = [=](std::unique_ptr<DecoderParametersBase> parameters) -> UniqueDecoder
    // {
    //     return nullptr;
    // };
}

std::shared_ptr<CodecRegistry>& CodecRegistry::codec()
{
    static std::shared_ptr<CodecRegistry> codecRegistry = std::make_shared<CodecRegistry>();
    return codecRegistry;
}

double CodecRegistry::getPixelFormatSize(
        CodecAlpha alpha,
        Codec4CC subType,
        int quality)
{
    return 4; // !!! not correct, for bitrate estimation; should be moved to encoder
}

std::string CodecRegistry::logName_;
std::string CodecRegistry::logName()
{
    return logName_;
}

HapEncoder::HapEncoder(std::unique_ptr<EncoderParametersBase>& params)
    : Encoder(std::move(params)),
      count_(parameters().codec4CC == kHapYCoCgACodecSubType ? 2 : 1),
      chunkCounts_((parameters().chunkCounts == HapChunkCounts{ 0, 0 })
                   ? HapChunkCounts{ 1, 1 }
                   : parameters().chunkCounts),     // auto represented as 0, 0
      textureFormats_(getTextureFormats(parameters().codec4CC)),
      compressors_{ HapCompressorSnappy, HapCompressorSnappy }
{
    SquishEncoderQuality quality = (SquishEncoderQuality)parameters().quality;
    for (size_t i = 0; i < count_; ++i)
    {
        converters_[i] = TextureConverter::create(parameters().frameSize, textureFormats_[i], quality);
        sizes_[i] = (unsigned long)converters_[i]->size();
    }
}

HapEncoder::~HapEncoder()
{
}

std::unique_ptr<EncoderJob> HapEncoder::create()
{
    std::array<TextureConverter*, 2> converters;
    for (size_t i = 0; i < count_; ++i)
        converters[i] = converters_[i].get();   //!!! should probably move things like this back on HapEncoder

    return std::make_unique<HapEncoderJob>(
            parameters().frameSize,
            count_,
            chunkCounts_,
            textureFormats_,
            compressors_,
            converters,
            sizes_
        );
}

std::array<unsigned int, 2> HapEncoder::getTextureFormats(Codec4CC subType)
{
    if (subType == kHapCodecSubType) {
        return { HapTextureFormat_RGB_DXT1 };
    }
    else if (subType == kHapAlphaCodecSubType) {
        return { HapTextureFormat_RGBA_DXT5 };
    }
    else if (subType == kHapYCoCgCodecSubType) {
        return { HapTextureFormat_YCoCg_DXT5 };
    }
    else if (subType == kHapYCoCgACodecSubType) {
        return { HapTextureFormat_YCoCg_DXT5, HapTextureFormat_A_RGTC1 };
    }
    else if (subType == kHapAOnlyCodecSubType) {
        return { HapTextureFormat_A_RGTC1 };
    }
    else
        throw std::runtime_error("unknown codec");
}

HapEncoderJob::HapEncoderJob(
    FrameSize frameSize,
    unsigned int count,
    HapChunkCounts chunkCounts,
    std::array<unsigned int, 2> textureFormats,
    std::array<unsigned int, 2> compressors,
    std::array<TextureConverter*, 2> converters,
    std::array<unsigned long, 2> sizes)
    : frameSize_(frameSize),
      count_(count),
      chunkCounts_(chunkCounts),
      textureFormats_(textureFormats),
      compressors_(compressors),
      converters_(converters),
      sizes_(sizes)
{
}

void HapEncoderJob::doCopyExternalToLocal(
    const uint8_t *data, size_t stride, FrameFormat format)
{
    rgbaTopLeftOrigin_.resize(frameSize_.width * frameSize_.height * 4);

    // convert host format to rgba top left origin
    FrameDef frameDef(frameSize_, format);
    convertHostFrameTo_RGBA_Top_Left_U8(data, stride, frameDef, (uint8_t*)& rgbaTopLeftOrigin_[0], frameSize_.width * 4);
}

void HapEncoderJob::doEncode(EncodeOutput& out)
{
    // convert input texture from rgba to <subcodec defined> dxt [+ dxt]
    for (unsigned int i = 0; i < count_; ++i)
    {
        converters_[i]->convert(
            &rgbaTopLeftOrigin_[0],
            ycocg_,
            buffers_[i]);
    }

    // encode textures for output stream
    std::array<void*, 2> bufferPtrs;              // for hap_encode
    std::array<unsigned long, 2> buffersBytes;    // for hap_encode
    out.buffer.resize(getMaxEncodedSize());
    unsigned long outputBufferBytesUsed;
    for (unsigned int i = 0; i < count_; ++i)
    {
        bufferPtrs[i] = const_cast<uint8_t *>(&(buffers_[i][0]));
        buffersBytes[i] = (unsigned long)buffers_[i].size();
    }

    auto result = HapEncode(
        count_,
        const_cast<const void **>(&bufferPtrs[0]), const_cast<unsigned long *>(&buffersBytes[0]),
        const_cast<unsigned int *>(&textureFormats_[0]),
        const_cast<unsigned int *>(&compressors_[0]),
        const_cast<unsigned int *>(&chunkCounts_[0]),
        &out.buffer[0], (unsigned long)out.buffer.size(),
        &outputBufferBytesUsed);

    if (HapResult_No_Error != result)
    {
        throw std::runtime_error("failed to encode frame");
    }

    out.buffer.resize(outputBufferBytesUsed);
}

size_t HapEncoderJob::getMaxEncodedSize() const
{
    return HapMaxEncodedLength(count_, const_cast<unsigned long*>(&sizes_[0]), const_cast<unsigned int*>(&textureFormats_[0]), const_cast<unsigned int*>(&chunkCounts_[0]));
}
