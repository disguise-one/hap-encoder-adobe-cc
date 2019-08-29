#include "util.hpp"

template<int SRC_FOR_DEST0, int SRC_FOR_DEST1, int SRC_FOR_DEST2, int SRC_FOR_DEST3,
    typename SRC_CHANNEL_TYPE, typename STRIDET,
    typename WIDTHT, typename HEIGHTT,
    typename DST_CHANNEL_TYPE, typename DST_STRIDET>
    void copy_noflip(const SRC_CHANNEL_TYPE *src, STRIDET stride, WIDTHT width, HEIGHTT height,
        DST_CHANNEL_TYPE *dest, DST_STRIDET destStride)
{
    // TODO: needs optimization
    size_t widthX4 = width * 4;
    for (size_t row = 0; row < height; row++)
    {
        const SRC_CHANNEL_TYPE *sourceRow = (const SRC_CHANNEL_TYPE *)((const uint8_t*)src + row * stride);
        DST_CHANNEL_TYPE *destRow = (DST_CHANNEL_TYPE *)((uint8_t*)dest + row * destStride);


        for (size_t i = 0; i < widthX4; i += 4) {
            destRow[i] = sourceRow[i + SRC_FOR_DEST0];
            destRow[i + 1] = sourceRow[i + SRC_FOR_DEST1];
            destRow[i + 2] = sourceRow[i + SRC_FOR_DEST2];
            destRow[i + 3] = sourceRow[i + SRC_FOR_DEST3];
        }
    }
}

template<int SRC_FOR_DEST0, int SRC_FOR_DEST1, int SRC_FOR_DEST2, int SRC_FOR_DEST3,
    typename SRC_CHANNEL_TYPE, typename STRIDET,
    typename WIDTHT, typename HEIGHTT,
    typename DST_CHANNEL_TYPE, typename DST_STRIDET>
    void copy_flip(const SRC_CHANNEL_TYPE *src, STRIDET stride, WIDTHT width, HEIGHTT height,
        DST_CHANNEL_TYPE *dest, DST_STRIDET destStride)
{
    // TODO: needs optimization
    size_t widthX4 = width * 4;
    for (size_t row = 0; row < height; row++)
    {
        const SRC_CHANNEL_TYPE *sourceRow = (const SRC_CHANNEL_TYPE *)((const uint8_t*)src + row * stride);
        DST_CHANNEL_TYPE *destRow = (DST_CHANNEL_TYPE *)((uint8_t*)dest + (height - row - 1) * destStride);


        for (size_t i = 0; i < widthX4; i += 4) {
            destRow[i] = sourceRow[i + SRC_FOR_DEST0];
            destRow[i + 1] = sourceRow[i + SRC_FOR_DEST1];
            destRow[i + 2] = sourceRow[i + SRC_FOR_DEST2];
            destRow[i + 3] = sourceRow[i + SRC_FOR_DEST3];
        }
    }
}


template<int SRC_FOR_DEST0, int SRC_FOR_DEST1, int SRC_FOR_DEST2, int SRC_FOR_DEST3,
    typename SRC_CHANNEL_TYPE, typename STRIDET,
    typename WIDTHT, typename HEIGHTT,
    typename DST_CHANNEL_TYPE,
    typename TO_DST_SCALET, typename DST_STRIDET>
    void copy_noflip_scaled(const SRC_CHANNEL_TYPE *src, STRIDET stride, WIDTHT width, HEIGHTT height,
        DST_CHANNEL_TYPE *dest, DST_STRIDET destStride, TO_DST_SCALET toDestScale)
{
    // TODO: needs optimization
    size_t widthX4 = width * 4;
    for (size_t row = 0; row < height; row++)
    {
        const SRC_CHANNEL_TYPE *sourceRow = (const SRC_CHANNEL_TYPE *)((const uint8_t*)src + row * stride);
        DST_CHANNEL_TYPE *destRow = (DST_CHANNEL_TYPE *)((uint8_t*)dest + row * destStride);


        for (size_t i = 0; i < widthX4; i += 4) {
            destRow[i]     = (DST_CHANNEL_TYPE)(sourceRow[i + SRC_FOR_DEST0] * toDestScale);
            destRow[i + 1] = (DST_CHANNEL_TYPE)(sourceRow[i + SRC_FOR_DEST1] * toDestScale);
            destRow[i + 2] = (DST_CHANNEL_TYPE)(sourceRow[i + SRC_FOR_DEST2] * toDestScale);
            destRow[i + 3] = (DST_CHANNEL_TYPE)(sourceRow[i + SRC_FOR_DEST3] * toDestScale);
        }
    }
}

template<int SRC_FOR_DEST0, int SRC_FOR_DEST1, int SRC_FOR_DEST2, int SRC_FOR_DEST3,
    typename SRC_CHANNEL_TYPE, typename STRIDET,
    typename WIDTHT, typename HEIGHTT,
    typename DST_CHANNEL_TYPE,
    typename TO_DST_SCALET, typename DST_STRIDET>
    void copy_flip_scaled(const SRC_CHANNEL_TYPE *src, STRIDET stride, WIDTHT width, HEIGHTT height,
        DST_CHANNEL_TYPE *dest, DST_STRIDET destStride, TO_DST_SCALET toDestScale)
{
    // TODO: needs optimization
    size_t widthX4 = width * 4;
    for (size_t row = 0; row < height; row++)
    {
        const SRC_CHANNEL_TYPE *sourceRow = (const SRC_CHANNEL_TYPE *)((const uint8_t*)src + row * stride);
        DST_CHANNEL_TYPE *destRow = (DST_CHANNEL_TYPE *)((uint8_t*)dest + (height - row - 1) * destStride);


        for (size_t i = 0; i < widthX4; i += 4) {
            destRow[i] =     (DST_CHANNEL_TYPE)(sourceRow[i + SRC_FOR_DEST0] * toDestScale);
            destRow[i + 1] = (DST_CHANNEL_TYPE)(sourceRow[i + SRC_FOR_DEST1] * toDestScale);
            destRow[i + 2] = (DST_CHANNEL_TYPE)(sourceRow[i + SRC_FOR_DEST2] * toDestScale);
            destRow[i + 3] = (DST_CHANNEL_TYPE)(sourceRow[i + SRC_FOR_DEST3] * toDestScale);
        }
    }
}


void convertHostFrameTo_RGBA_Top_Left_U16(const uint8_t *data, size_t stride, const FrameDef& frameDef, uint16_t *dest, size_t destStrideInBytes)
{
    switch (frameDef.format)
    {
        case ChannelLayout_BGRA | FrameOrigin_BottomLeft | ChannelFormat_U16_32k:
            copy_flip_scaled<2, 1, 0, 3>((uint16_t*)data, stride, frameDef.width, frameDef.height, dest, destStrideInBytes, 65535.0 / 32768.0);
            break;
        case ChannelLayout_ARGB | FrameOrigin_TopLeft | ChannelFormat_U16_32k:
            copy_noflip_scaled<1, 2, 3, 0>((uint16_t *)data, stride, frameDef.width, frameDef.height, dest, destStrideInBytes, 65535.0/32768.0);
            break;
        case ChannelLayout_BGRA | FrameOrigin_BottomLeft | ChannelFormat_F32:
            copy_flip_scaled<2, 1, 0, 3>((float *)data, stride, frameDef.width, frameDef.height, dest, destStrideInBytes, 65535.0);
            break;
        case ChannelLayout_ARGB | FrameOrigin_TopLeft | ChannelFormat_F32:
            copy_noflip_scaled<2, 1, 0, 3>((float *)data, stride, frameDef.width, frameDef.height, dest, destStrideInBytes, 65535.0);
            break;
        case ChannelLayout_BGRA | FrameOrigin_BottomLeft | ChannelFormat_U8:
            copy_flip_scaled<2, 1, 0, 3>((uint8_t *)data, stride, frameDef.width, frameDef.height, dest, destStrideInBytes, 256.0);
            break;
        case ChannelLayout_ARGB | FrameOrigin_TopLeft | ChannelFormat_U8:
            copy_noflip_scaled<1, 2, 3, 0>((uint8_t *)data, stride, frameDef.width, frameDef.height, dest, destStrideInBytes, 256.0);
            break;
        default:
            throw std::runtime_error("unhandled host format");
    }
}

void convertRGBA_Top_Left_U16_ToHostFrame(const uint16_t* source, uint8_t *data, size_t stride, const FrameDef& frameDef)
{
    switch (frameDef.format)
    {
    case ChannelLayout_BGRA | FrameOrigin_BottomLeft | ChannelFormat_U16_32k:
        copy_flip_scaled<2, 1, 0, 3>((uint16_t*)source, frameDef.width * 8, frameDef.width, frameDef.height, (uint16_t*)data, stride, 32768.0/65535.0);
        break;
    case ChannelLayout_BGRA | FrameOrigin_BottomLeft | ChannelFormat_F32:
        copy_flip_scaled<2, 1, 0, 3>((uint16_t *)source, frameDef.width * 8, frameDef.width, frameDef.height, (float*)data, stride, 1.0 / 65535.0);
        break;
    case ChannelLayout_BGRA | FrameOrigin_BottomLeft | ChannelFormat_U8:
        copy_flip_scaled<2, 1, 0, 3>((uint16_t *)source, frameDef.width * 8, frameDef.width, frameDef.height, (uint8_t*)data, stride, 1.0/256.0);
        break;
    default:
        throw std::runtime_error("unhandled host format");
    }
}


void convertHostFrameTo_RGBA_Top_Left_U8(const uint8_t* data, size_t stride, const FrameDef& frameDef, uint8_t* dest, size_t destStrideInBytes)
{
    switch (frameDef.format)
    {
    case ChannelLayout_BGRA | FrameOrigin_BottomLeft | ChannelFormat_U16_32k:
        copy_flip_scaled<2, 1, 0, 3>((uint16_t*)data, stride, frameDef.width, frameDef.height, dest, destStrideInBytes, 255.0 / 32768.0);
        break;
    case ChannelLayout_ARGB | FrameOrigin_TopLeft | ChannelFormat_U16_32k:
        copy_noflip_scaled<1, 2, 3, 0>((uint16_t*)data, stride, frameDef.width, frameDef.height, dest, destStrideInBytes, 255.0 / 32768.0);
        break;
    case ChannelLayout_BGRA | FrameOrigin_BottomLeft | ChannelFormat_F32:
        copy_flip_scaled<2, 1, 0, 3>((float*)data, stride, frameDef.width, frameDef.height, dest, destStrideInBytes, 255.0);
        break;
    case ChannelLayout_ARGB | FrameOrigin_TopLeft | ChannelFormat_F32:
        copy_noflip_scaled<2, 1, 0, 3>((float*)data, stride, frameDef.width, frameDef.height, dest, destStrideInBytes, 255.0);
        break;
    case ChannelLayout_BGRA | FrameOrigin_BottomLeft | ChannelFormat_U8:
        copy_flip_scaled<2, 1, 0, 3>((uint8_t*)data, stride, frameDef.width, frameDef.height, dest, destStrideInBytes, 1.0);
        break;
    case ChannelLayout_ARGB | FrameOrigin_TopLeft | ChannelFormat_U8:
        copy_noflip_scaled<1, 2, 3, 0>((uint8_t*)data, stride, frameDef.width, frameDef.height, dest, destStrideInBytes, 1.0);
        break;
    default:
        throw std::runtime_error("unhandled host format");
    }
}

void convertRGBA_Top_Left_U8_ToHostFrame(const uint8_t* source, uint8_t* data, size_t stride, const FrameDef& frameDef)
{
    switch (frameDef.format)
    {
    case ChannelLayout_BGRA | FrameOrigin_BottomLeft | ChannelFormat_U16_32k:
        copy_flip_scaled<2, 1, 0, 3>((uint16_t*)source, frameDef.width * 4, frameDef.width, frameDef.height, (uint16_t*)data, stride, 32768.0 / 255.0);
        break;
    case ChannelLayout_BGRA | FrameOrigin_BottomLeft | ChannelFormat_F32:
        copy_flip_scaled<2, 1, 0, 3>((uint16_t*)source, frameDef.width * 4, frameDef.width, frameDef.height, (float*)data, stride, 1.0 / 255.0);
        break;
    case ChannelLayout_BGRA | FrameOrigin_BottomLeft | ChannelFormat_U8:
        copy_flip_scaled<2, 1, 0, 3>((uint16_t*)source, frameDef.width * 4, frameDef.width, frameDef.height, (uint8_t*)data, stride, 1.0);
        break;
    default:
        throw std::runtime_error("unhandled host format");
    }
}
