#include "main.hpp"
#include "configure.hpp"
#include "premiereParams.hpp"
#include "prstring.hpp"
#include "export_settings.hpp"
#include "exporter/exporter.hpp"
#include "configure.hpp"
#include <vector>

// utf16 to utf8
#include <AtlBase.h>
#include <AtlConv.h>

DllExport PREMPLUGENTRY xSDKExport(csSDK_int32 selector, exportStdParms* stdParmsP, void* param1, void* param2)
{
	prMALError result = exportReturn_Unsupported;

	switch (selector)
	{
	case exSelStartup:
		result = startup(stdParmsP, reinterpret_cast<exExporterInfoRec*>(param1));
		break;

	case exSelBeginInstance:
		result = beginInstance(stdParmsP, reinterpret_cast<exExporterInstanceRec*>(param1));
		break;

	case exSelEndInstance:
		result = endInstance(stdParmsP, reinterpret_cast<exExporterInstanceRec*>(param1));
		break;

	case exSelGenerateDefaultParams:
		result = generateDefaultParams(stdParmsP, reinterpret_cast<exGenerateDefaultParamRec*>(param1));
		break;

	case exSelPostProcessParams:
		result = postProcessParams(stdParmsP, reinterpret_cast<exPostProcessParamsRec*>(param1));
		break;

	case exSelGetParamSummary:
		result = getParamSummary(stdParmsP, reinterpret_cast<exParamSummaryRec*>(param1));
		break;

	case exSelQueryOutputSettings:
		result = queryOutputSettings(stdParmsP, reinterpret_cast<exQueryOutputSettingsRec*>(param1));
		break;

	case exSelQueryExportFileExtension:
		result = fileExtension(stdParmsP, reinterpret_cast<exQueryExportFileExtensionRec*>(param1));
		break;

	case exSelParamButton:
		result = paramButton(stdParmsP, reinterpret_cast<exParamButtonRec*>(param1));
		break;

	case exSelValidateParamChanged:
		result = validateParamChanged(stdParmsP, reinterpret_cast<exParamChangedRec*>(param1));
		break;

	case exSelValidateOutputSettings:
		result = malNoError;
		break;

	case exSelExport:
		result = doExport(stdParmsP, reinterpret_cast<exDoExportRec*>(param1));
		break;

	default:
		break;
	}
	return result;
}

prMALError startup(exportStdParms* stdParms, exExporterInfoRec* infoRec)
{
    if (infoRec->exportReqIndex == 0)
    {
        infoRec->classID = HAP_VIDEOCLSS;
        infoRec->fileType = HAP_VIDEOFILETYPE;
        infoRec->hideInUI = kPrFalse;
        infoRec->isCacheable = kPrFalse;
        infoRec->exportReqIndex = 0;
        infoRec->canExportVideo = kPrTrue;
        infoRec->canExportAudio = kPrFalse;
        infoRec->canEmbedCaptions = kPrFalse;
        infoRec->singleFrameOnly = kPrFalse;
        infoRec->wantsNoProgressBar = kPrFalse;
        infoRec->doesNotSupportAudioOnly = kPrTrue;
        infoRec->interfaceVersion = EXPORTMOD_VERSION;

        copyConvertStringLiteralIntoUTF16(HAP_VIDEO_FILE_NAME, infoRec->fileTypeName);
        copyConvertStringLiteralIntoUTF16(HAP_VIDEOFILEEXT, infoRec->fileTypeDefaultExtension);
        return exportReturn_IterateExporter;
    }

    return exportReturn_IterateExporterDone;
}

prMALError beginInstance(exportStdParms* stdParmsP, exExporterInstanceRec* instanceRecP)
{
	SPErr spError = kSPNoError;
	PrSDKMemoryManagerSuite* memorySuite;
	SPBasicSuite* spBasic = stdParmsP->getSPBasicSuite();

	if (spBasic == nullptr)
		return exportReturn_ErrMemory;

	spError = spBasic->AcquireSuite(kPrSDKMemoryManagerSuite, kPrSDKMemoryManagerSuiteVersion, const_cast<const void**>(reinterpret_cast<void**>(&memorySuite)));
	void *settingsMem = (memorySuite->NewPtrClear(sizeof(ExportSettings)));

	if (settingsMem == nullptr)
		return exportReturn_ErrMemory;

	ExportSettings* settings = new(settingsMem) ExportSettings();

	settings->fileType = instanceRecP->fileType;
	settings->spBasic = spBasic;
	settings->memorySuite = memorySuite;
    spError = spBasic->AcquireSuite(kPrSDKExporterUtilitySuite, kPrSDKExporterUtilitySuiteVersion, const_cast<const void**>(reinterpret_cast<void**>(&(settings->exporterUtilitySuite))));
    spError = spBasic->AcquireSuite(kPrSDKExportParamSuite, kPrSDKExportParamSuiteVersion, const_cast<const void**>(reinterpret_cast<void**>(&(settings->exportParamSuite))));
    spError = spBasic->AcquireSuite(kPrSDKExportProgressSuite, kPrSDKExportProgressSuiteVersion, const_cast<const void**>(reinterpret_cast<void**>(&(settings->exportProgressSuite))));
	spError = spBasic->AcquireSuite(kPrSDKExportFileSuite, kPrSDKExportFileSuiteVersion, const_cast<const void**>(reinterpret_cast<void**>(&(settings->exportFileSuite))));
	spError = spBasic->AcquireSuite(kPrSDKExportInfoSuite, kPrSDKExportInfoSuiteVersion, const_cast<const void**>(reinterpret_cast<void**>(&(settings->exportInfoSuite))));
	spError = spBasic->AcquireSuite(kPrSDKErrorSuite, kPrSDKErrorSuiteVersion3, const_cast<const void**>(reinterpret_cast<void**>(&(settings->errorSuite))));
	spError = spBasic->AcquireSuite(kPrSDKClipRenderSuite, kPrSDKClipRenderSuiteVersion, const_cast<const void**>(reinterpret_cast<void**>(&(settings->clipRenderSuite))));
	spError = spBasic->AcquireSuite(kPrSDKMarkerSuite, kPrSDKMarkerSuiteVersion, const_cast<const void**>(reinterpret_cast<void**>(&(settings->markerSuite))));
	spError = spBasic->AcquireSuite(kPrSDKPPixSuite, kPrSDKPPixSuiteVersion, const_cast<const void**>(reinterpret_cast<void**>(&(settings->ppixSuite))));
	spError = spBasic->AcquireSuite(kPrSDKTimeSuite, kPrSDKTimeSuiteVersion, const_cast<const void**>(reinterpret_cast<void**>(&(settings->timeSuite))));
    spError = spBasic->AcquireSuite(kPrSDKWindowSuite, kPrSDKWindowSuiteVersion, const_cast<const void**>(reinterpret_cast<void**>(&(settings->windowSuite))));

	instanceRecP->privateData = reinterpret_cast<void*>(settings);

	return malNoError;
}

prMALError endInstance(exportStdParms* stdParmsP, exExporterInstanceRec* instanceRecP)
{
	prMALError result = malNoError;
	ExportSettings* settings = reinterpret_cast<ExportSettings*>(instanceRecP->privateData);
	SPBasicSuite* spBasic = stdParmsP->getSPBasicSuite();

	if (spBasic == nullptr || settings == nullptr)
		return malNoError;

    if (settings->exporterUtilitySuite)
        result = spBasic->ReleaseSuite(kPrSDKExporterUtilitySuite, kPrSDKExporterUtilitySuiteVersion);

    if (settings->exportParamSuite)
        result = spBasic->ReleaseSuite(kPrSDKExportParamSuite, kPrSDKExportParamSuiteVersion);

    if (settings->exportProgressSuite)
		result = spBasic->ReleaseSuite(kPrSDKExportProgressSuite, kPrSDKExportProgressSuiteVersion);

	if (settings->exportFileSuite)
		result = spBasic->ReleaseSuite(kPrSDKExportFileSuite, kPrSDKExportFileSuiteVersion);

	if (settings->exportInfoSuite)
		result = spBasic->ReleaseSuite(kPrSDKExportInfoSuite, kPrSDKExportInfoSuiteVersion);

	if (settings->errorSuite)
		result = spBasic->ReleaseSuite(kPrSDKErrorSuite, kPrSDKErrorSuiteVersion3);

	if (settings->clipRenderSuite)
		result = spBasic->ReleaseSuite(kPrSDKClipRenderSuite, kPrSDKClipRenderSuiteVersion);

	if (settings->markerSuite)
		result = spBasic->ReleaseSuite(kPrSDKMarkerSuite, kPrSDKMarkerSuiteVersion);

	if (settings->ppixSuite)
		result = spBasic->ReleaseSuite(kPrSDKPPixSuite, kPrSDKPPixSuiteVersion);

	if (settings->timeSuite)
		result = spBasic->ReleaseSuite(kPrSDKTimeSuite, kPrSDKTimeSuiteVersion);

	if (settings->windowSuite)
		result = spBasic->ReleaseSuite(kPrSDKWindowSuite, kPrSDKWindowSuiteVersion);

	settings->~ExportSettings();

	if (settings->memorySuite)
	{
		PrSDKMemoryManagerSuite* memorySuite = settings->memorySuite;
		memorySuite->PrDisposePtr(reinterpret_cast<PrMemoryPtr>(settings));
		result = spBasic->ReleaseSuite(kPrSDKMemoryManagerSuite, kPrSDKMemoryManagerSuiteVersion);
	}

	return result;
}

prMALError queryOutputSettings(exportStdParms *stdParmsP, exQueryOutputSettingsRec *outputSettingsP)
{
	const csSDK_uint32 exID = outputSettingsP->exporterPluginID;
	exParamValues width, height, frameRate, hapSubcodec, pixelAspectRatio, fieldType;
	ExportSettings* privateData = reinterpret_cast<ExportSettings*>(outputSettingsP->privateData);
	PrSDKExportParamSuite* paramSuite = privateData->exportParamSuite;
	const csSDK_int32 mgroupIndex = 0;
	float fps = 0.0f;

	if (outputSettingsP->inExportVideo)
	{
		paramSuite->GetParamValue(exID, mgroupIndex, ADBEVideoWidth, &width);
		outputSettingsP->outVideoWidth = width.value.intValue;
		paramSuite->GetParamValue(exID, mgroupIndex, ADBEVideoHeight, &height);
		outputSettingsP->outVideoHeight = height.value.intValue;
		paramSuite->GetParamValue(exID, mgroupIndex, ADBEVideoFPS, &frameRate);
		outputSettingsP->outVideoFrameRate = frameRate.value.timeValue;
		paramSuite->GetParamValue(exID, mgroupIndex, ADBEVideoCodec, &hapSubcodec);
		privateData->hapSubcodec = reinterpret_cast<CodecSubType &>(hapSubcodec.value.intValue);
		paramSuite->GetParamValue(exID, mgroupIndex, ADBEVideoAspect, &pixelAspectRatio);
		outputSettingsP->outVideoAspectNum = pixelAspectRatio.value.ratioValue.numerator;
		outputSettingsP->outVideoAspectDen = pixelAspectRatio.value.ratioValue.denominator;
		paramSuite->GetParamValue(exID, mgroupIndex, ADBEVideoFieldType, &fieldType);
		outputSettingsP->outVideoFieldType = fieldType.value.intValue;
	}

	// Calculate bitrate
	PrTime ticksPerSecond = 0;
	csSDK_uint32 videoBitrate = 0;

	if (outputSettingsP->inExportVideo)
	{
		privateData->timeSuite->GetTicksPerSecond(&ticksPerSecond);
		fps = static_cast<float>(ticksPerSecond) / frameRate.value.timeValue;
		paramSuite->GetParamValue(exID, mgroupIndex, "HAPSubcodec", &hapSubcodec);
		videoBitrate = static_cast<csSDK_uint32>(width.value.intValue * height.value.intValue * getPixelFormatSize(hapSubcodec.value.intValue) * fps);
	}

	outputSettingsP->outBitratePerSecond = videoBitrate * 8 / 1000;

	return malNoError;
}

prMALError fileExtension(exportStdParms* stdParmsP, exQueryExportFileExtensionRec* exportFileExtensionRecP)
{
	copyConvertStringLiteralIntoUTF16(HAP_VIDEOFILEEXT, exportFileExtensionRecP->outFileExtension);

	return malNoError;
}

prMALError renderAndWriteAllVideo(exDoExportRec* exportInfoP)
{
	prMALError result = malNoError;
	const csSDK_uint32 exID = exportInfoP->exporterPluginID;
	ExportSettings* settings = reinterpret_cast<ExportSettings*>(exportInfoP->privateData);
	exParamValues ticksPerFrame, width, height, hapSubcodec, pixelAspectRatio;
	PrTime ticksPerSecond;

	settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoFPS, &ticksPerFrame);
	settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoWidth, &width);
	settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoHeight, &height);
	settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoCodec, &hapSubcodec);
	settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoAspect, &pixelAspectRatio);
	settings->timeSuite->GetTicksPerSecond(&ticksPerSecond);
    const int64_t frameRateNumerator = ticksPerSecond;
    const int64_t frameRateDenominator = ticksPerFrame.value.timeValue;

	std::unique_ptr<Codec> codec = std::unique_ptr<Codec>(
        Codec::create(reinterpret_cast<CodecSubType&>(hapSubcodec.value.intValue),
					  FrameDef(width.value.intValue, height.value.intValue)));

	// get the filename
	csSDK_int32 outPathLength=1024;
	prUTF16Char outPlatformPath[1024];
	
	prMALError error = settings->exportFileSuite->GetPlatformPath(exportInfoP->fileObject, &outPathLength, outPlatformPath);
	if (malNoError != error)
		return error;

	CW2A utf8(outPlatformPath, CP_UTF8);
	const char* filename = utf8.m_psz;

    FILE *fp;
    if (0 != fopen_s(&fp, filename, "wb"))
        throw std::runtime_error(std::string("couldn't open ") + filename);

    std::unique_ptr<MovieWriter> movieWriter = std::make_unique<MovieWriter>(
        codec->subType(),
        width.value.intValue, height.value.intValue,
        frameRateNumerator, frameRateDenominator,
        [&](const uint8_t* buffer, size_t size) { return fwrite(buffer, 1, size, fp);  },
        [&](int64_t offset, int whence) { return _fseeki64(fp, offset, whence); },
        [&]() { fclose(fp);  },
        [&](const char *msg) { /* !!! log it */ });

    int64_t nFrames = (exportInfoP->endTime - exportInfoP->startTime) / ticksPerFrame.value.timeValue;
    settings->exporter = std::make_unique<Exporter>(std::move(codec), std::move(movieWriter), nFrames);

    ExportLoopRenderParams renderParams;
    renderParams.inRenderParamsSize = sizeof(ExportLoopRenderParams);
    renderParams.inRenderParamsVersion = kPrSDKExporterUtilitySuiteVersion;
    renderParams.inFinalPixelFormat = PrPixelFormat_BGRA_4444_8u;
    renderParams.inStartTime = exportInfoP->startTime;
    renderParams.inEndTime = exportInfoP->endTime;
    renderParams.inReservedProgressPreRender = 0.0; //!!!
    renderParams.inReservedProgressPostRender = 0.0; //!!!

    auto onFrameComplete = [](
        csSDK_uint32 inWhichPass,
        csSDK_uint32 inFrameNumber,
        csSDK_uint32 inFrameRepeatCount,
        PPixHand inRenderedFrame,
        void* inCallbackData)
    {
        try
        {
            char* bgra_buffer;
            int32_t bgra_stride;
            ExportSettings* settings = reinterpret_cast<ExportSettings*>(inCallbackData);
            settings->ppixSuite->GetPixels(inRenderedFrame, PrPPixBufferAccess_ReadOnly, &bgra_buffer);
            settings->ppixSuite->GetRowBytes(inRenderedFrame, &bgra_stride);

            settings->exporter->dispatch(inFrameNumber, (uint8_t*)bgra_buffer, bgra_stride);
        }
        catch (...)
        {
            return malUnknownError;
        }
        return malNoError;
    };

    settings->exporterUtilitySuite->DoMultiPassExportLoop(
        exportInfoP->exporterPluginID,
        &renderParams,
        1,  // number of passes
        onFrameComplete,
        settings
    );

    settings->exporter.reset(nullptr);

	return result;
}

prMALError doExport(exportStdParms* stdParmsP, exDoExportRec* exportInfoP)
{
	PrTime exportDuration = exportInfoP->endTime - exportInfoP->startTime;
	const csSDK_uint32 exID = exportInfoP->exporterPluginID;

	if (exportInfoP->exportVideo)
		return renderAndWriteAllVideo(exportInfoP);

    return 	malNoError;
}
