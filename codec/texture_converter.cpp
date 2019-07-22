#include <stdexcept>

#include "texture_converter.hpp"
#include "hap.h"
#include "squish.h"

extern "C" {
#include "YCoCg.h"
}
#include "YCoCgDXT.h"

static int roundUpToMultipleOf4(int n)
{
	return (n + 3) & ~3;
}

// texture conversion from adobe-preferred to hap_encode required
// these converters all use squish as the final stage
class SquishTextureConverter : public TextureConverter
{
public:
	SquishTextureConverter(const FrameDef& frameDef, int squishFlags)
		: TextureConverter(frameDef), squishFlags_(squishFlags)
	{}
	virtual ~SquishTextureConverter() {};

private:
    size_t size() const override
    {
        return squish::GetStorageRequirements(frameDef().width, frameDef().height, squishFlags_);
    }

	virtual void doConvert(
		const uint8_t* in_rgba,
		std::vector<uint8_t> &ycocg,
		std::vector<uint8_t> &outputBuffer) override
	{
		outputBuffer.resize(size());

		void *blocks = &(outputBuffer[0]);
		float *metric = nullptr;
		squish::CompressImage(in_rgba, frameDef().width, frameDef().height, blocks, squishFlags_, metric);
	}

	int squishFlags_;
};


class TextureConverterToYCoCg_Dxt5 : public TextureConverter
{
public:
	TextureConverterToYCoCg_Dxt5(const FrameDef& frameDef) : TextureConverter(frameDef) {}
	~TextureConverterToYCoCg_Dxt5() {}

    size_t size() const override
    {
        return roundUpToMultipleOf4(frameDef().width) * roundUpToMultipleOf4(frameDef().height);
    }

    void doConvert(
		const uint8_t* in_rgba,
		std::vector<uint8_t> &ycocg,
		std::vector<uint8_t> &outputBuffer) override
	{
		int rowbytes = frameDef().width * 4;
		ycocg.resize(rowbytes * frameDef().height);

		ConvertRGB_ToCoCg_Y8888(
			in_rgba,                // const uint8_t *src,
			&ycocg[0],              // uint8_t *dst
			frameDef().width,       // unsigned long width,
			frameDef().height,      // unsigned long height,
			rowbytes,               // size_t src_rowbytes
			rowbytes,               // size_t dst_rowbytes,
			false                   // int allow_tile
		);

		outputBuffer.resize(size());

		CompressYCoCgDXT5(
			&(ycocg[0]),
			&(outputBuffer[0]),
			frameDef().width, frameDef().height,
			frameDef().width * 4  // stride
		);
	}
};


TextureConverter::~TextureConverter()
{
}

size_t TextureConverter::size() const
{
    throw std::runtime_error("unknown texture conversion");
}


std::unique_ptr<TextureConverter> TextureConverter::create(const FrameDef& frameDef, unsigned int destFormat, SquishEncoderQuality quality)
{
	int flag_quality;
	switch (quality)
	{
	case kSquishEncoderFastQuality:
		flag_quality = squish::kColourRangeFit;
		break;
	case kSquishEncoderBestQuality:
		flag_quality = squish::kColourIterativeClusterFit;
		break;
	default:
		flag_quality = squish::kColourClusterFit;
		break;
	}

	switch (destFormat)
	{
	case HapTextureFormat_RGB_DXT1:
		return std::make_unique<SquishTextureConverter>(frameDef, squish::kDxt1 | flag_quality);
	case HapTextureFormat_RGBA_DXT5:
		return std::make_unique<SquishTextureConverter>(frameDef, squish::kDxt5 | flag_quality);
	case HapTextureFormat_YCoCg_DXT5:
		return std::make_unique<TextureConverterToYCoCg_Dxt5>(frameDef);
	case HapTextureFormat_A_RGTC1:
		return std::make_unique<SquishTextureConverter>(frameDef, squish::kRgtc1A);
	default:
		throw std::runtime_error("unknown conversion");
	}
}


void TextureConverter::convert(const uint8_t* in_rgba,
							   std::vector<uint8_t> &intermediate_ycocg,
							   std::vector<uint8_t> &outputBuffer)
{
	doConvert(in_rgba, intermediate_ycocg, outputBuffer);
}


void TextureConverter::doConvert(
	const uint8_t* in_rgba,
	std::vector<uint8_t> &intermediate_ycocg,
	std::vector<uint8_t> &outputBuffer)
{
}