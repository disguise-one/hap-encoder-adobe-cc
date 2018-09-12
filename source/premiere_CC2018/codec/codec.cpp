#include <stdexcept>
#include <string>
#include <tmmintrin.h>

#include "hap.h"

#include "codec.hpp"

int roundUpToMultipleOf4(int n)
{
    return (n + 3) & ~3;
}

Codec::Codec(
    CodecSubType subType,
    const FrameDef& frameDef,
    HapChunkCounts chunkCounts,
    const std::vector<unsigned int>& textureFormats)
    : subType_(subType),
      frameDef_(frameDef),
      count_((int)textureFormats.size()),
      chunkCounts_(chunkCounts),
      compressors_{ HapCompressorSnappy, HapCompressorSnappy }
{
    for (size_t i = 0; i < count_; ++i)
    {
        textureFormats_[i] = textureFormats[i];
        converters_[i] = TextureConverter::create(frameDef, textureFormats[i]);
        sizes_[i] = (unsigned long)converters_[i]->size();
    }
}


Codec::~Codec()
{
}


std::unique_ptr<Codec> Codec::create(CodecSubType codecType, const FrameDef& frameDef, HapChunkCounts chunkCounts)
{
    std::vector<unsigned int> textureFormats;

    // auto represented as 0, 0
    if (chunkCounts == HapChunkCounts{0, 0})
        chunkCounts = HapChunkCounts{ 1, 1 };

    if (codecType == kHapCodecSubType) {
        textureFormats = { HapTextureFormat_RGB_DXT1 };
    }
    else if (codecType == kHapAlphaCodecSubType) {
        textureFormats = { HapTextureFormat_RGBA_DXT5 };
    }
    else if (codecType == kHapYCoCgCodecSubType) {
        textureFormats = { HapTextureFormat_YCoCg_DXT5 };
    }
    else if (codecType == kHapYCoCgACodecSubType) {
        textureFormats = { HapTextureFormat_YCoCg_DXT5, HapTextureFormat_A_RGTC1 };
    }
    else if (codecType == kHapAOnlyCodecSubType) {
        textureFormats = { HapTextureFormat_A_RGTC1 };
    }
    else
        throw std::runtime_error("unknown codec");

    return std::make_unique<Codec>(codecType, frameDef, chunkCounts, textureFormats);
}

std::string Codec::getSubTypeAsString() const
{
    return std::string(subType_.begin(), subType_.end());
}

size_t Codec::getMaxEncodedSize() const
{
    return HapMaxEncodedLength(count_, const_cast<unsigned long *>(&sizes_[0]), const_cast<unsigned int *>(&textureFormats_[0]), const_cast<unsigned int*>(&chunkCounts_[0]));
}


static void swizzleBGRA2RGBAFlipAndUnstride(const uint8_t* in, size_t stride, size_t width, size_t height, uint8_t* out)
{
    // TODO: needs optimization

    size_t widthBytes = width * 4;
    for (size_t row = 0; row < height; row++)
    {
        const uint8_t *sourceRow = in + row * stride;
        uint8_t *destRow = out + (height - row - 1) * widthBytes;

        for (size_t i = 0; i < widthBytes; i += 4) {
            destRow[i] = sourceRow[i + 2];
            destRow[i + 1] = sourceRow[i + 1];
            destRow[i + 2] = sourceRow[i];
            destRow[i + 3] = sourceRow[i + 3];
        }
    }
}

void Codec::copyExternalToLocal(
    const uint8_t *bgraBottomLeftOrigin, size_t stride,
    EncodeInput& in) const
{
    // convert adobe format (bgra bottom left) to (rgba top left) for squish
    in.rgbaTopLeftOrigin.resize(frameDef_.width * frameDef_.height * 4);

    swizzleBGRA2RGBAFlipAndUnstride(
        bgraBottomLeftOrigin, stride,
        frameDef_.width, frameDef_.height,
        &in.rgbaTopLeftOrigin[0]);
}

void Codec::encode(const EncodeInput& in, EncodeScratchpad& scratchpad, EncodeOutput& out) const
{
    // convert input texture from rgba to <subcodec defined> dxt [+ dxt]
    for (unsigned int i = 0; i < count_; ++i)
    {
        converters_[i]->convert(
            &in.rgbaTopLeftOrigin[0],
            scratchpad.ycocg,
            scratchpad.buffers[i]);
    }

    // encode textures for output stream
    std::array<void*, 2> bufferPtrs;              // for hap_encode
    std::array<unsigned long, 2> buffersBytes;    // for hap_encode
    out.buffer.resize(getMaxEncodedSize());
    unsigned long outputBufferBytesUsed;
    for (unsigned int i = 0; i < count_; ++i)
    {
        bufferPtrs[i] = &(scratchpad.buffers[i][0]);
        buffersBytes[i] = (unsigned long)scratchpad.buffers[i].size();
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
