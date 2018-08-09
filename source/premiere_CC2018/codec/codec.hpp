#pragma once

// base class for different kinds of Hap encoders

#include <array>
#include <memory>
#include <vector>

#include "texture_converter.hpp"

typedef std::array<char, 4> CodecSubType;

#define kHapCodecSubType CodecSubType({'H' , 'a', 'p', '1'})
#define kHapAlphaCodecSubType CodecSubType({'H', 'a', 'p', '5'})
#define kHapYCoCgCodecSubType CodecSubType({'H', 'a', 'p', 'Y'})
#define kHapYCoCgACodecSubType CodecSubType({'H', 'a', 'p', 'M'})
#define kHapAOnlyCodecSubType CodecSubType({'H', 'a', 'p', 'A'})

// Placeholders for inputs, processing and outputs for encode process

struct EncodeInput
{
	const uint8_t *bgraBottomLeftOrigin;
	size_t stride;
};

struct EncodeScratchpad
{
    // not all of these are used by all codecs
    std::vector<uint8_t> rgbaTopLeftOrigin;       // for squish
    std::vector<uint8_t> ycocg;                   // for ycog -> ycog_dxt
    std::array<std::vector<uint8_t>, 2> buffers;  // for hap_encode
};

struct EncodeOutput
{
	std::vector<uint8_t> buffer;
};

// Instantiate once per input frame definition
//

class Codec
{
public:
	Codec(
        CodecSubType subType,
		const FrameDef& frameDef,
		const std::vector<unsigned int>& textureFormats);
	~Codec();

	static std::unique_ptr<Codec> create(CodecSubType codecType, const FrameDef& frameDef);

    CodecSubType subType() const { return subType_; }
	std::string getSubTypeAsString() const;
	size_t getMaxEncodedSize() const;

    void copyExternalToLocal(
        const EncodeInput& in, EncodeScratchpad& scratchpad, EncodeOutput& out) const;
    void encode(EncodeScratchpad& scratchpad, EncodeOutput& out) const;

private:
    CodecSubType subType_;
	FrameDef frameDef_;
	unsigned int count_;
	std::array<unsigned long, 2> sizes_;
	std::array<unsigned int, 2> chunkCounts_;
	std::array<unsigned int, 2> textureFormats_;
	std::array<unsigned int, 2> compressors_;
	std::array<std::unique_ptr<TextureConverter>, 2> converters_;
};
