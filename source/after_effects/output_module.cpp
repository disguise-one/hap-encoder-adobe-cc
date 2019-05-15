#include <codecvt>
#include <new>

#include "../premiere_CC2018/exporter/exporter.hpp"
#include "codec_registration.hpp"

#include "output_module.h"

static AEGP_PluginID S_mem_id = 0;

static A_Err DeathHook(	
    AEGP_GlobalRefcon unused1 ,
    AEGP_DeathRefcon unused2)
{
    CodecRegistry::codec().reset();

    return A_Err_NONE;
}

// nuisance
std::string to_string(const std::wstring& fromUTF16)
{
    //setup converter
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;

    return converter.to_bytes(fromUTF16);
}

struct OutputOptions
{
    OutputOptions() : width(1920), height(1080), quality(1) {}
    ~OutputOptions() {}

    int width;
    int height;
    int quality;

    std::unique_ptr<Exporter> exporter;
};


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
        *file = fopen(filename.c_str(), "wb");
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
        auto result = _fseeki64(*file, offset, whence);
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
    const FrameDef& frameDef, CodecAlpha alpha, int quality,
    int64_t frameRateNumerator, int64_t frameRateDenominator,
    int64_t maxFrames, int32_t reserveMetadataSpace,
    const MovieFile& file, MovieErrorCallback errorCallback,
    bool withAudio, int sampleRate, int32_t numAudioChannels,
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
        writer->addAudioStream(numAudioChannels, sampleRate);
    }

    writer->writeHeader();

#if 0
    if (withAudio)
        renderAndWriteAllAudio(exportInfoP, error, writer.get());
#endif

    return std::make_unique<Exporter>(std::move(encoder), std::move(writer));
}

static A_Err	
My_InitOutputSpec(
    AEIO_BasicData			*basic_dataP,
    AEIO_OutSpecH			outH, 
    A_Boolean				*user_interacted)
{
    A_Err						err				= A_Err_NONE;
    AEIO_Handle					new_optionsH	= NULL, 
                                old_optionsH	= 0;
    OutputOptions	*new_optionsP,
                    *old_optionsP;
    AEGP_SuiteHandler			suites(basic_dataP->pica_basicP);

    ERR(suites.IOOutSuite4()->AEGP_GetOutSpecOptionsHandle(outH, reinterpret_cast<void**>(&old_optionsH)));

    if (!err) {
        ERR(suites.MemorySuite1()->AEGP_NewMemHandle(	S_mem_id, 
                                                       "InitOutputSpec options", 
                                                       sizeof(OutputOptions),
                                                       AEGP_MemFlag_CLEAR, 
                                                       &new_optionsH));
        if (!err && new_optionsH) {
            ERR(suites.MemorySuite1()->AEGP_LockMemHandle(new_optionsH, reinterpret_cast<void**>(&new_optionsP)));

            if (!err && new_optionsP) {

                if (!old_optionsH) {
                    new (new_optionsP) OutputOptions;
                } else {
                    ERR(suites.MemorySuite1()->AEGP_LockMemHandle(old_optionsH, reinterpret_cast<void**>(&old_optionsP)));

                    if (!err && new_optionsP && old_optionsP) {
                        memcpy(new_optionsP, old_optionsP, sizeof(OutputOptions));

                        *user_interacted = FALSE;  // output options have changed


                        ERR(suites.MemorySuite1()->AEGP_UnlockMemHandle(old_optionsH));
                    }
                }
                ERR(suites.MemorySuite1()->AEGP_UnlockMemHandle(new_optionsH));

                ERR(suites.IOOutSuite4()->AEGP_SetOutSpecOptionsHandle(outH, new_optionsH, reinterpret_cast<void**>(&old_optionsH)));
            }
        }
    }
    if (old_optionsH){
        ERR(suites.MemorySuite1()->AEGP_FreeMemHandle(old_optionsH));
    }
    return err;
}

static A_Err	
My_GetFlatOutputOptions(
    AEIO_BasicData	*basic_dataP,
    AEIO_OutSpecH	outH, 
    AEIO_Handle		*new_optionsPH)
{
    A_Err						err				= A_Err_NONE;
    AEIO_Handle					old_optionsH	= NULL;
    OutputOptions *new_optionsP;
    AEGP_SuiteHandler			suites(basic_dataP->pica_basicP);

    OutputOptionsUP old_optionsUP = OutputOptionsHandleWrapper::wrap(suites, outH);
    if (!old_optionsUP)
        return A_Err_PARAMETER;

    ERR(suites.MemorySuite1()->AEGP_NewMemHandle( S_mem_id, 
                                                    "flat optionsH", 
                                                    sizeof(OutputOptions),
                                                    AEGP_MemFlag_CLEAR, 
                                                    new_optionsPH));
    if (!err && *new_optionsPH) {
        ERR(suites.MemorySuite1()->AEGP_LockMemHandle(*new_optionsPH, reinterpret_cast<void**>(&new_optionsP)));

        if (!err && new_optionsP && old_optionsUP) {
            // Convert the old unflat structure into a separate flat structure for output
            // In this case, we just do a simple copy
            memcpy(new_optionsP, old_optionsUP.get(), sizeof(OutputOptions));

            ERR(suites.MemorySuite1()->AEGP_UnlockMemHandle(*new_optionsPH));
        }
    }

    return err;
}

static A_Err	
My_DisposeOutputOptions(
    AEIO_BasicData	*basic_dataP,
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
My_UserOptionsDialog(
    AEIO_BasicData			*basic_dataP,
    AEIO_OutSpecH			outH, 
    const PF_EffectWorld	*sample0,
    A_Boolean				*user_interacted0)
{ 
    A_Err						err				= A_Err_NONE;
    AEGP_SuiteHandler			suites(basic_dataP->pica_basicP);
    AEIO_Handle					optionsH		= NULL, 
                                old_optionsH	= 0;
    OutputOptions *optionsP;

    ERR(suites.IOOutSuite4()->AEGP_GetOutSpecOptionsHandle(outH, reinterpret_cast<void**>(&optionsH)));
    if (!err){
        ERR(suites.MemorySuite1()->AEGP_LockMemHandle(optionsH, reinterpret_cast<void**>(&optionsP)));

        basic_dataP->msg_func(0, "todo: quality option here");

        ERR(suites.MemorySuite1()->AEGP_UnlockMemHandle(optionsH));

        ERR(suites.IOOutSuite4()->AEGP_SetOutSpecOptionsHandle(outH, optionsH, reinterpret_cast<void**>(&old_optionsH)));
    }

    return err;
};

static A_Err	
My_GetOutputInfo(
    AEIO_BasicData		*basic_dataP,
    AEIO_OutSpecH		outH,
    AEIO_Verbiage		*verbiageP)
{ 
    A_Err err			= A_Err_NONE;
    AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);

    suites.ANSICallbacksSuite1()->strcpy(verbiageP->name, "filename");
    suites.ANSICallbacksSuite1()->strcpy(verbiageP->type, "MOV (NotchLC)");
    suites.ANSICallbacksSuite1()->strcpy(verbiageP->sub_type, "No codecs supported in this sample");
    return err;
};

    
static A_Err	
My_OutputInfoChanged(
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
My_SetOutputFile(
    AEIO_BasicData		*basic_dataP,
    AEIO_OutSpecH		outH, 
    const A_UTF16Char	*file_pathZ)
{ 
    return AEIO_Err_USE_DFLT_CALLBACK;
}

static A_Err	
My_StartAdding(
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
    A_char				name[AEGP_MAX_PATH_SIZE] = {'\0'};
    AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);

    AEGP_ProjectH		my_projH		= 0;
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

#if 0
    // If video
    if (!err && name && widthL && heightL) {
        header.hasVideo		=	TRUE;

        if (depth > 32){
            header.rowbytesLu	=	(unsigned long)(8 * widthL);
        } else {
            header.rowbytesLu	=	(unsigned long)(4 * widthL);
        }
    }

    if (!err && soundRateF > 0) {
        header.hasAudio		=	TRUE;
        header.rateF		=	soundRateF;
        ERR(suites.IOOutSuite4()->AEGP_GetOutSpecSoundChannels(outH, &header.num_channels));
        ERR(suites.IOOutSuite4()->AEGP_GetOutSpecSoundSampleSize(outH, &header.bytes_per_sample));

        header.encoding		=	AEIO_E_UNSIGNED_PCM;
    }
#endif

    // Get timecode
    if (!err) {
        ERR(suites.ProjSuite6()->AEGP_GetProjectByIndex(0, &my_projH));
        ERR(suites.ProjSuite6()->AEGP_GetProjectTimeDisplay(my_projH, &time_display));

        ERR(suites.IOOutSuite4()->AEGP_GetOutSpecStartFrame(outH, &start_frameL));
    }

    if (!err) {
        ERR(suites.IOOutSuite4()->AEGP_GetOutSpecFilePath(outH, &file_pathH, &file_reservedB));

        ERR(suites.MemorySuite1()->AEGP_LockMemHandle(file_pathH, reinterpret_cast<void**>(&file_pathZ)));
        // convert file path to utf8
        std::string filePath(to_string((wchar_t *)file_pathZ));
        ERR(suites.MemorySuite1()->AEGP_UnlockMemHandle(file_pathH));
        ERR(suites.MemorySuite1()->AEGP_FreeMemHandle(file_pathH));

        //
        OutputOptionsUP optionsUP = OutputOptionsHandleWrapper::wrap(suites, outH);
        if (!optionsUP)
            return A_Err_PARAMETER;

        try {
            bool withAlpha = (alpha.alpha != AEIO_Alpha_NONE);
            int clampedQuality = std::clamp(4, 1, 5);  //!!! 4 is optimal; replace with enum
            int64_t frameRateNumerator = fps;
            int64_t frameRateDenominator = A_Fixed_ONE;
            int64_t maxFrames = (int64_t)duration.value * fps / A_Fixed_ONE / duration.scale;
            int64_t reserveMetadataSpace = 0;
            auto movieErrorCallback = [](...) {};
            bool withAudio = false;
            int64_t sampleRate = 0;
            int numAudioChannels = 0;

            auto movieFile = createMovieFile(filePath, [](...) {});

            movieFile.onOpenForWrite();  //!!! move to writer

            optionsUP->exporter = createExporter(
                FrameDef(widthL, heightL, true, true, false),
                withAlpha ? CodecAlpha::withAlpha : CodecAlpha::withoutAlpha,
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
My_AddFrame(
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

    char* rgba_buffer_tl = (char *)wP->data; //!!! PF_GET_PIXEL_DATA16(wP, nullptr, PF_Pixel16**(&bgra_buffer));
    int32_t rgba_stride = wP->rowbytes;
    if (!rgba_buffer_tl)
        return A_Err_PARAMETER; //  throw std::runtime_error("could not GetPixels on completed frame");

    try {
        for (auto iFrame = frame_index; iFrame < frame_index + frames; ++iFrame)
            optionsUP->exporter->dispatch(iFrame, (uint8_t*)rgba_buffer_tl, rgba_stride);
    }
    catch (...)
    {
        return A_Err_PARAMETER;  //!!! AE generally hard crashes in event of errors
    }

    return err; 
};
                                
static A_Err	
My_EndAdding(
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
My_OutputFrame(
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
My_WriteLabels(
    AEIO_BasicData	*basic_dataP,
    AEIO_OutSpecH	outH, 
    AEIO_LabelFlags	*written)
{ 
    return AEIO_Err_USE_DFLT_CALLBACK;
};

static A_Err	
My_GetSizes(
    AEIO_BasicData	*basic_dataP,
    AEIO_OutSpecH	outH, 
    A_u_longlong	*free_space, 
    A_u_longlong	*file_size)
{ 
    return AEIO_Err_USE_DFLT_CALLBACK;
};

static A_Err	
My_Flush(
    AEIO_BasicData	*basic_dataP,
    AEIO_OutSpecH	outH)
{ 
    /*	free any temp buffers you kept around for
        writing.
    */
    return A_Err_NONE; 
};

#if 0
static A_Err	
My_AddSoundChunk(
    AEIO_BasicData	*basic_dataP,
    AEIO_OutSpecH	outH, 
    const A_Time	*start, 
    A_u_long		num_samplesLu,
    const void		*dataPV)
{ 
    A_Err err		= A_Err_NONE, err2 = A_Err_NONE;
    AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);
    
    AEIO_Handle					optionsH = NULL;
    IO_FlatFileOutputOptions	*optionsP = NULL;
    
    ERR(suites.IOOutSuite4()->AEGP_GetOutSpecOptionsHandle(outH, reinterpret_cast<void**>(&optionsH)));

    if (!err && optionsH) {
        ERR(suites.MemorySuite1()->AEGP_LockMemHandle(optionsH, reinterpret_cast<void**>(&optionsP)));
        if (!err) {
            A_char report[AEGP_MAX_ABOUT_STRING_SIZE] = {'\0'};
            suites.ANSICallbacksSuite1()->sprintf(report, "NotchLC : Pretended to write %d samples of audio requested.", num_samplesLu); 
            ERR(suites.UtilitySuite3()->AEGP_ReportInfo(	basic_dataP->aegp_plug_id, report));
        }
    }
    ERR2(suites.MemorySuite1()->AEGP_UnlockMemHandle(optionsH));
    return err; 
};
#endif

static A_Err	
My_Idle(
    AEIO_BasicData			*basic_dataP,
    AEIO_ModuleSignature	sig,
    AEIO_IdleFlags			*idle_flags0)
{ 
    return A_Err_NONE; 
};	


static A_Err	
My_GetDepths(
    AEIO_BasicData			*basic_dataP,
    AEIO_OutSpecH			outH, 
    AEIO_SupportedDepthFlags		*which)
{ 
    /*	Enumerate possible output depths by OR-ing 
        together different AEIO_SupportedDepthFlags.
    */
    
    *which =	AEIO_SupportedDepthFlags_DEPTH_64;		// 16-bit with alpha

    return A_Err_NONE; 
};

static A_Err	
My_GetOutputSuffix(
    AEIO_BasicData	*basic_dataP,
    AEIO_OutSpecH	outH, 
    A_char			*suffix)
{ 
    return AEIO_Err_USE_DFLT_CALLBACK;
};


static A_Err	
My_SetUserData(                
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
    
    if (info) {
        info->sig						=	'NLC_';
        info->max_width					=	32768;
        info->max_height				=   32768;
        info->num_filetypes				=	1;
        info->num_extensions			=	1;
        info->num_clips					=	0;
        
        info->create_kind.type			=	'NLC_';
        info->create_kind.creator		=	'DTEK';

        info->create_ext.pad			=	'.';
        info->create_ext.extension[0]	=	'm';
        info->create_ext.extension[1]	=	'o';
        info->create_ext.extension[2]	=	'v';

        suites.ANSICallbacksSuite1()->strcpy(info->name, "NotchLC");
        
        info->num_aux_extensionsS		=	0;

        info->flags						=	AEIO_MFlag_OUTPUT			| 
                                            AEIO_MFlag_FILE				|
                                            AEIO_MFlag_VIDEO; //			| 
                                            // AEIO_MFlag_AUDIO			|
                                            // AEIO_MFlag_NO_TIME;
                                            // AEIO_MFlag_CAN_DO_MARKERS	|
                                            // AEIO_MFlag_HAS_AUX_DATA;

        info->read_kinds[0].mac.type			=	'NLC_';
        info->read_kinds[0].mac.creator			=	AEIO_ANY_CREATOR;
        info->read_kinds[1].ext.pad				=	'.';
        info->read_kinds[1].ext.extension[0]	=	'm';
        info->read_kinds[1].ext.extension[1]	=	'o';
        info->read_kinds[1].ext.extension[2]	=	'v';
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
        funcs->AEIO_AddFrame				=	My_AddFrame;
        // !!! funcs->AEIO_AddSoundChunk			=	My_AddSoundChunk;
        funcs->AEIO_DisposeOutputOptions	=	My_DisposeOutputOptions;
        funcs->AEIO_EndAdding				=	My_EndAdding;
        funcs->AEIO_Flush					=	My_Flush;
        funcs->AEIO_GetDepths				=	My_GetDepths;
        funcs->AEIO_GetOutputInfo			=	My_GetOutputInfo;
        funcs->AEIO_GetOutputSuffix			=	My_GetOutputSuffix;
        funcs->AEIO_GetSizes				=	My_GetSizes;
        funcs->AEIO_Idle					=	My_Idle;
        funcs->AEIO_OutputFrame				=	My_OutputFrame;
        funcs->AEIO_SetOutputFile			=	My_SetOutputFile;
        funcs->AEIO_SetUserData				=	My_SetUserData;
        funcs->AEIO_StartAdding				=	My_StartAdding;
        funcs->AEIO_UserOptionsDialog		=	My_UserOptionsDialog;
        funcs->AEIO_WriteLabels				=	My_WriteLabels;
        funcs->AEIO_InitOutputSpec			=	My_InitOutputSpec;
        funcs->AEIO_GetFlatOutputOptions	=	My_GetFlatOutputOptions;
        funcs->AEIO_OutputInfoChanged		=	My_OutputInfoChanged;

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
