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

typedef struct ExportSettings
{
	ExportSettings();
	~ExportSettings();

	csSDK_int32 fileType;
	std::unique_ptr<MovieWriter> movieWriter;
	CodecSubType hapSubcodec;
	std::unique_ptr<Codec> codec;
	EncodeInput encodeInput;
    EncodeScratchpad encodeScratchpad;
	EncodeOutput encodeOutput;
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
