#include <new>
#include <sstream>

#include <nlohmann/json.hpp>
// for convenience
using json = nlohmann::json;

#include "../premiere_CC2018/exporter/exporter.hpp"
#include "../premiere_CC2018/string_conversion.hpp"
#include "codec_registration.hpp"

#include "output_module.h"
#include "ui.h"

static AEGP_PluginID S_mem_id = 0;

static A_Err DeathHook(
    AEGP_GlobalRefcon unused1 ,
    AEGP_DeathRefcon unused2)
{
    CodecRegistry::codec().reset();

    return A_Err_NONE;
}

struct OutputOptions
{
    OutputOptions()
        : subType{ CodecRegistry::codec()->details().defaultSubType },
          quality(CodecRegistry::defaultQuality()),
          chunkCount(0)
    {}
    ~OutputOptions() {}

    CodecSubType subType;
    int          quality;
    int          chunkCount;

    // we're forced to store the entire output module context here, including temporary state
    // appears to be no other per-module supplied spot, and we don't want to use static globals
    std::unique_ptr<Exporter> exporter;
};

void to_json(json& j, const OutputOptions& o) {
    const auto& codec = *CodecRegistry::codec();

    j = json{};
    bool hasSubTypes = (codec.details().subtypes.size() > 0);
    if (hasSubTypes) {
        j["subType"] = o.subType;
    }
    if (codec.hasQuality(o.subType)) {
        j["quality"] = o.quality;
    };
    if (codec.details().hasChunkCount) {
        j["chunkCount"] = o.chunkCount;
    }
}

void from_json(const json& j, OutputOptions& o) {
    const auto& codec = *CodecRegistry::codec();

    if (codec.details().subtypes.size())
    {
        j.at("subType").get_to(o.subType);
    }
    if (codec.hasQuality(o.subType)) {
        j.at("quality").get_to(o.quality);
    }
    if (codec.details().hasChunkCount) {
        j.at("chunkCount").get_to(o.chunkCount);
    }
}


// we need to get a local structure, OutputOptions, which will be associated with an
// output sessions. This is obtained via a handle, then locking that handle
struct OutputOptionsHandleWrapper;
typedef std::unique_ptr<OutputOptions, OutputOptionsHandleWrapper> OutputOptionsUP;

struct OutputOptionsHandleWrapper
{
    typedef std::function<void()> Releaser;
    Releaser release;

    OutputOptionsHandleWrapper(Releaser release_)
        : release(release_)
    {
    }

    void operator()(OutputOptions *)
    {
        release();
    }

    static OutputOptionsUP
    wrap(AEGP_SuiteHandler& suites, AEIO_OutSpecH outH)
    {
        A_Err err = A_Err_NONE;
        AEIO_Handle optionsH = 0;
        OutputOptions *optionsP;

        ERR(suites.IOOutSuite4()->AEGP_GetOutSpecOptionsHandle(outH, reinterpret_cast<void**>(&optionsH)));
        if (!err && optionsH) {
            ERR(suites.MemorySuite1()->AEGP_LockMemHandle(optionsH, reinterpret_cast<void**>(&optionsP)));

            if (!err)
                return OutputOptionsUP(
                    optionsP,
                    OutputOptionsHandleWrapper([&, optionsH](...) {  suites.MemorySuite1()->AEGP_UnlockMemHandle(optionsH); })
                );
        }
        return OutputOptionsUP(nullptr, OutputOptionsHandleWrapper(0));
    }
};


static MovieFile createMovieFile(const std::string &filename,
                                 MovieErrorCallback errorCallback)
{
    MovieFile fileWrapper;
    auto file=std::make_shared<FILE *>((FILE *)nullptr);
    fileWrapper.onOpenForWrite = [=]() {
#ifdef _WIN64
        fopen_s(file.get(), filename.c_str(), "wb");
#else
        FILE *ptr = fopen(filename.c_str(), "wb");
        *file = ptr;
#endif
        if (!(*file))
            throw std::runtime_error("couldn't open output file");
    };
    fileWrapper.onWrite = [=](const uint8_t* buffer, size_t size) {
        auto nWritten = fwrite(buffer, size, 1, *file);
        if (!nWritten) {
            errorCallback("Could not write to file");
            return -1;
        }
        return 0;
    };
    fileWrapper.onSeek = [=](int64_t offset, int whence) {
#ifdef AE_OS_WIN
        auto result = _fseeki64(*file, offset, whence);
#else
        auto result = fseek(*file, offset, whence);
#endif
        if (0 != result) {
            errorCallback("Could not seek in file");
            return -1;
        }
        return 0;
    };
    fileWrapper.onClose = [=]() {
        return (fclose(*file)==0) ? 0 : -1;
    };

    return fileWrapper;
}


static std::unique_ptr<Exporter> createExporter(
    const FrameDef& frameDef, CodecAlpha alpha, bool hasCodecSubType, CodecSubType subType, bool hasChunkCount, HapChunkCounts chunkCounts, int quality,
    int64_t frameRateNumerator, int64_t frameRateDenominator,
    int32_t maxFrames, int32_t reserveMetadataSpace,
    const MovieFile& file, MovieErrorCallback errorCallback,
    bool withAudio, int sampleRate, int32_t numAudioChannels, int32_t audioBytesPerSample, AudioEncoding audioEncoding,
    bool writeMoovTagEarly
)
{
    std::unique_ptr<EncoderParametersBase> parameters = std::make_unique<EncoderParametersBase>(
        frameDef,
        alpha,
        hasCodecSubType, subType,
        hasChunkCount, chunkCounts,
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
        writer->addAudioStream(numAudioChannels, sampleRate, audioBytesPerSample, audioEncoding);
    }

    writer->writeHeader();

    return std::make_unique<Exporter>(std::move(encoder), std::move(writer));
}

static A_Err
AEIO_InitOutputSpec(
    AEIO_BasicData *basic_dataP,
    AEIO_OutSpecH   outH,
    A_Boolean      *user_interacted)
{
    A_Err       err = A_Err_NONE;
    AEIO_Handle new_optionsH = NULL,
                old_optionsH = 0;
    OutputOptions  *new_optionsP;
    char           *old_optionsP;
    AEGP_SuiteHandler suites(basic_dataP->pica_basicP);

    ERR(suites.IOOutSuite4()->AEGP_GetOutSpecOptionsHandle(outH, reinterpret_cast<void**>(&old_optionsH)));

    if (!err) {
        ERR(suites.MemorySuite1()->AEGP_NewMemHandle(S_mem_id,
            "InitOutputSpec options",
            sizeof(OutputOptions),
            AEGP_MemFlag_CLEAR,
            &new_optionsH));
        if (!err && new_optionsH) {
            ERR(suites.MemorySuite1()->AEGP_LockMemHandle(new_optionsH, reinterpret_cast<void**>(&new_optionsP)));

            if (!err && new_optionsP) {
                new (new_optionsP) OutputOptions();

                if (!old_optionsH) {
                    // old code constructed here, but we do so above because we always want a
                    // constructed object, even if the the following code fails
                }
                else {
                    AEGP_MemSize old_options_size;
                    ERR(suites.MemorySuite1()->AEGP_GetMemHandleSize(old_optionsH, &old_options_size));
                    ERR(suites.MemorySuite1()->AEGP_LockMemHandle(old_optionsH, reinterpret_cast<void**>(&old_optionsP)));

                    if (!err && new_optionsP && old_optionsP) {
                        std::string s(old_optionsP, old_options_size);
                        try {
                            auto j = json::parse(s);
                            j.get_to(*new_optionsP);
                            *user_interacted = FALSE;
                        }
                        catch (...)
                        {
                            // error = badly serialised data; indicate we're replacing it
                            *user_interacted = TRUE;
                        }

                        ERR(suites.MemorySuite1()->AEGP_UnlockMemHandle(old_optionsH));
                    }
                }
                ERR(suites.MemorySuite1()->AEGP_UnlockMemHandle(new_optionsH));

                ERR(suites.IOOutSuite4()->AEGP_SetOutSpecOptionsHandle(outH, new_optionsH, reinterpret_cast<void**>(&old_optionsH)));
            }
        }
    }
    if (old_optionsH) {
        ERR(suites.MemorySuite1()->AEGP_FreeMemHandle(old_optionsH));
    }
    return err;
}


static A_Err    
AEIO_GetFlatOutputOptions(
    AEIO_BasicData    *basic_dataP,
    AEIO_OutSpecH    outH, 
    AEIO_Handle        *new_optionsPH)
{
    A_Err                        err                = A_Err_NONE;
    AEIO_Handle                    old_optionsH    = NULL;
    OutputOptions *new_optionsP;
    AEGP_SuiteHandler            suites(basic_dataP->pica_basicP);

    OutputOptionsUP old_optionsUP = OutputOptionsHandleWrapper::wrap(suites, outH);
    if (!old_optionsUP)
        return A_Err_PARAMETER;


    json j(*old_optionsUP);
    std::string s = j.dump();

    ERR(suites.MemorySuite1()->AEGP_NewMemHandle( S_mem_id, 
                                                  "flat optionsH", 
                                                  AEGP_MemSize(s.size() + 1),
                                                  AEGP_MemFlag_CLEAR, 
                                                  new_optionsPH));
    if (!err && *new_optionsPH) {
        ERR(suites.MemorySuite1()->AEGP_LockMemHandle(*new_optionsPH, reinterpret_cast<void**>(&new_optionsP)));

        if (!err && new_optionsP && old_optionsUP) {
            // Convert the old unflat structure into a separate flat structure for output
            // In this case, we just do a simple copy
            memcpy(new_optionsP, &s[0], s.size() + 1);

            ERR(suites.MemorySuite1()->AEGP_UnlockMemHandle(*new_optionsPH));
        }
    }

    return err;
}


static A_Err    
AEIO_DisposeOutputOptions(
    AEIO_BasicData    *basic_dataP,
    void			*optionsPV)
{ 
    AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);
    AEIO_Handle			optionsH	=	reinterpret_cast<AEIO_Handle>(optionsPV);
    OutputOptions      *optionsP;
    A_Err				err			=	A_Err_NONE;
    
    if (optionsH){
        ERR(suites.MemorySuite1()->AEGP_LockMemHandle(optionsH, reinterpret_cast<void**>(&optionsP)));
        optionsP->~OutputOptions();
        ERR(suites.MemorySuite1()->AEGP_UnlockMemHandle(optionsH));
        ERR(suites.MemorySuite1()->AEGP_FreeMemHandle(optionsH));
    }
    return err;
};

static A_Err	
AEIO_UserOptionsDialog(
    AEIO_BasicData			*basic_dataP,
    AEIO_OutSpecH			outH, 
    const PF_EffectWorld	*sample0,
    A_Boolean				*user_interacted0)
{ 
    A_Err						err				= A_Err_NONE;
    AEGP_SuiteHandler			suites(basic_dataP->pica_basicP);
    AEIO_Handle					optionsH		= NULL, 
                                old_optionsH	= 0;
    {
        OutputOptionsUP optionsUP = OutputOptionsHandleWrapper::wrap(suites, outH);
        if (!optionsUP)
            return A_Err_PARAMETER;

        // get platform handles
#ifdef AE_OS_WIN
        HWND hwndOwner = NULL;
        hwndOwner = GetForegroundWindow();
#else
        void *hwndOwner = nullptr;
        // #error TODO: 
#endif
        if (NULL==hwndOwner)
            suites.UtilitySuite6()->AEGP_GetMainHWND((void *)&hwndOwner);

        if (ui_OutDialog(optionsUP->subType, optionsUP->quality, optionsUP->chunkCount, (void *)&hwndOwner))
            *user_interacted0 = TRUE;
        else
            *user_interacted0 = FALSE;

        // if (!err) {
        //    ERR(suites.IOOutSuite4()->AEGP_SetOutSpecOptionsHandle(outH, optionsH, reinterpret_cast<void**>(&old_optionsH)));
        //}
    }

    return err;
};

static A_Err	
AEIO_GetOutputInfo(
    AEIO_BasicData		*basic_dataP,
    AEIO_OutSpecH		outH,
    AEIO_Verbiage		*verbiageP)
{ 
    A_Err err			= A_Err_NONE;
    AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);

    auto codecName = CodecRegistry::codec()->details().fileFormatName;
    auto productName = CodecRegistry::codec()->details().productName;

    suites.ANSICallbacksSuite1()->strcpy(verbiageP->name, "filename");
    suites.ANSICallbacksSuite1()->strcpy(verbiageP->type, (std::string("MOV (") + codecName + ")").c_str());

    OutputOptionsUP optionsUP = OutputOptionsHandleWrapper::wrap(suites, outH);
    if (!optionsUP)
        return A_Err_PARAMETER;
    std::string qualityAsString = CodecRegistry::codec()->qualityDescriptions()[optionsUP->quality];

    suites.ANSICallbacksSuite1()->strcpy(verbiageP->sub_type,
                                         (productName + std::string("\rQuality setting: ") + qualityAsString).c_str());
    return err;
};

    
static A_Err	
AEIO_OutputInfoChanged(
    AEIO_BasicData		*basic_dataP,
    AEIO_OutSpecH		outH)
{
    /*	This function is called when either the user 
        or the plug-in has changed the output options info.
        You may want to update your plug-in's internal
        representation of the output at this time. 
        We've exercised the likely getters below.
    */
    
    A_Err err					=	A_Err_NONE;
    
    AEIO_AlphaLabel	alpha;
    AEFX_CLR_STRUCT(alpha);
    
    FIEL_Label		fields;
    AEFX_CLR_STRUCT(fields);

    A_short			depthS		=	0;
    A_Time			durationT	=	{0,1};

    A_Fixed			native_fps	=	0;
    A_Ratio			hsf			=	{1,1};
    A_Boolean		is_missingB	=	TRUE;
    
    AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);	

    ERR(suites.IOOutSuite4()->AEGP_GetOutSpecIsMissing(outH, &is_missingB));
    
    if (!is_missingB)
    {
        // Find out all the details of the output spec; update
        // your options data as necessary.
        
        ERR(suites.IOOutSuite4()->AEGP_GetOutSpecAlphaLabel(outH, &alpha));
        ERR(suites.IOOutSuite4()->AEGP_GetOutSpecDepth(outH, &depthS));
        ERR(suites.IOOutSuite4()->AEGP_GetOutSpecInterlaceLabel(outH, &fields));
        ERR(suites.IOOutSuite4()->AEGP_GetOutSpecDuration(outH, &durationT));
        ERR(suites.IOOutSuite4()->AEGP_GetOutSpecFPS(outH, &native_fps));
        ERR(suites.IOOutSuite4()->AEGP_GetOutSpecHSF(outH, &hsf));
    }
    return err;
}

static A_Err	
AEIO_SetOutputFile(
    AEIO_BasicData		*basic_dataP,
    AEIO_OutSpecH		outH, 
    const A_UTF16Char	*file_pathZ)
{ 
    return AEIO_Err_USE_DFLT_CALLBACK;
}

static A_Err	
AEIO_StartAdding(
    AEIO_BasicData		*basic_dataP,
    AEIO_OutSpecH		outH, 
    A_long				flags)
{ 
    A_Err				err			=	A_Err_NONE;
    A_Time				duration	=	{0,1};
    A_short				depth		=	0;
    A_Fixed				fps			=	0;
    AEIO_AlphaLabel     alpha;
    A_long				widthL 		= 	0,	
                        heightL 	= 	0;
    A_FpLong			soundRateF	=	0.0;
    AEIO_SndChannels    num_channels;
    AEIO_SndSampleSize  bytes_per_sample;
    AEIO_SndEncoding    snd_encoding;
    A_char				name[AEGP_MAX_PATH_SIZE] = {'\0'};
    AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);

    AEGP_ProjectH		projH		= 0;
    AEGP_TimeDisplay3	time_display;
    A_long				start_frameL	= 0;

    AEGP_MemHandle				file_pathH;
    A_Boolean					file_reservedB;
    A_UTF16Char					*file_pathZ;

    AEFX_CLR_STRUCT(time_display);

    ERR(suites.IOOutSuite4()->AEGP_GetOutSpecDuration(outH, &duration));
    ERR(suites.IOOutSuite4()->AEGP_GetOutSpecDimensions(outH, &widthL, &heightL));
    ERR(suites.IOOutSuite4()->AEGP_GetOutSpecDepth(outH, &depth));
    ERR(suites.IOOutSuite4()->AEGP_GetOutSpecSoundRate(outH, &soundRateF));
    ERR(suites.IOOutSuite4()->AEGP_GetOutSpecFPS(outH, &fps));
    ERR(suites.IOOutSuite4()->AEGP_GetOutSpecAlphaLabel(outH, &alpha));

    if (!err && soundRateF > 0) {
        ERR(suites.IOOutSuite4()->AEGP_GetOutSpecSoundChannels(outH, &num_channels));
        ERR(suites.IOOutSuite4()->AEGP_GetOutSpecSoundSampleSize(outH, &bytes_per_sample));
        ERR(suites.IOOutSuite4()->AEGP_GetOutSpecSoundEncoding(outH, &snd_encoding));

        // need to ensure encoding AEIO_E_UNSIGNED_PCM;
    }

    // Get timecode
    if (!err) {
        ERR(suites.ProjSuite6()->AEGP_GetProjectByIndex(0, &projH));
        ERR(suites.ProjSuite6()->AEGP_GetProjectTimeDisplay(projH, &time_display));

        ERR(suites.IOOutSuite4()->AEGP_GetOutSpecStartFrame(outH, &start_frameL));
    }

#if 0
    // Get ICC color profile information
    A_Boolean shouldEmbedICC(false);
    AEGP_ColorProfileP color_profileP(nullptr);
    std::unique_ptr<const AEGP_ColorProfileP, std::function<void(const AEGP_ColorProfileP *)> > color_profileUP;
    AEGP_MemHandle icc_profileH;
    std::unique_ptr<AEGP_MemHandle, std::function<void(AEGP_MemHandle *)> > icc_profileUP;
    
    std::vector<uint8_t> icc_color_profile;

    if (!err) {
        ERR(suites.IOOutSuite4()->AEGP_GetOutSpecShouldEmbedICCProfile(outH, &shouldEmbedICC));
        if (!err && shouldEmbedICC) {
            ERR(suites.IOOutSuite4()->AEGP_GetNewOutSpecColorProfile(basic_dataP->aegp_plug_id, outH, &color_profileP));
            if (!err) {
                color_profileUP = std::move(std::unique_ptr<const AEGP_ColorProfileP, std::function<void(const AEGP_ColorProfileP *)> >(
                    &color_profileP,
                    [&](const AEGP_ColorProfileP* c) { suites.ColorSettingsSuite2()->AEGP_DisposeColorProfile(*c); }));

                ERR(suites.ColorSettingsSuite2()->AEGP_GetNewICCProfileFromColorProfile(
                    basic_dataP->aegp_plug_id,
                    *color_profileUP,
                    &icc_profileH));
                icc_profileUP = std::move(std::unique_ptr<AEGP_MemHandle, std::function<void(AEGP_MemHandle *)> >(
                    &icc_profileH,
                    [&](const AEGP_MemHandle* h) { suites.MemorySuite1()->AEGP_FreeMemHandle(*h); }));


                AEGP_MemSize icc_profile_size;
                ERR(suites.MemorySuite1()->AEGP_GetMemHandleSize(*icc_profileUP, &icc_profile_size));
                uint8_t *ptr;
                ERR(suites.MemorySuite1()->AEGP_LockMemHandle(*icc_profileUP, (void **)&ptr));
                if (!err) {
                    icc_color_profile.resize(icc_profile_size);
                    memcpy(&icc_color_profile[0], ptr, icc_profile_size);
                    ERR(suites.MemorySuite1()->AEGP_UnlockMemHandle(*icc_profileUP));
                }
            }
        }
    }
#endif

    if (!err) {
        ERR(suites.IOOutSuite4()->AEGP_GetOutSpecFilePath(outH, &file_pathH, &file_reservedB));

        ERR(suites.MemorySuite1()->AEGP_LockMemHandle(file_pathH, reinterpret_cast<void**>(&file_pathZ)));
        // convert file path to utf8
        std::string filePath(SDKStringConvert::to_string(file_pathZ));
        ERR(suites.MemorySuite1()->AEGP_UnlockMemHandle(file_pathH));
        ERR(suites.MemorySuite1()->AEGP_FreeMemHandle(file_pathH));

        //
        OutputOptionsUP optionsUP = OutputOptionsHandleWrapper::wrap(suites, outH);
        if (!optionsUP)
            return A_Err_PARAMETER;

        try {
            CodecAlpha codecAlpha;

            //!!! this is irrelevant - AEX will deliver whatever it 'thinks best' later
            FrameFormat format(FrameOrigin_TopLeft | ChannelLayout_ARGB);
            switch (depth) {
            case 24:
                format |= ChannelFormat_U8;
                codecAlpha = withoutAlpha;
                break;
            case 32:
                format |= ChannelFormat_U8;
                codecAlpha = withAlpha;
                break;
            case 48:
                format |= ChannelFormat_U16_32k;
                codecAlpha = withoutAlpha;
                break;
            case 64:
                format |= ChannelFormat_U16_32k;
                codecAlpha = withAlpha;
                break;
            case 96:
                format |= ChannelFormat_F32;   // (?)
                codecAlpha = withoutAlpha;
                break;
            case 128:
                format |= ChannelFormat_F32;   // (?)
                codecAlpha = withAlpha;
                break;
            default:
                throw std::runtime_error("unsupported depth");
            }
            int clampedQuality = std::clamp(optionsUP->quality, 1, 5);  //!!! 4 is optimal; replace with enum
            int64_t frameRateNumerator = fps;
            int64_t frameRateDenominator = A_Fixed_ONE;
            int32_t maxFrames = (int32_t)((int64_t)duration.value * fps / A_Fixed_ONE / duration.scale);
            int reserveMetadataSpace = 0;
            auto movieErrorCallback = [](...) {};
            bool withAudio = (soundRateF > 0);
            int sampleRate = int(soundRateF);
            int numAudioChannels = num_channels;
            int audioBytesPerSample = bytes_per_sample;
            AudioEncoding audioEncoding = ((snd_encoding == AEIO_E_UNSIGNED_PCM) ? AudioEncoding_Unsigned_PCM : AudioEncoding_Signed_PCM);

            auto movieFile = createMovieFile(filePath, [](...) {});

            movieFile.onOpenForWrite();  //!!! move to writer

            const auto& codec = *CodecRegistry::codec();
            bool hasCodecSubType(codec.details().subtypes.size() > 0);
            CodecSubType subType = optionsUP->subType;

            HapChunkCounts chunkCounts{ static_cast<unsigned int>(optionsUP->chunkCount), static_cast<unsigned int>(optionsUP->chunkCount)};
            bool hasChunkCounts(codec.details().hasChunkCount);

            optionsUP->exporter = createExporter(
                FrameDef(widthL, heightL,
                         format),
                codecAlpha,
                hasCodecSubType, subType,
                hasChunkCounts, chunkCounts,
                clampedQuality,
                frameRateNumerator,
                frameRateDenominator,
                maxFrames,
                reserveMetadataSpace,
                movieFile,
                movieErrorCallback,
                withAudio,
                sampleRate,
                numAudioChannels,
                audioBytesPerSample,
                audioEncoding,
                false // writeMoovTagEarly
            );
        }
        catch (...)
        {
            return A_Err_PARAMETER;  //!!! AE generally hard crashes in event of errors
        }
    }
    return err; 
};

static A_Err	
AEIO_AddFrame(
    AEIO_BasicData			*basic_dataP,
    AEIO_OutSpecH			outH, 
    A_long					frame_index, 
    A_long					frames,
    const PF_EffectWorld	*wP, 
    const A_LPoint			*origin0,
    A_Boolean				was_compressedB,	
    AEIO_InterruptFuncs		*inter0)
{ 
    A_Err		err			= A_Err_NONE;
    A_Boolean	deep_worldB	= PF_WORLD_IS_DEEP(wP);
                #ifdef AE_OS_MAC
                #pragma unused (deep_worldB)
                #endif

    AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);

    OutputOptionsUP optionsUP = OutputOptionsHandleWrapper::wrap(suites, outH);
    if (!optionsUP)
        return A_Err_PARAMETER;

    {
        // Begin scope here to ensure 'suites' is valid for duration

        // PF_WorldSuite2 not supplied in AEGP_SuiteHandler (why?), but seems only way to query image format (why?)
        // AEIO isn't provided with a PF_InData so the documented accessors won't work.
        // AEGP_SuiteScoper needs a PF_InData (but not really)
        PF_InData hack;
        hack.pica_basicP = const_cast<SPBasicSuite*>(basic_dataP->pica_basicP);
        _PF_UtilCallbacks hack2;
        hack2.ansi.sprintf = suites.ANSICallbacksSuite1()->sprintf;
        hack.utils = &hack2;
        AEFX_SuiteScoper<PF_WorldSuite2> worldSuite2(&hack, kPFWorldSuite, kPFWorldSuiteVersion2);

        PF_PixelFormat pixelFormat;
        worldSuite2->PF_GetPixelFormat(wP, &pixelFormat);

        FrameFormat format(FrameOrigin_TopLeft | ChannelLayout_ARGB);
        switch (pixelFormat) {
        case PF_PixelFormat_ARGB32:
            format |= ChannelFormat_U8;
            break;
        case PF_PixelFormat_ARGB64:
            format |= ChannelFormat_U16_32k;
            break;
        case PF_PixelFormat_ARGB128:
            format |= ChannelFormat_F32;   // (?)
            break;
        default:
            throw std::runtime_error("unsupported depth");
        }

        char* rgba_buffer_tl = (char *)wP->data; //!!! PF_GET_PIXEL_DATA16(wP, nullptr, PF_Pixel16**(&bgra_buffer));
        if (!rgba_buffer_tl)
            return A_Err_PARAMETER; //  throw std::runtime_error("could not GetPixels on completed frame");
        auto rgba_stride = wP->rowbytes;

        try {
            for (auto iFrame = frame_index; iFrame < frame_index + frames; ++iFrame)
                optionsUP->exporter->dispatch(iFrame, (uint8_t*)rgba_buffer_tl, rgba_stride, format);
        }
        catch (...)
        {
            return A_Err_PARAMETER;  //!!! AE generally hard crashes in event of errors
        }
    }

    return err; 
};
                                
static A_Err	
AEIO_EndAdding(
    AEIO_BasicData	*basic_dataP,
    AEIO_OutSpecH			outH, 
    A_long					flags)
{ 
    AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);

    OutputOptionsUP optionsUP = OutputOptionsHandleWrapper::wrap(suites, outH);
    if (!optionsUP)
        return A_Err_PARAMETER;

    try
    {
        optionsUP->exporter->close();
        optionsUP->exporter.reset(nullptr);
    }
    catch (...)
    {
        return A_Err_PARAMETER;   //!!! AE generally hard crashes in event of errors
    }
    

    return A_Err_NONE;
};

static A_Err	
AEIO_OutputFrame(
    AEIO_BasicData			*basic_dataP,
    AEIO_OutSpecH			outH, 
    const PF_EffectWorld	*wP)
{ 
    A_Err err	=	A_Err_NONE;

    /*
        +	Re-interpret the supplied PF_World in your own
            format, and save it out to the outH's path.

    */
    return err;
};

static A_Err	
AEIO_WriteLabels(
    AEIO_BasicData	*basic_dataP,
    AEIO_OutSpecH	outH, 
    AEIO_LabelFlags	*written)
{ 
    return AEIO_Err_USE_DFLT_CALLBACK;
};

static A_Err	
AEIO_GetSizes(
    AEIO_BasicData	*basic_dataP,
    AEIO_OutSpecH	outH, 
    A_u_longlong	*free_space, 
    A_u_longlong	*file_size)
{ 
    return AEIO_Err_USE_DFLT_CALLBACK;
};

static A_Err	
AEIO_Flush(
    AEIO_BasicData	*basic_dataP,
    AEIO_OutSpecH	outH)
{ 
    /*	free any temp buffers you kept around for
        writing.
    */
    return A_Err_NONE; 
};

static A_Err	
AEIO_AddSoundChunk(
    AEIO_BasicData	*basic_dataP,
    AEIO_OutSpecH	outH, 
    const A_Time	*start, 
    A_u_long		num_samplesLu,
    const void		*dataPV)
{ 
    A_Err err		= A_Err_NONE, err2 = A_Err_NONE;
    AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);
    OutputOptionsUP optionsUP = OutputOptionsHandleWrapper::wrap(suites, outH);
    if (!optionsUP)
        return A_Err_PARAMETER;

    AEIO_SndChannels    num_channels;
    AEIO_SndSampleSize  bytes_per_sample;
    ERR(suites.IOOutSuite4()->AEGP_GetOutSpecSoundChannels(outH, &num_channels));
    ERR(suites.IOOutSuite4()->AEGP_GetOutSpecSoundSampleSize(outH, &bytes_per_sample));
    if (!err) {
        optionsUP->exporter->dispatch_audio_at_end(
            reinterpret_cast<const uint8_t *>(dataPV),
            num_channels * num_samplesLu * bytes_per_sample);
    }

    return err; 
};

static A_Err	
AEIO_Idle(
    AEIO_BasicData			*basic_dataP,
    AEIO_ModuleSignature	sig,
    AEIO_IdleFlags			*idle_flags0)
{ 
    return A_Err_NONE; 
};	


static A_Err	
AEIO_GetDepths(
    AEIO_BasicData			*basic_dataP,
    AEIO_OutSpecH			outH, 
    AEIO_SupportedDepthFlags		*which)
{ 
    /*	Enumerate possible output depths by OR-ing 
        together different AEIO_SupportedDepthFlags.
    */

    *which =
        AEIO_SupportedDepthFlags_DEPTH_24 |
        AEIO_SupportedDepthFlags_DEPTH_32 |
        AEIO_SupportedDepthFlags_DEPTH_48 |  // 16-bit without alpha
        AEIO_SupportedDepthFlags_DEPTH_64 |  // 16-bit with alpha
        AEIO_SupportedDepthFlags_DEPTH_96 |  // float without alpha (?)
        AEIO_SupportedDepthFlags_DEPTH_128;  // float with alpha (?)

    return A_Err_NONE; 
};

static A_Err	
AEIO_GetOutputSuffix(
    AEIO_BasicData	*basic_dataP,
    AEIO_OutSpecH	outH, 
    A_char			*suffix)
{ 
    return AEIO_Err_USE_DFLT_CALLBACK;
};


static A_Err	
AEIO_SetUserData(                
    AEIO_BasicData	*basic_dataP,
    AEIO_OutSpecH			outH,
    A_u_long				typeLu,
    A_u_long				indexLu,
    const AEIO_Handle		dataH)
{ 
    return A_Err_NONE; 
};

static A_Err
ConstructModuleInfo(
    SPBasicSuite		*pica_basicP,			
    AEIO_ModuleInfo		*info)
{
    A_Err err = A_Err_NONE;

    AEGP_SuiteHandler	suites(pica_basicP);

    const auto& codec = *CodecRegistry::codec();

    if (info) {
        info->sig						=	codec.details().afterEffectsSig;
        info->max_width					=	32768;
        info->max_height				=   32768;
        info->num_filetypes				=	1;
        info->num_extensions			=	1;
        info->num_clips					=	0;
        
        info->create_kind.type			=    codec.details().afterEffectsType;
        info->create_kind.creator		=	'DTEK';

        info->create_ext.pad			=	'.';
        info->create_ext.extension[0]	=	codec.details().videoFileExt[0];
        info->create_ext.extension[1]	=   codec.details().videoFileExt[1];
        info->create_ext.extension[2]	=   codec.details().videoFileExt[2];

        suites.ANSICallbacksSuite1()->strcpy(info->name, codec.details().fileFormatName.c_str());
        
        info->num_aux_extensionsS		=	0;

        info->flags						=	AEIO_MFlag_OUTPUT           | 
                                            AEIO_MFlag_FILE             |
                                            AEIO_MFlag_VIDEO            | 
                                            AEIO_MFlag_AUDIO            |
                                            AEIO_MFlag_CANT_SOUND_INTERLEAVE; //			|
                                            // AEIO_MFlag_NO_TIME;
                                            // AEIO_MFlag_CAN_DO_MARKERS	|
                                            // AEIO_MFlag_HAS_AUX_DATA;
#if 0
        info->flags2                    =   AEIO_MFlag2_SUPPORTS_ICC_PROFILES;
#endif

        info->read_kinds[0].mac.type			=	codec.details().afterEffectsMacType;
        info->read_kinds[0].mac.creator			=	AEIO_ANY_CREATOR;
        info->read_kinds[1].ext.pad				=	'.';
        info->read_kinds[1].ext.extension[0]	=   codec.details().videoFileExt[0];
        info->read_kinds[1].ext.extension[1]	=   codec.details().videoFileExt[1];
        info->read_kinds[1].ext.extension[2]	=   codec.details().videoFileExt[2];
    } else {
        err = A_Err_STRUCT;
    }
    return err;
}

A_Err
ConstructFunctionBlock(
    AEIO_FunctionBlock4	*funcs)
{
    if (funcs) {
        funcs->AEIO_AddFrame				=	AEIO_AddFrame;
        funcs->AEIO_AddSoundChunk			=	AEIO_AddSoundChunk;
        funcs->AEIO_DisposeOutputOptions	=	AEIO_DisposeOutputOptions;
        funcs->AEIO_EndAdding				=	AEIO_EndAdding;
        funcs->AEIO_Flush					=	AEIO_Flush;
        funcs->AEIO_GetDepths				=	AEIO_GetDepths;
        funcs->AEIO_GetOutputInfo			=	AEIO_GetOutputInfo;
        funcs->AEIO_GetOutputSuffix			=	AEIO_GetOutputSuffix;
        funcs->AEIO_GetSizes				=	AEIO_GetSizes;
        funcs->AEIO_Idle					=	AEIO_Idle;
        funcs->AEIO_OutputFrame				=	AEIO_OutputFrame;
        funcs->AEIO_SetOutputFile			=	AEIO_SetOutputFile;
        funcs->AEIO_SetUserData				=	AEIO_SetUserData;
        funcs->AEIO_StartAdding				=	AEIO_StartAdding;
        funcs->AEIO_UserOptionsDialog		=	AEIO_UserOptionsDialog;
        funcs->AEIO_WriteLabels				=	AEIO_WriteLabels;
        funcs->AEIO_InitOutputSpec			=	AEIO_InitOutputSpec;
        funcs->AEIO_GetFlatOutputOptions	=	AEIO_GetFlatOutputOptions;
        funcs->AEIO_OutputInfoChanged		=	AEIO_OutputInfoChanged;

        return A_Err_NONE;
    } else {
        return A_Err_STRUCT;
    }
}
A_Err
EntryPointFunc(
    struct SPBasicSuite		*pica_basicP,			/* >> */
    A_long				 	major_versionL,			/* >> */		
    A_long					minor_versionL,			/* >> */		
    AEGP_PluginID			aegp_plugin_id,			/* >> */
    AEGP_GlobalRefcon		*global_refconP)		/* << */
{
    A_Err 				err					= A_Err_NONE;
    AEIO_ModuleInfo		info;
    AEIO_FunctionBlock4	funcs;
    AEGP_SuiteHandler	suites(pica_basicP);	

    AEFX_CLR_STRUCT(info);
    AEFX_CLR_STRUCT(funcs);
    
    ERR(suites.RegisterSuite5()->AEGP_RegisterDeathHook(aegp_plugin_id, DeathHook, 0));
    ERR(ConstructModuleInfo(pica_basicP, &info));
    ERR(ConstructFunctionBlock(&funcs));

    ERR(suites.RegisterSuite5()->AEGP_RegisterIO(	aegp_plugin_id,
                                                    0,
                                                    &info, 
                                                    &funcs));

    ERR(suites.UtilitySuite3()->AEGP_RegisterWithAEGP(	NULL,
                                                       "NotchLC",
                                                       &S_mem_id));
    return err;
}
