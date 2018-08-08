#pragma once

#include <mutex>

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
#include <PrSDKThreadedWorkSuite.h>
#include "SDK_Segment_Utils.h"
#include "movie_writer/movie_writer.hpp"
#include "codec/codec.hpp"

struct ExporterEncodeBuffers
{
    EncodeInput input;
    EncodeScratchpad scratchpad;
    EncodeOutput output;
};

class Exporter
{
public:
    Exporter(
        std::unique_ptr<Codec> codec,
        std::unique_ptr<MovieWriter> writer,
        int64_t nFrames);
    ~Exporter();
    
    // thread safe to be called 'on frame rendered'
    void dispatch(int64_t iFrame, const uint8_t* bgra_bottom_left_origin_data, size_t stride) const;

private:
    std::unique_ptr<ExporterEncodeBuffers> getEncodeBuffers() const;
    void recycleEncodeBuffers(std::unique_ptr<ExporterEncodeBuffers> toRecycle) const;

    void writeInSequence(int64_t iFrame, std::unique_ptr<ExporterEncodeBuffers> buffers) const;

    std::unique_ptr<Codec> codec_;
    mutable std::unique_ptr<MovieWriter> writer_;
    int64_t nFrames_;


    mutable std::mutex freeListMutex_;
    mutable std::vector<std::unique_ptr<ExporterEncodeBuffers> > freeList_;

    mutable std::mutex writeQueueMutex_;
    mutable std::vector<std::pair<int64_t, std::unique_ptr<ExporterEncodeBuffers> > > writeQueue_;
    mutable int32_t nextFrameToWrite_; // protected with writeQueueMutex_ too
};

typedef struct ExportSettings
{
	ExportSettings();
	~ExportSettings();

	csSDK_int32 fileType;
    CodecSubType hapSubcodec;
    std::unique_ptr<Exporter> exporter;
    csSDK_int32 movCurrentFrame;
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
    PrSDKThreadedWorkSuite* threadedWorkSuite;
    PrSDKMemoryManagerSuite* memorySuite;
	PrSDKWindowSuite* windowSuite;
	prFieldType sourceFieldType;
} ExportSettings;

csSDK_int32 getPixelFormatSize(const PrFourCC subtype);
