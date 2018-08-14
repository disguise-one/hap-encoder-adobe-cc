#include "premiereParams.hpp"
#include "export_settings.hpp"
#include "prstring.hpp"
#include "configure.hpp"

prMALError generateDefaultParams(exportStdParms *stdParms, exGenerateDefaultParamRec *generateDefaultParamRec)
{
    prMALError result = malNoError;
    ExportSettings* settings = reinterpret_cast<ExportSettings*>(generateDefaultParamRec->privateData);
    PrSDKExportParamSuite* exportParamSuite = settings->exportParamSuite;
    PrSDKExportInfoSuite* exportInfoSuite = settings->exportInfoSuite;
    PrSDKTimeSuite* timeSuite = settings->timeSuite;
    csSDK_int32	exporterPluginID = generateDefaultParamRec->exporterPluginID;
    csSDK_int32	mgroupIndex = 0;
    PrParam	hasVideo,
        hasAudio,
        seqWidth,
        seqHeight,
        seqPARNum,
        seqPARDen,
        seqFrameRate,
        seqFieldOrder,
        seqChannelType,
        seqSampleRate;
    prUTF16Char tempString[256];

    if (exportInfoSuite)
    {
        exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_SourceHasVideo, &hasVideo);
        exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_SourceHasAudio, &hasAudio);
        exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_VideoWidth, &seqWidth);
        exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_VideoHeight, &seqHeight);

        if (seqWidth.mInt32 == 0)
            seqWidth.mInt32 = 1920;

        if (seqHeight.mInt32 == 0)
            seqHeight.mInt32 = 1080;

        exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_VideoFrameRate, &seqFrameRate);
        exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_PixelAspectNumerator, &seqPARNum);
		exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_PixelAspectDenominator, &seqPARDen);
        exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_VideoFieldType, &seqFieldOrder);
        exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_AudioChannelsType, &seqChannelType);
        exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_AudioSampleRate, &seqSampleRate);

        settings->sourceFieldType = seqFieldOrder.mInt32;
    }

    if (exportParamSuite)
    {
        exportParamSuite->AddMultiGroup(exporterPluginID, &mgroupIndex);
        copyConvertStringLiteralIntoUTF16(TOP_VIDEO_PARAM_GROUP_NAME, tempString);
        exportParamSuite->AddParamGroup(exporterPluginID, mgroupIndex, ADBETopParamGroup, ADBEVideoTabGroup, tempString, kPrFalse, kPrFalse, kPrFalse);
        copyConvertStringLiteralIntoUTF16(VIDEO_CODEC_PARAM_GROUP_NAME, tempString);
        exportParamSuite->AddParamGroup(exporterPluginID, mgroupIndex, ADBEVideoTabGroup, ADBEVideoCodecGroup, tempString, kPrFalse, kPrFalse, kPrFalse);
        copyConvertStringLiteralIntoUTF16(BASIC_VIDEO_PARAM_GROUP_NAME, tempString);
        exportParamSuite->AddParamGroup(exporterPluginID, mgroupIndex, ADBEVideoTabGroup, ADBEBasicVideoGroup, tempString, kPrFalse, kPrFalse, kPrFalse);

        exNewParamInfo widthParam;
        exParamValues widthValues;
        safeStrCpy(widthParam.identifier, 256, ADBEVideoWidth);
        widthParam.paramType = exParamType_int;
        widthParam.flags = exParamFlag_none;
        widthValues.rangeMin.intValue = 16;
        widthValues.rangeMax.intValue = 8192;
        widthValues.value.intValue = seqWidth.mInt32;
        widthValues.disabled = kPrFalse;
        widthValues.hidden = kPrFalse;
        widthParam.paramValues = widthValues;
        exportParamSuite->AddParam(exporterPluginID, mgroupIndex, ADBEBasicVideoGroup, &widthParam);

        exNewParamInfo heightParam;
        exParamValues heightValues;
        safeStrCpy(heightParam.identifier, 256, ADBEVideoHeight);
        heightParam.paramType = exParamType_int;
        heightParam.flags = exParamFlag_none;
        heightValues.rangeMin.intValue = 16;
        heightValues.rangeMax.intValue = 8192;
        heightValues.value.intValue = seqHeight.mInt32;
        heightValues.disabled = kPrFalse;
        heightValues.hidden = kPrFalse;
        heightParam.paramValues = heightValues;
        exportParamSuite->AddParam(exporterPluginID, mgroupIndex, ADBEBasicVideoGroup, &heightParam);

		exNewParamInfo hapSubcodecParam;
		exParamValues hapSubcodecValues;
		safeStrCpy(hapSubcodecParam.identifier, 256, ADBEVideoCodec);
		hapSubcodecParam.paramType = exParamType_int;
		hapSubcodecParam.flags = exParamFlag_none;
		hapSubcodecValues.rangeMin.intValue = 0;
		hapSubcodecValues.rangeMax.intValue = 4;
		auto temp = kHapAlphaCodecSubType;
		hapSubcodecValues.value.intValue = reinterpret_cast<int32_t&>(temp); //!!! seqHapSubcodec.mInt32;
		hapSubcodecValues.disabled = kPrFalse;
		hapSubcodecValues.hidden = kPrFalse;
		hapSubcodecParam.paramValues = hapSubcodecValues;
		exportParamSuite->AddParam(exporterPluginID, mgroupIndex, ADBEBasicVideoGroup, &hapSubcodecParam);

		exNewParamInfo PARParam;
		exParamValues PARValues;
		safeStrCpy(PARParam.identifier, 256, ADBEVideoAspect);
		PARParam.paramType = exParamType_ratio;
		PARParam.flags = exParamFlag_none;
		PARValues.rangeMin.ratioValue.numerator = 10;
		PARValues.rangeMin.ratioValue.denominator = 11;
		PARValues.rangeMax.ratioValue.numerator = 2;
		PARValues.rangeMax.ratioValue.denominator = 1;
		PARValues.value.ratioValue.numerator = seqPARNum.mInt32;
		PARValues.value.ratioValue.denominator = seqPARDen.mInt32;
		PARValues.disabled = kPrFalse;
		PARValues.hidden = kPrFalse;
		PARParam.paramValues = PARValues;
		exportParamSuite->AddParam(exporterPluginID, mgroupIndex, ADBEBasicVideoGroup, &PARParam);

		exNewParamInfo frameRateParam;
        exParamValues frameRateValues;
        safeStrCpy(frameRateParam.identifier, 256, ADBEVideoFPS);
        frameRateParam.paramType = exParamType_ticksFrameRate;
        frameRateParam.flags = exParamFlag_none;
        frameRateValues.rangeMin.timeValue = 1;
        timeSuite->GetTicksPerSecond(&frameRateValues.rangeMax.timeValue);
        frameRateValues.value.timeValue = seqFrameRate.mInt64;
        frameRateValues.disabled = kPrFalse;
        frameRateValues.hidden = kPrFalse;
        frameRateParam.paramValues = frameRateValues;
        exportParamSuite->AddParam(exporterPluginID, mgroupIndex, ADBEBasicVideoGroup, &frameRateParam);

        // exNewParamInfo compParam;
        // exParamValues compValues;
        // safeStrCpy(compParam.identifier, 256, ADBECompressionLevel);
        // compParam.paramType = exParamType_int;
        // compParam.flags = exParamFlag_none;
        // compValues.rangeMin.intValue = 0;
        // compValues.rangeMax.intValue = 0;
        // compValues.value.intValue = 11;
        // compValues.disabled = kPrFalse;
        // compValues.hidden = kPrFalse;
        // compParam.paramValues = compValues;
		// exportParamSuite->AddParam(exporterPluginID, mgroupIndex, ADBEBasicVideoGroup, &compParam);

        exportParamSuite->SetParamsVersion(exporterPluginID, 3);
    }

    return result;
}

prMALError postProcessParams(exportStdParms *stdParmsP, exPostProcessParamsRec *postProcessParamsRecP)
{
    const csSDK_uint32 exID = postProcessParamsRecP->exporterPluginID;
    ExportSettings* settings = reinterpret_cast<ExportSettings*>(postProcessParamsRecP->privateData);
    PrTime ticksPerSecond = 0;
	exOneParamValueRec tempHapSubcodec;
	CodecSubType HAPsubcodecs[] = { kHapCodecSubType, kHapAlphaCodecSubType, kHapYCoCgCodecSubType, kHapYCoCgACodecSubType, kHapAOnlyCodecSubType };
    exOneParamValueRec tempFrameRate;
    PrTime frameRates[] = { 10, 15, 23, 24, 25, 29, 30, 50, 59, 60 };
    PrTime frameRateNumDens[][2] = { { 10, 1 }, { 15, 1 }, { 24000, 1001 }, { 24, 1 }, { 25, 1 }, { 30000, 1001 }, { 30, 1 }, { 50, 1 }, { 60000, 1001 }, { 60, 1 } };
    //exOneParamValueRec tempComp;
    csSDK_int32 compLevel[] = { 3, 9, 11, 12 };

    prUTF16Char tempString[256];
    const wchar_t* frameRateStrings[] = { STR_FRAME_RATE_10, STR_FRAME_RATE_15, STR_FRAME_RATE_23976, STR_FRAME_RATE_24, STR_FRAME_RATE_25, STR_FRAME_RATE_2997, STR_FRAME_RATE_30, STR_FRAME_RATE_50, STR_FRAME_RATE_5994, STR_FRAME_RATE_60 };
	const wchar_t *hapSubcodecStrings[] = { STR_HAP_SUBCODEC_0, STR_HAP_SUBCODEC_1, STR_HAP_SUBCODEC_2, STR_HAP_SUBCODEC_3, STR_HAP_SUBCODEC_4 };

	settings->timeSuite->GetTicksPerSecond(&ticksPerSecond);
    for (csSDK_int32 i = 0; i < sizeof(frameRates) / sizeof(PrTime); i++)
        frameRates[i] = ticksPerSecond / frameRateNumDens[i][0] * frameRateNumDens[i][1];

    copyConvertStringLiteralIntoUTF16(VIDEO_CODEC_PARAM_GROUP_NAME, tempString);
    settings->exportParamSuite->SetParamName(exID, 0, ADBEVideoCodecGroup, tempString);

    copyConvertStringLiteralIntoUTF16(STR_CODEC, tempString);
    settings->exportParamSuite->SetParamName(exID, 0, ADBEVideoCodec, tempString);
    copyConvertStringLiteralIntoUTF16(STR_CODEC_TOOLTIP, tempString);
    settings->exportParamSuite->SetParamDescription(exID, 0, ADBEVideoCodec, tempString);

    copyConvertStringLiteralIntoUTF16(BASIC_VIDEO_PARAM_GROUP_NAME, tempString);
    settings->exportParamSuite->SetParamName(exID, 0, ADBEBasicVideoGroup, tempString);

    copyConvertStringLiteralIntoUTF16(STR_WIDTH, tempString);
    settings->exportParamSuite->SetParamName(exID, 0, ADBEVideoWidth, tempString);

    copyConvertStringLiteralIntoUTF16(STR_HEIGHT, tempString);
    settings->exportParamSuite->SetParamName(exID, 0, ADBEVideoHeight, tempString);

	copyConvertStringLiteralIntoUTF16(STR_HAP_SUBCODEC, tempString);
	settings->exportParamSuite->SetParamName(exID, 0, ADBEVideoCodec, tempString);

    copyConvertStringLiteralIntoUTF16(STR_FRAME_RATE, tempString);
    settings->exportParamSuite->SetParamName(exID, 0, ADBEVideoFPS, tempString);

    settings->exportParamSuite->ClearConstrainedValues(exID, 0, ADBEVideoCodec);
	for (csSDK_int32 i = 0; i < sizeof(HAPsubcodecs) / sizeof(HAPsubcodecs[0]); i++)
	{
		tempHapSubcodec.intValue = reinterpret_cast<int32_t &>(HAPsubcodecs[i][0]);
		copyConvertStringLiteralIntoUTF16(hapSubcodecStrings[i], tempString);
		settings->exportParamSuite->AddConstrainedValuePair(exID, 0, ADBEVideoCodec, &tempHapSubcodec, tempString);
	}

    settings->exportParamSuite->ClearConstrainedValues(exID, 0, ADBEVideoFPS);

    for (csSDK_int32 i = 0; i < sizeof(frameRates) / sizeof(PrTime); i++)
    {
        tempFrameRate.timeValue = frameRates[i];
        copyConvertStringLiteralIntoUTF16(frameRateStrings[i], tempString);
        settings->exportParamSuite->AddConstrainedValuePair(exID, 0, ADBEVideoFPS, &tempFrameRate, tempString);
    }

    return malNoError;
}

prMALError getParamSummary(exportStdParms *stdParmsP, exParamSummaryRec *summaryRecP)
{
    wchar_t videoSummary[256];
    exParamValues width, height, frameRate, hapSubcodec, pixelAspectRatio;
    ExportSettings* settings = reinterpret_cast<ExportSettings*>(summaryRecP->privateData);
    PrSDKExportParamSuite* paramSuite = settings->exportParamSuite;
    PrSDKTimeSuite* timeSuite = settings->timeSuite;
    PrTime ticksPerSecond;
    const csSDK_int32 mgroupIndex = 0;
    const csSDK_int32 exporterPluginID = summaryRecP->exporterPluginID;

    if (!paramSuite)
        return malNoError;

    paramSuite->GetParamValue(exporterPluginID, mgroupIndex, ADBEVideoWidth, &width);
    paramSuite->GetParamValue(exporterPluginID, mgroupIndex, ADBEVideoHeight, &height);
    paramSuite->GetParamValue(exporterPluginID, mgroupIndex, ADBEVideoFPS, &frameRate);
	paramSuite->GetParamValue(exporterPluginID, mgroupIndex, ADBEVideoCodec, &hapSubcodec);
    paramSuite->GetParamValue(exporterPluginID, mgroupIndex, ADBEVideoAspect, &pixelAspectRatio);
    timeSuite->GetTicksPerSecond(&ticksPerSecond);

    swprintf(videoSummary, 256, L"%ix%i, %.2f fps", width.value.intValue, height.value.intValue, static_cast<float>(ticksPerSecond) / static_cast<float>(frameRate.value.timeValue));
    copyConvertStringLiteralIntoUTF16(videoSummary, summaryRecP->videoSummary);

    return malNoError;
}

prMALError paramButton(exportStdParms *stdParmsP, exParamButtonRec *getFilePrefsRecP)
{
    return malNoError;
}

prMALError validateParamChanged(exportStdParms *stdParmsP, exParamChangedRec *validateParamChangedRecP)
{
    ExportSettings* settings = reinterpret_cast<ExportSettings*>(validateParamChangedRecP->privateData);

    if (settings->exportParamSuite == nullptr)
        return exportReturn_ErrMemory;

    return malNoError;
}
