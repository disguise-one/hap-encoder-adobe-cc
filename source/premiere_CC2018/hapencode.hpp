#pragma once

#include <PrSDKTypes.h>
#include <PrSDKWindowSuite.h>
#include <PrSDKSequenceRenderSuite.h>
#include <PrSDKSequenceAudioSuite.h>
#include <PrSDKMemoryManagerSuite.h>
#include <PrSDKPPixSuite.h>
#include <PrSDKMarkerSuite.h>
#include <PrSDKClipRenderSuite.h>
#include <PrSDKErrorSuite.h>
#include <PrSDKExportFileSuite.h>
#include <PrSDKExportInfoSuite.h>
#include <PrSDKExportProgressSuite.h>
#include <PrSDKExportParamSuite.h>
#include "SDK_Segment_Utils.h"
#include "movie_writer/movie_writer.hpp"
#include "codec/codec.hpp"

class Exporter
{
public:
    Exporter(
        std::unique_ptr<Codec> codec,
        std::unique_ptr<MovieWriter> writer,
        int64_t nFrames);
    ~Exporter();
    
    void dispatch(int64_t iFrame, const uint8_t* bgra_bottom_left_origin_data, size_t stride) const;

private:
    std::unique_ptr<Codec> codec_;
    std::unique_ptr<MovieWriter> writer_;
    int64_t nFrames_;

    // TODO: these expect dispatch to be called in strict sequence; need to make them pooled for
    //       multiple simultaneous calls
    mutable EncodeInput input_;
    mutable EncodeScratchpad scratchpad_;
    mutable EncodeOutput output_;
};

typedef struct ExportSettings
{
	ExportSettings();
	~ExportSettings();

	csSDK_int32 fileType;
    CodecSubType hapSubcodec;
    std::unique_ptr<Exporter> exporter;
    csSDK_int32 movCurrentFrame;
    VideoSequenceParser* videoSequenceParser;
	SPBasicSuite* spBasic;
	PrSDKExportParamSuite* exportParamSuite;
	PrSDKExportProgressSuite* exportProgressSuite;
	PrSDKExportInfoSuite* exportInfoSuite;
	PrSDKExportFileSuite* exportFileSuite;
	PrSDKErrorSuite3* errorSuite;
	PrSDKClipRenderSuite* clipRenderSuite;
	PrSDKMarkerSuite* markerSuite;
	PrSDKPPixSuite* ppixSuite;
	PrSDKTimeSuite* timeSuite;
	PrSDKMemoryManagerSuite* memorySuite;
	PrSDKSequenceAudioSuite1* sequenceAudioSuite;
	PrSDKSequenceRenderSuite* sequenceRenderSuite;
	PrSDKWindowSuite* windowSuite;
	csSDK_uint32 videoRenderID;
	prFieldType sourceFieldType;
} ExportSettings;

csSDK_int32 getPixelFormatSize(const PrFourCC subtype);
prMALError renderAndWriteVideoFrame(const PrTime videoTime, exDoExportRec* exportInfoP, csSDK_uint32& bytesWritten);
