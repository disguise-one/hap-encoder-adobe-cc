#pragma once

#include <stdint.h>

#include "codec_registration.hpp"

void convertHostFrameTo_RGBA_Top_Left_U8(const uint8_t* data, size_t stride, const FrameDef& frameDef, uint8_t* dest);
void convertRGBA_Top_Left_U8_ToHostFrame(const uint8_t* source, uint8_t* data, size_t stride, const FrameDef& frameDef);
void convertHostFrameTo_RGBA_Top_Left_U16(const uint8_t* data, size_t stride, const FrameDef& frameDef, uint16_t* dest);
void convertRGBA_Top_Left_U16_ToHostFrame(const uint16_t* source, uint8_t* data, size_t stride, const FrameDef& frameDef);
