#pragma once

// base class for different kinds of Hap encoders

#include <array>
#include <memory>
#include <vector>

#include "codec_registration.hpp"

enum SquishEncoderQuality {
    kSquishEncoderFastQuality = 0,
    kSquishEncoderNormalQuality = 1,
    kSquishEncoderBestQuality = 2
};

// texture conversion from adobe-preferred to hap_encode required
// these converters all use squish as the final stage
class TextureConverter
{
public:
	TextureConverter(const FrameSize& frameSize)
		: frameSize_(frameSize)
	{}
	virtual ~TextureConverter();

	static std::unique_ptr<TextureConverter> create(const FrameSize& frameSize, unsigned int destFormat, SquishEncoderQuality quality);

	const FrameSize& frameSize() const { return frameSize_; }

    virtual size_t size() const;   // storage required

	void convert(const uint8_t* in_rgba,
		std::vector<uint8_t> &intermediate_ycocg,
		std::vector<uint8_t> &outputBuffer);

private:
	virtual void doConvert(
		const uint8_t* in_rgba,
		std::vector<uint8_t> &intermediate_ycocg,
		std::vector<uint8_t> &outputBuffer)=0;

	FrameSize frameSize_;
};
