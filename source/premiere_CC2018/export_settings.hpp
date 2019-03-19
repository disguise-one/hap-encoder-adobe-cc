#pragma once

#include "exporter/exporter.hpp"

#include <PrSDKTypes.h>
#include <PrSDKWindowSuite.h>
#include <PrSDKMemoryManagerSuite.h>
#include <PrSDKPPixSuite.h>
#include <PrSDKMarkerSuite.h>
#include <PrSDKClipRenderSuite.h>
#include <PrSDKAudioSuite.h>
#include <PrSDKSequenceAudioSuite.h>
#include <PrSDKErrorSuite.h>
#include <PrSDKExportFileSuite.h>
#include <PrSDKExportInfoSuite.h>
#include <PrSDKExportProgressSuite.h>
#include <PrSDKExportParamSuite.h>
#include <PrSDKExporterUtilitySuite.h>
#include "SDK_Segment_Utils.h"

class CodecRegistry;

typedef struct ExportSettings
{
	ExportSettings();
	~ExportSettings();

	csSDK_int32 fileType;
    std::unique_ptr<Exporter> exporter;
	SPBasicSuite* spBasic;
    std::function<void(const std::string& error)> reportError;
    std::function<void(const std::string& error)> logMessage;
    PrSDKExporterUtilitySuite* exporterUtilitySuite;
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
	PrSDKWindowSuite* windowSuite;
	PrSDKAudioSuite* audioSuite;
	PrSDKSequenceAudioSuite1* sequenceAudioSuite;
} ExportSettings;

csSDK_int32 getPixelFormatSize(const PrFourCC subtype);
