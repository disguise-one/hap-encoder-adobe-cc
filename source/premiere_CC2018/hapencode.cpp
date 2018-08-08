#include "hapencode.hpp"
#include "codec/codec.hpp"
#include "configure.hpp"

Exporter::Exporter(
    std::unique_ptr<Codec> codec,
    std::unique_ptr<MovieWriter> writer,
    int64_t nFrames)
  : codec_(std::move(codec)), writer_(std::move(writer)), nFrames_(nFrames)

{
}

Exporter::~Exporter()
{
}

// TODO: thread-safe
void Exporter::dispatch(int64_t iFrame, const uint8_t* bgra_bottom_left_origin_data, size_t stride) const
{
    // convert Premiere frame to HAP frame
    input_.bgraBottomLeftOrigin = (uint8_t *)bgra_bottom_left_origin_data;
    input_.stride = stride;

    codec_->encode(input_, scratchpad_, output_);

    // write it out
    writer_->writeFrame(&output_.buffer[0], output_.buffer.size());
}

csSDK_int32 getPixelFormatSize(const PrFourCC subtype)
{
	return 4;
}

prMALError renderAndWriteVideoFrame(const PrTime videoTime, exDoExportRec* exportInfoP, csSDK_uint32& bytesWritten)
{
	csSDK_int32 resultS = malNoError;
	const csSDK_uint32 exID = exportInfoP->exporterPluginID;
	ExportSettings* settings = reinterpret_cast<ExportSettings *>(exportInfoP->privateData);
	csSDK_int32 rowbytes = 0;
	exParamValues width, height, hapSubcodec, pixelAspectRatio, fieldType, compressLevel;
	PrPixelFormat renderedPixelFormat;
	char* frameBufferP = nullptr;
	uint8_t* toDiskBuffer = nullptr;
	SequenceRender_ParamsRec renderParms;
	PrPixelFormat pixelFormats[] = { PrPixelFormat_BGRA_4444_8u };

	settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoWidth, &width);
	settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoHeight, &height);
	settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoCodec, &hapSubcodec);
	settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoAspect, &pixelAspectRatio);
	settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoFieldType, &fieldType);
	settings->exportParamSuite->GetParamValue(exID, 0, ADBECompressionLevel, &compressLevel);

	renderParms.inRequestedPixelFormatArray = pixelFormats;
	renderParms.inRequestedPixelFormatArrayCount = 1;
	renderParms.inWidth = width.value.intValue;
	renderParms.inHeight = height.value.intValue;
	renderParms.inPixelAspectRatioNumerator = pixelAspectRatio.value.ratioValue.numerator;
	renderParms.inPixelAspectRatioDenominator = pixelAspectRatio.value.ratioValue.denominator;
	renderParms.inRenderQuality = kPrRenderQuality_Max;
	renderParms.inFieldType = fieldType.value.intValue;
	renderParms.inDeinterlace = kPrFalse;
	renderParms.inDeinterlaceQuality = kPrRenderQuality_Max;
	renderParms.inCompositeOnBlack = kPrFalse;

	SequenceRender_GetFrameReturnRec renderResult;
	resultS = settings->sequenceRenderSuite->RenderVideoFrame(settings->videoRenderID, videoTime, &renderParms, kRenderCacheType_None, &renderResult);
	settings->ppixSuite->GetPixels(renderResult.outFrame, PrPPixBufferAccess_ReadOnly, &frameBufferP);
	settings->ppixSuite->GetRowBytes(renderResult.outFrame, &rowbytes);
	settings->ppixSuite->GetPixelFormat(renderResult.outFrame, &renderedPixelFormat);

	if (resultS == suiteError_CompilerCompileAbort)
		return suiteError_CompilerCompileAbort;

	try {
		settings->exporter->dispatch(settings->movCurrentFrame++, (uint8_t *)frameBufferP, rowbytes);
	}
	catch (...)
	{
		return malUnknownError;
	}
	
	settings->ppixSuite->Dispose(renderResult.outFrame);

	return resultS;
}
