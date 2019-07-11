#include "configure.hpp"
#include "main.hpp"
#include "premiereParams.hpp"
#include "prstring.hpp"
#include "export_settings.hpp"
#include "exporter/exporter.hpp"
#include "configure.hpp"
#include <vector>
#include <locale>

csSDK_int32 GetNumberOfAudioChannels(csSDK_int32 audioChannelType);
static void renderAndWriteAllAudio(exDoExportRec *exportInfoP, prMALError &error, MovieWriter *writer);

// nuisance
static std::wstring to_wstring(const std::string& str)
{
    return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.from_bytes(str);
}


DllExport PREMPLUGENTRY xSDKExport(csSDK_int32 selector, exportStdParms* stdParmsP, void* param1, void* param2)
{
	prMALError result = exportReturn_Unsupported;

	switch (selector)
	{
	case exSelStartup:
		result = startup(stdParmsP, reinterpret_cast<exExporterInfoRec*>(param1));
		break;

    case exSelShutdown:
        CodecRegistry::codec().reset();
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
        // singleton needed from here on
        CodecRegistry::codec();

        std::string logName = CodecRegistry::codec()->logName();
        std::wstring logNameForPr(to_wstring(logName));


        infoRec->classID = HAP_VIDEOCLSS;
        infoRec->fileType = HAP_VIDEOFILETYPE;
        infoRec->hideInUI = kPrFalse;
        infoRec->isCacheable = kPrFalse;
        infoRec->exportReqIndex = 0;
        infoRec->canExportVideo = kPrTrue;
        infoRec->canExportAudio = kPrTrue;
        infoRec->canEmbedCaptions = kPrFalse;
        infoRec->canConformToMatchParams = kPrTrue;
        infoRec->singleFrameOnly = kPrFalse;
        infoRec->wantsNoProgressBar = kPrFalse;
        infoRec->doesNotSupportAudioOnly = kPrTrue;
        infoRec->interfaceVersion = EXPORTMOD_VERSION;
        copyConvertStringLiteralIntoUTF16(logNameForPr.c_str(), infoRec->fileTypeName);
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
    spError = spBasic->AcquireSuite(kPrSDKAudioSuite, kPrSDKAudioSuiteVersion, const_cast<const void**>(reinterpret_cast<void**>(&(settings->audioSuite))));
    spError = spBasic->AcquireSuite(kPrSDKSequenceAudioSuite, kPrSDKSequenceAudioSuiteVersion1, const_cast<const void**>(reinterpret_cast<void**>(&(settings->sequenceAudioSuite))));

    // convenience callback
    auto report = settings->exporterUtilitySuite->ReportEvent;
    auto pluginId = instanceRecP->exporterPluginID;
    settings->reportError = [report, pluginId](const std::string& error) {

        StringForPr title(to_wstring(CodecRegistry::logName() + " - ERROR"));
        StringForPr detail(to_wstring(error));

        report(
            pluginId, PrSDKErrorSuite2::kEventTypeError,
            title.get(),
            detail.get());
    };
    settings->logMessage = [report, pluginId](const std::string& message) {

        StringForPr title(to_wstring(CodecRegistry::logName()));
        StringForPr detail(to_wstring(message));

        report(
            pluginId, PrSDKErrorSuite2::kEventTypeError,
            title.get(),
            detail.get());
    };

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

    if (settings->audioSuite)
        result = spBasic->ReleaseSuite(kPrSDKAudioSuite, kPrSDKAudioSuiteVersion);

    if (settings->sequenceAudioSuite)
        result = spBasic->ReleaseSuite(kPrSDKSequenceAudioSuite, kPrSDKSequenceAudioSuiteVersion1);

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
    exParamValues width, height, frameRate;
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
        outputSettingsP->outVideoAspectNum = 1;
		outputSettingsP->outVideoAspectDen = 1;
		// paramSuite->GetParamValue(exID, mgroupIndex, ADBEVideoFieldType, &fieldType);
		outputSettingsP->outVideoFieldType = prFieldsNone;
	}

	// Calculate bitrate
	PrTime ticksPerSecond = 0;
	csSDK_uint32 videoBitrate = 0;

	if (outputSettingsP->inExportVideo)
	{
		privateData->timeSuite->GetTicksPerSecond(&ticksPerSecond);
		fps = static_cast<float>(ticksPerSecond) / frameRate.value.timeValue;
    }

    if (outputSettingsP->inExportAudio)
    {
        exParamValues sampleRate, channelType;
        paramSuite->GetParamValue(exID, mgroupIndex, ADBEAudioRatePerSecond, &sampleRate);
        paramSuite->GetParamValue(exID, mgroupIndex, ADBEAudioNumChannels, &channelType);
        
        outputSettingsP->outAudioChannelType = static_cast<PrAudioChannelType>(channelType.value.intValue);
        outputSettingsP->outAudioSampleRate = sampleRate.value.floatValue;
        outputSettingsP->outAudioSampleType = kPrAudioSampleType_16BitInt;
    }

	outputSettingsP->outBitratePerSecond = videoBitrate * 8 / 1000;

	return malNoError;
}

prMALError fileExtension(exportStdParms* stdParmsP, exQueryExportFileExtensionRec* exportFileExtensionRecP)
{
	copyConvertStringLiteralIntoUTF16(HAP_VIDEOFILEEXT, exportFileExtensionRecP->outFileExtension);

	return malNoError;
}

static prMALError c_onFrameComplete(
    csSDK_uint32 inWhichPass,
    csSDK_uint32 inFrameNumber,
    csSDK_uint32 inFrameRepeatCount,
    PPixHand inRenderedFrame,
    void* inCallbackData)
{
    ExportSettings* settings = reinterpret_cast<ExportSettings*>(inCallbackData);
    try
    {
        char* bgra_buffer;
        int32_t bgra_stride;
        prMALError error = settings->ppixSuite->GetPixels(inRenderedFrame, PrPPixBufferAccess_ReadOnly, &bgra_buffer);
        if (malNoError != error)
            throw std::runtime_error("could not GetPixels on completed frame");

        error = settings->ppixSuite->GetRowBytes(inRenderedFrame, &bgra_stride);
        if (malNoError != error)
            throw std::runtime_error("could not GetRowBytes on completed frame");

        for (auto iFrame=inFrameNumber; iFrame<inFrameNumber + inFrameRepeatCount; ++iFrame)
            settings->exporter->dispatch(iFrame, (uint8_t*)bgra_buffer, bgra_stride, 0);  //!!! could support multiple formats here
    }
    catch (const std::exception& ex)
    {
        settings->reportError(ex.what());
        return malUnknownError;
    }
    catch (...)
    {
        settings->reportError("unspecified error while processing frame");
        return malUnknownError;
    }
    return malNoError;
}

static MovieFile createMovieFile(PrSDKExportFileSuite* exportFileSuite, csSDK_int32 fileObject,
                                 MovieErrorCallback errorCallback)
{
    // cache some things
    auto Write = exportFileSuite->Write;
    auto Seek = exportFileSuite->Seek;
    auto Close = exportFileSuite->Close;

    MovieFile fileWrapper;
    fileWrapper.onOpenForWrite = [=]() {
        //--- this error flag may be overwritten fairly deeply in callbacks so original error may be
        //--- passed up to Adobe
        prMALError error = exportFileSuite->Open(fileObject);
        if (malNoError != error)
            throw std::runtime_error("couldn't open output file");
    };
    fileWrapper.onWrite = [=](const uint8_t* buffer, size_t size) {
        prMALError writeError = Write(fileObject, (void *)buffer, (int32_t)size);
        if (malNoError != writeError) {
            errorCallback("Could not write to file");
            return -1;
        }
        return 0;
    };
    fileWrapper.onSeek = [=](int64_t offset, int whence) {
        int64_t newPosition;
        ExFileSuite_SeekMode seekMode;
        if (whence == SEEK_SET)
            seekMode = fileSeekMode_Begin;
        else if (whence == SEEK_END)
            seekMode = fileSeekMode_End;
        else if (whence == SEEK_CUR)
            seekMode = fileSeekMode_Current;
        else
            throw std::runtime_error("unhandled file seek mode");
        prMALError seekError = Seek(fileObject, offset, newPosition, seekMode);
        if (malNoError != seekError) {
            errorCallback("Could not seek in file");
            return -1;
        }
        return 0;
    };
    fileWrapper.onClose = [=]() {
        return (malNoError == Close(fileObject)) ? 0 : -1;
    };

    return fileWrapper;
}

static std::unique_ptr<Exporter> createExporter(
    const FrameDef& frameDef, CodecAlpha alpha, int quality,
    int64_t frameRateNumerator, int64_t frameRateDenominator,
    int64_t maxFrames, int32_t reserveMetadataSpace,
    const MovieFile& file, MovieErrorCallback errorCallback,
    bool withAudio, int sampleRate, int32_t numAudioChannels,
    exDoExportRec* exportInfoP, prMALError& error,  //!!! not ideal passing these in
    bool writeMoovTagEarly
)
{
    std::unique_ptr<EncoderParametersBase> parameters = std::make_unique<EncoderParametersBase>(
        frameDef,
        alpha,
        quality
        );

    UniqueEncoder encoder = CodecRegistry::codec()->createEncoder(std::move(parameters));

    std::unique_ptr<MovieWriter> writer = std::make_unique<MovieWriter>(
        encoder->subType(), encoder->name(),
        frameDef.width, frameDef.height,
        encoder->encodedBitDepth(),
        frameRateNumerator, frameRateDenominator,
        maxFrames, reserveMetadataSpace,
        file,
        errorCallback,
        writeMoovTagEarly   // writeMoovTagEarly
        );

    if (withAudio)
    {
        writer->addAudioStream(numAudioChannels, sampleRate, 2, AudioEncoding_Signed_PCM);
    }

    writer->writeHeader();

    if (withAudio)
        renderAndWriteAllAudio(exportInfoP, error, writer.get());

    return std::make_unique<Exporter>(std::move(encoder), std::move(writer));
}

void exportLoop(exDoExportRec* exportInfoP, prMALError& error)
{
    ExportSettings* settings = reinterpret_cast<ExportSettings*>(exportInfoP->privateData);

    ExportLoopRenderParams renderParams;

    renderParams.inRenderParamsSize = sizeof(ExportLoopRenderParams);
    renderParams.inRenderParamsVersion = kPrSDKExporterUtilitySuiteVersion;
    renderParams.inFinalPixelFormat = CodecRegistry().isHighBitDepth()
        ? PrPixelFormat_BGRA_4444_16u // PrPixelFormat_BGRA_4444_32f
        : PrPixelFormat_BGRA_4444_8u;
    renderParams.inStartTime = exportInfoP->startTime;
    renderParams.inEndTime = exportInfoP->endTime;
    renderParams.inReservedProgressPreRender = 0.0; //!!!
    renderParams.inReservedProgressPostRender = 0.0; //!!!

    prMALError multipassExportError = settings->exporterUtilitySuite->DoMultiPassExportLoop(
        exportInfoP->exporterPluginID,
        &renderParams,
        1,  // number of passes
        c_onFrameComplete,
        settings
    );
    if (malNoError != multipassExportError)
    {
        if (error == malNoError)  // retain error if it was set in per-frame export
            error = multipassExportError;
        throw std::runtime_error("DoMultiPassExportLoop failed");
    }
}

static void renderAndWriteAllVideo(exDoExportRec* exportInfoP, prMALError& error)
{
	const csSDK_uint32 exID = exportInfoP->exporterPluginID;
	ExportSettings* settings = reinterpret_cast<ExportSettings*>(exportInfoP->privateData);
	exParamValues ticksPerFrame, width, height, includeAlphaChannel, quality;
	PrTime ticksPerSecond;

    settings->logMessage("codec implementation: " + CodecRegistry::logName());

	settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoFPS, &ticksPerFrame);
	settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoWidth, &width);
	settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoHeight, &height);
    settings->exportParamSuite->GetParamValue(exID, 0, NOTCHLCIncludeAlphaChannel, &includeAlphaChannel);
    settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoQuality, &quality);
    settings->timeSuite->GetTicksPerSecond(&ticksPerSecond);

    const int64_t frameRateNumerator = ticksPerSecond;
    const int64_t frameRateDenominator = ticksPerFrame.value.timeValue;

    int64_t maxFrames = double((exportInfoP->endTime - exportInfoP->startTime)) / frameRateDenominator;
    
    //!!!
    int clampedQuality = std::clamp(quality.value.intValue, 1, 5);

    ChannelFormat channelFormat(CodecRegistry::isHighBitDepth() ? ChannelFormat_U16_32k : ChannelFormat_U8); // we're going to request frames in keeping with the codec's high bit depth
    FrameFormat format(channelFormat | FrameOrigin_BottomLeft | ChannelLayout_BGRA);
    FrameDef frameDef(width.value.intValue, height.value.intValue, format);

    CodecAlpha alpha = includeAlphaChannel.value.intValue ? withAlpha : withoutAlpha;

    MovieErrorCallback errorCallback([&](const char *msg) { settings->reportError(msg); });

    MovieFile movieFile(createMovieFile(settings->exportFileSuite, exportInfoP->fileObject, errorCallback));

    bool withAudio(false);
    int32_t numAudioChannels(0);
    int audioSampleRate(0);
    if (exportInfoP->exportAudio) 
    {
        withAudio = true;

        exParamValues sampleRate, channelType;
        settings->exportParamSuite->GetParamValue(exID, 0, ADBEAudioRatePerSecond, &sampleRate);
        settings->exportParamSuite->GetParamValue(exID, 0, ADBEAudioNumChannels, &channelType);
        audioSampleRate = (int)sampleRate.value.floatValue;
        numAudioChannels = GetNumberOfAudioChannels(channelType.value.intValue);
    }

    try {
        movieFile.onOpenForWrite();  //!!! move to writer

        settings->exporter = createExporter(
            frameDef, alpha, clampedQuality,
            frameRateNumerator, frameRateDenominator,
            maxFrames,
            exportInfoP->reserveMetaDataSpace,
            movieFile, errorCallback,
            withAudio, audioSampleRate, numAudioChannels,
            exportInfoP, error,
            true  // writeMoovTagEarly
        );

        exportLoop(exportInfoP, error);

        // this may throw
        try
        {
            settings->exporter->close();
        }
        catch (const MovieWriterInvalidData&)
        {
            // this will happen if we MovieWriter didn't guess large enough on header size.
            //  => we have to guess a header size else Adobe will copy the file and rejig it
            //     with the header at the start. This is unwanted but unavoidable if the header
            //     isn't placed ahead of mdat
            // => simplest way out is to redo the export, without the guess, and let adobe do its copy

            // start with as clean a slate as possible
            try {
                settings->exporter.reset(nullptr);
            }
            catch (...)
            {
            }

            movieFile.onOpenForWrite();  //!!! move to writer

            settings->exporter = createExporter(
                frameDef, alpha, clampedQuality,
                frameRateNumerator, frameRateDenominator,
                maxFrames,
                exportInfoP->reserveMetaDataSpace,
                movieFile, errorCallback,
                withAudio, audioSampleRate, numAudioChannels,
                exportInfoP, error,
                false   // writeMoovTagEarly
            );

            exportLoop(exportInfoP, error);

            settings->exporter->close();
        }
    }
    catch (...)
    {
        settings->exporter.reset(nullptr);
        throw;
    }

    settings->exporter.reset(nullptr);
}

csSDK_int32 GetNumberOfAudioChannels(csSDK_int32 audioChannelType)
{
    csSDK_int32 numberOfChannels = -1;

    if (audioChannelType == kPrAudioChannelType_Mono)
        numberOfChannels = 1;

    else if (audioChannelType == kPrAudioChannelType_Stereo)
        numberOfChannels = 2;

    else if (audioChannelType == kPrAudioChannelType_51)
        numberOfChannels = 6;

    return numberOfChannels;
}

static void renderAndWriteAllAudio(exDoExportRec *exportInfoP, prMALError &error, MovieWriter *writer)
{
    // All audio calls to and from Premiere use arrays of buffers of 32-bit floats to pass audio.
    // Audio is not interleaved, rather separate channels are stored in separate buffers.
    const int kAudioSampleSizePremiere = sizeof(float_t);

    // Assume we export 16bit audio and pack up to 1024 samples per packet
    const int kAudioSampleSizeOutput = sizeof(int16_t);
    const int kMaxAudioSamplesPerPacket = 1024;

    const csSDK_uint32 exID = exportInfoP->exporterPluginID;
    ExportSettings *settings = reinterpret_cast<ExportSettings *>(exportInfoP->privateData);
    exParamValues ticksPerFrame, sampleRate, channelType;

    settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoFPS, &ticksPerFrame);
    settings->exportParamSuite->GetParamValue(exID, 0, ADBEAudioRatePerSecond, &sampleRate);
    settings->exportParamSuite->GetParamValue(exID, 0, ADBEAudioNumChannels, &channelType);
    csSDK_int32 numAudioChannels = GetNumberOfAudioChannels(channelType.value.intValue);

    csSDK_uint32 audioRenderID = 0;
    settings->sequenceAudioSuite->MakeAudioRenderer(exID,
                                                    exportInfoP->startTime,
                                                    (PrAudioChannelType)channelType.value.intValue,
                                                    kPrAudioSampleType_32BitFloat,
                                                    (float)sampleRate.value.floatValue,
                                                    &audioRenderID);

    PrTime ticksPerSample = 0;
    settings->timeSuite->GetTicksPerAudioSample((float)sampleRate.value.floatValue, &ticksPerSample);

    PrTime exportDuration = exportInfoP->endTime - exportInfoP->startTime;
    csSDK_int64 totalAudioSamples = exportDuration / ticksPerSample;
    csSDK_int64 samplesRemaining = totalAudioSamples;

    // Allocate audio buffers
    csSDK_int32 audioBufferSize = kMaxAudioSamplesPerPacket;
    auto audioBufferOut = (csSDK_int16 *)settings->memorySuite->NewPtr(numAudioChannels * audioBufferSize * kAudioSampleSizeOutput);
    float *audioBuffer[kMaxAudioChannelCount];
    for (csSDK_int32 bufferIndexL = 0; bufferIndexL < numAudioChannels; bufferIndexL++)
    {
        audioBuffer[bufferIndexL] = (float *)settings->memorySuite->NewPtr(audioBufferSize * kAudioSampleSizePremiere);
    }

    // Progress bar init with label
    float progress = 0.f;
    prUTF16Char tempStrProgress[256];
    copyConvertStringLiteralIntoUTF16(L"Preparing Audio...", tempStrProgress);
    settings->exportProgressSuite->SetProgressString(exID, tempStrProgress);

    // GetAudio loop
    csSDK_int32 samplesRequested, maxBlipSize;
    csSDK_int64 samplesExported = 0; // pts
    prMALError resultS = malNoError;
    while (samplesRemaining && (resultS == malNoError))
    {
        // Find size of blip to ask for
        settings->sequenceAudioSuite->GetMaxBlip(audioRenderID, ticksPerFrame.value.timeValue, &maxBlipSize);
        samplesRequested = std::min(audioBufferSize, maxBlipSize);
        if (samplesRequested > samplesRemaining)
            samplesRequested = (csSDK_int32)samplesRemaining;

        // Fill the buffer with audio
        resultS = settings->sequenceAudioSuite->GetAudio(audioRenderID, samplesRequested, audioBuffer, kPrFalse);
        if (resultS != malNoError)
            break;

        settings->audioSuite->ConvertAndInterleaveTo16BitInteger(audioBuffer, audioBufferOut,
                                                                 numAudioChannels, samplesRequested);

        // Write audioBufferOut as one packet
        writer->writeAudioFrame(reinterpret_cast<const uint8_t *>(audioBufferOut),
                                int64_t(samplesRequested) * int64_t(numAudioChannels) * int64_t(kAudioSampleSizeOutput),
                                samplesExported);

        // Write audioBufferOut as separate samples
        // auto buf = reinterpret_cast<const uint8_t *>(audioBufferOut);
        // for (csSDK_int32 i = 0; i < samplesRequested; i++)
        // {
        //     csSDK_int32 offset = i * numAudioChannels * kAudioSampleSizeOutput;
        //     writer->writeAudioFrame(&buf[offset],
        //                             numAudioChannels * kAudioSampleSizeOutput,
        //                             samplesExported + i);
        // }

        // Calculate remaining audio
        samplesExported += samplesRequested;
        samplesRemaining -= samplesRequested;

        // Update progress bar percent
        progress = (float) samplesExported / totalAudioSamples * 0.06f;
        settings->exportProgressSuite->UpdateProgressPercent(exID, progress);
    }
    error = resultS;

    // Reset progress bar label
    copyConvertStringLiteralIntoUTF16(L"", tempStrProgress);
    settings->exportProgressSuite->SetProgressString(exID, tempStrProgress);

    // Free up
    settings->memorySuite->PrDisposePtr((PrMemoryPtr)audioBufferOut);
    for (csSDK_int32 bufferIndexL = 0; bufferIndexL < numAudioChannels; bufferIndexL++)
    {
        settings->memorySuite->PrDisposePtr((PrMemoryPtr)audioBuffer[bufferIndexL]);
    }
    settings->sequenceAudioSuite->ReleaseAudioRenderer(exID, audioRenderID);
}

prMALError doExport(exportStdParms* stdParmsP, exDoExportRec* exportInfoP)
{
    ExportSettings* settings = reinterpret_cast<ExportSettings*>(exportInfoP->privateData);
    prMALError error = malNoError;

    try {
        // if (exportInfoP->exportAudio)
        //     renderAndWriteAllAudio(exportInfoP, error);

        if (exportInfoP->exportVideo)
            renderAndWriteAllVideo(exportInfoP, error);
    }
    catch (const std::exception& ex)
    {
        settings->reportError(ex.what());
        return (error == malNoError) ? malUnknownError : error;
    }
    catch (...)
    {
        settings->reportError("unspecified error while rendering and writing video");
        return (error == malNoError) ? malUnknownError : error;
    }

    return 	malNoError;
}
