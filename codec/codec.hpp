#pragma once

// base class for different kinds of Hap encoders

#include <array>
#include <memory>
#include <vector>

#include "codec_registration.hpp"

#include "texture_converter.hpp"

// Placeholders for inputs, processing and outputs for encode process

class HapEncoderJob : public EncoderJob
{
public:
    HapEncoderJob(
        const FrameDef& frameDef,
        unsigned int count,
        HapChunkCounts chunkCounts,
        std::array<unsigned int, 2> textureFormats,
        std::array<unsigned int, 2> compressors,
        std::array<TextureConverter*, 2> converters,
        std::array<unsigned long, 2> sizes
        );
    ~HapEncoderJob() {}

private:
    virtual void doCopyExternalToLocal(
        const uint8_t* data,
        size_t stride,
        FrameFormat format) override;
    virtual void doEncode(EncodeOutput& out) override;

    size_t getMaxEncodedSize() const;

    FrameDef frameDef_;
    unsigned int count_;
    HapChunkCounts chunkCounts_;
    std::array<unsigned int, 2> textureFormats_;
    std::array<unsigned int, 2> compressors_;
    std::array<TextureConverter*, 2> converters_;
    std::array<unsigned long, 2> sizes_;

    std::vector<uint8_t> rgbaTopLeftOrigin_;       // for squish

    // not all of these are used by all codecs
    std::vector<uint8_t> ycocg_;                   // for ycog -> ycog_dxt
    std::array<std::vector<uint8_t>, 2> buffers_;  // for hap_encode
};

// Instantiate once per input frame definition
//

class HapEncoder : public Encoder
{
public:
    HapEncoder(std::unique_ptr<EncoderParametersBase>& params);
	~HapEncoder();

    virtual std::unique_ptr<EncoderJob> create() override;

private:
    static std::array<unsigned int, 2> getTextureFormats(Codec4CC subType);

	unsigned int count_;
	HapChunkCounts chunkCounts_;
	std::array<unsigned int, 2> textureFormats_;
	std::array<unsigned int, 2> compressors_;
	std::array<std::unique_ptr<TextureConverter>, 2> converters_;
    std::array<unsigned long, 2> sizes_;
};
