#pragma once

#include "exporter/exporter.hpp"

#include <PrSDKTypes.h>
#include <PrSDKWindowSuite.h>
#include <PrSDKMemoryManagerSuite.h>
#include <PrSDKPPixSuite.h>
#include <PrSDKMarkerSuite.h>
#include <PrSDKClipRenderSuite.h>
#include <PrSDKErrorSuite.h>
#include <PrSDKExportFileSuite.h>
#include <PrSDKExportInfoSuite.h>
#include <PrSDKExportProgressSuite.h>
#include <PrSDKExportParamSuite.h>
#include <PrSDKExporterUtilitySuite.h>
#include "SDK_Segment_Utils.h"

typedef struct ExportSettings
{
	ExportSettings();
	~ExportSettings();

	csSDK_int32 fileType;
    CodecSubType hapSubcodec;
    std::unique_ptr<Exporter> exporter;
	SPBasicSuite* spBasic;
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
} ExportSettings;

csSDK_int32 getPixelFormatSize(const PrFourCC subtype);
