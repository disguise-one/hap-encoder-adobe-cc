#include <new>

#include "codec_registration.hpp"
#include "importer.hpp"

//!!! #ifdef PRWIN_ENV
//!!! #include "SDK_Async_Import.h"
//!!! #endif

static prMALError 
ImporterInit(
    imStdParms        *stdParms, 
    imImportInfoRec    *importInfo)
{
    importInfo->setupOnDblClk        = kPrFalse;        // If user dbl-clicks file you imported, pop your setup dialog
    //!!! importInfo->canSave              = kPrTrue;        // Can 'save as' files to disk, real file only
    
    //!!!// imDeleteFile8 is broken on MacOS when renaming a file using the Save Captured Files dialog
    //!!!// So it is not recommended to set this on MacOS yet (bug 1627325)
    //!!!#ifdef PRWIN_ENV
    //!!!importInfo->canDelete            = kPrTrue;        // File importers only, use if you only if you have child files
    //!!!#endif
    
    importInfo->dontCache            = kPrFalse;       // Don't let Premiere cache these files
    //!!!importInfo->hasSetup             = kPrTrue;        // Set to kPrTrue if you have a setup dialog
    importInfo->keepLoaded           = kPrFalse;       // If you MUST stay loaded use, otherwise don't: play nice
    importInfo->priority             = 100;
    //!!! importInfo->canTrim              = kPrTrue;
    //!!! importInfo->canCalcSizes         = kPrTrue;
    if (stdParms->imInterfaceVer >= IMPORTMOD_VERSION_6)
    {
        importInfo->avoidAudioConform = kPrTrue;
    }                            

    return imIsCacheable;
}

static prMALError 
ImporterGetPrefs8(
    imStdParms          *stdParms, 
    imFileAccessRec8    *fileInfo8, 
    imGetPrefsRec       *prefsRec)
{
    ImporterLocalRec8   *ldata;

    // Note: if canOpen is not set to kPrTrue, I'm not getting this selector. Why?
    // Answer: because this selector is associated directly with "hasSetup"

    if(prefsRec->prefsLength == 0)
    {
        // first time we are called we will have to supply prefs data.
        // reterun size of the buffer to store prefs.
        prefsRec->prefsLength = sizeof(ImporterLocalRec8);
    }
    else
    {
        ldata = (ImporterLocalRec8 *)prefsRec->prefs;
        //!!! could copy things into prefsRec->
    }
    return malNoError;
}


prMALError
ImporterOpenFile8(
    imStdParms		*stdParms,
    imFileRef		*fileRef,
    imFileOpenRec8	*fileOpenRec8)
{
    prMALError			result = malNoError;
    ImporterLocalRec8H	localRecH;

    if (fileOpenRec8->privatedata)
    {
        localRecH = (ImporterLocalRec8H)fileOpenRec8->privatedata;
    }
    else
    {
        localRecH = (ImporterLocalRec8H)stdParms->piSuites->memFuncs->newHandle(sizeof(ImporterLocalRec8));
        fileOpenRec8->privatedata = (void*)localRecH;

        // Construct it
        new (*localRecH) ImporterLocalRec8;
        (*localRecH)->movieReader = nullptr;  //!!! do in ctor
    }

    // open the file
    try {
        HANDLE hFileRef = CreateFileW(fileOpenRec8->fileinfo.filepath,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);


        // Check to see if file is valid
        if (hFileRef == imInvalidHandleValue)
        {
            result = GetLastError();

            // Make sure the file is closed if returning imBadFile.
            // Otherwise, a lower priority importer will not be able to open it.
            result =
                imBadFile;
        }
        else
        {
            *fileRef = hFileRef;
            fileOpenRec8->fileinfo.fileref = fileRef;
            fileOpenRec8->fileinfo.filetype = 'nlc';
        }

        (*localRecH)->movieReader = std::make_unique<MovieReader>(
            CodecRegistry::codec().videoFormat(),
            [&, fileRef](const uint8_t* buffer, size_t size) {
                DWORD bytesReadLu;
                BOOL ok = ReadFile(*fileRef,
                    (LPVOID)buffer, (DWORD)size, 
                    &bytesReadLu,
                    NULL);
                if (!ok)
                    throw std::runtime_error("could not read");
                return bytesReadLu;
            },
            [&, fileRef](int64_t offset, int whence) {
                DWORD         dwMoveMethod;
                LARGE_INTEGER distanceToMove;
                distanceToMove.QuadPart = offset;

                if (whence == SEEK_SET)
                    dwMoveMethod = FILE_BEGIN;
                else if (whence == SEEK_END)
                    dwMoveMethod = FILE_END;
                else if (whence == SEEK_CUR)
                    dwMoveMethod = FILE_CURRENT;
                else
                    throw std::runtime_error("unhandled file seek mode");

                BOOL ok = SetFilePointerEx(
                    *fileRef,
                    distanceToMove,
                    NULL,
                    FILE_BEGIN);

                if (!ok)
                    throw std::runtime_error("could not read");

                return 0;
            },
            [&](const char *msg) {
                //!!! report here
            }
        );
    }
    catch (...)
    {
        result = imBadFile;
    }

    return result;
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
//	"Quiet" the file (it's being closed, but you maintain your Private data).  
//	
//	NOTE:	If you don't set any privateData, you will not get an imCloseFile call
//			so close it up here.

static prMALError
ImporterQuietFile(
    imStdParms			*stdParms,
    imFileRef			*fileRef,
    void				*privateData)
{
    // If file has not yet been closed
#ifdef PRWIN_ENV
    if (fileRef && *fileRef != imInvalidHandleValue)
    {
        ImporterLocalRec8H localRecH = (ImporterLocalRec8H)privateData;
        (*localRecH)->movieReader.reset(0);

        CloseHandle(*fileRef);
        *fileRef = imInvalidHandleValue;
    }
#else
    FSCloseFork(reinterpret_cast<intptr_t>(*SDKfileRef));
    *SDKfileRef = imInvalidHandleValue;
#endif

    return malNoError;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
//	Close the file.  You MUST have allocated Private data in imGetPrefs or you will not
//	receive this call.

static prMALError
ImporterCloseFile(
    imStdParms			*stdParms,
    imFileRef			*imFileRef,
    void				*privateData)
{
    ImporterLocalRec8H ldataH = reinterpret_cast<ImporterLocalRec8H>(privateData);

    // If file has not yet been closed
    if (imFileRef && *imFileRef != imInvalidHandleValue)
    {
        ImporterQuietFile(stdParms, imFileRef, privateData);
    }

    // Remove the privateData handle.
    // CLEANUP - Destroy the handle we created to avoid memory leaks
    if (ldataH && *ldataH && (*ldataH)->BasicSuite)
    {
        (*ldataH)->BasicSuite->ReleaseSuite(kPrSDKPPixCreatorSuite, kPrSDKPPixCreatorSuiteVersion);
        (*ldataH)->BasicSuite->ReleaseSuite(kPrSDKPPixCacheSuite, kPrSDKPPixCacheSuiteVersion);
        (*ldataH)->BasicSuite->ReleaseSuite(kPrSDKPPixSuite, kPrSDKPPixSuiteVersion);
        (*ldataH)->BasicSuite->ReleaseSuite(kPrSDKTimeSuite, kPrSDKTimeSuiteVersion);
        (*ldataH)->~ImporterLocalRec8();
        stdParms->piSuites->memFuncs->disposeHandle(reinterpret_cast<char**>(ldataH));
    }

    return malNoError;
}


static prMALError 
ImporterGetIndFormat(
    imStdParms     *stdParms, 
    csSDK_size_t    index, 
    imIndFormatRec *indFormatRec)
{
    prMALError    result        = malNoError;
    char formatname[255]    = "NotchLC Format";
    char shortname[32]      = "NotchLC";
    char platformXten[256]  = "mov\0\0";

    switch(index)
    {
        //    Add a case for each filetype.
        
    case 0:        
        
        indFormatRec->filetype = 'nlc'; //!!! CodecRegistry::codec().fileType();  //!!! 'MOOV';

            indFormatRec->canWriteTimecode    = kPrTrue;

            #ifdef PRWIN_ENV
            strcpy_s(indFormatRec->FormatName, sizeof (indFormatRec->FormatName), formatname);                 // The long name of the importer
            strcpy_s(indFormatRec->FormatShortName, sizeof (indFormatRec->FormatShortName), shortname);        // The short (menu name) of the importer
            strcpy_s(indFormatRec->PlatformExtension, sizeof (indFormatRec->PlatformExtension), platformXten); // The 3 letter extension
            #else
            strcpy(indFormatRec->FormatName, formatname);            // The Long name of the importer
            strcpy(indFormatRec->FormatShortName, shortname);        // The short (menu name) of the importer
            strcpy(indFormatRec->PlatformExtension, platformXten);   // The 3 letter extension
            #endif

            break;
        
    default:
        result = imBadFormatIndex;
    }
    return result;
}

#if 0
prMALError
GetInfoAudio(
    ImporterLocalRec8H    ldataH,
    imFileInfoRec8        *SDKFileInfo8)
{
    prMALError returnValue = malNoError;

    if((**ldataH).theFile.hasAudio)
    {
        SDKFileInfo8->hasAudio                = kPrTrue;

        // Importer API doesn't use channel-type enum from compiler API - need to map them
        if ((**ldataH).theFile.channelType == kPrAudioChannelType_Mono)
        {
            SDKFileInfo8->audInfo.numChannels = 1;
        }
        else if ((**ldataH).theFile.channelType == kPrAudioChannelType_Stereo)
        {
            SDKFileInfo8->audInfo.numChannels = 2;
        }
        else if ((**ldataH).theFile.channelType == kPrAudioChannelType_51)
        {
            SDKFileInfo8->audInfo.numChannels = 6;
        }
        else
        {
            returnValue = imBadFile;
        }

        SDKFileInfo8->audInfo.sampleRate    = (float)(**ldataH).theFile.sampleRate;
        // 32 bit float only for now
        SDKFileInfo8->audInfo.sampleType    = kPrAudioSampleType_32BitFloat;
        SDKFileInfo8->audDuration            = (**ldataH).theFile.numSampleFrames;

        #ifdef MULTISTREAM_AUDIO_TESTING
        if (!returnValue)
        {
            returnValue = MultiStreamAudioTesting(ldataH, SDKFileInfo8);
        }
        #endif
    }
    else
    {
        SDKFileInfo8->hasAudio = kPrFalse;
    }
    return returnValue;
}
#endif

/* 
    Populate the imFileInfoRec8 structure describing this file instance
    to Premiere.  Check file validity, allocate any private instance data 
    to share between different calls.
*/
prMALError
ImporterGetInfo8(
    imStdParms       *stdParms, 
    imFileAccessRec8 *fileAccessInfo8, 
    imFileInfoRec8   *fileInfo8)
{
    prMALError                    result                = malNoError;
    ImporterLocalRec8H            ldataH                = NULL;

    // If Premiere Pro 2.0 / Elements 2.0 or later, specify sequential audio so we can avoid
    // audio conforming.  Otherwise, specify random access audio so that we are not sent
    // imResetSequentialAudio and imGetSequentialAudio (which are not implemented in this sample)
    if (stdParms->imInterfaceVer >= IMPORTMOD_VERSION_6)
    {
        fileInfo8->accessModes = kSeparateSequentialAudio;
    }
    else
    {
        fileInfo8->accessModes = kRandomAccessImport;
    }

    //!!!#ifdef PRWIN_ENV
    //!!!fileInfo8->vidInfo.supportsAsyncIO            = kPrTrue;
    //!!!#elif defined PRMAC_ENV
    //!!!fileInfo8->vidInfo.supportsAsyncIO            = kPrFalse;
    //!!!#endif
    fileInfo8->vidInfo.supportsGetSourceVideo    = kPrTrue;
    fileInfo8->vidInfo.hasPulldown               = kPrFalse;
    fileInfo8->hasDataRate                       = kPrFalse; //!!! should be able to do thiskPrTrue;

    // Get a handle to our private data.  If it doesn't exist, allocate one
    // so we can use it to store our file instance info
    //!!!if(fileInfo8->privatedata)
    //!!!{

    //!!! presumably we went down a codepath that opened the file; otherwise none of the rest of the function would have made sense anyway
    ldataH = reinterpret_cast<ImporterLocalRec8H>(fileInfo8->privatedata);

    //!!!}
    //!!!else
    //!!!{
    //!!!    ldataH                 = reinterpret_cast<ImporterLocalRec8H>(stdParms->piSuites->memFuncs->newHandle(sizeof(ImporterLocalRec8)));
    //!!!    fileInfo8->privatedata = reinterpret_cast<void*>(ldataH);
    //!!!}

    // Either way, lock it in memory so it doesn't move while we modify it.
    stdParms->piSuites->memFuncs->lockHandle(reinterpret_cast<char**>(ldataH));

    // Acquire needed suites
    (*ldataH)->memFuncs = stdParms->piSuites->memFuncs;
    (*ldataH)->BasicSuite = stdParms->piSuites->utilFuncs->getSPBasicSuite();
    if ((*ldataH)->BasicSuite)
    {
        (*ldataH)->BasicSuite->AcquireSuite (kPrSDKPPixCreatorSuite, kPrSDKPPixCreatorSuiteVersion, (const void**)&(*ldataH)->PPixCreatorSuite);
        (*ldataH)->BasicSuite->AcquireSuite (kPrSDKPPixCacheSuite, kPrSDKPPixCacheSuiteVersion, (const void**)&(*ldataH)->PPixCacheSuite);
        (*ldataH)->BasicSuite->AcquireSuite (kPrSDKPPixSuite, kPrSDKPPixSuiteVersion, (const void**)&(*ldataH)->PPixSuite);
        (*ldataH)->BasicSuite->AcquireSuite (kPrSDKTimeSuite, kPrSDKTimeSuiteVersion, (const void**)&(*ldataH)->TimeSuite);
    }

    //!!! // Initialize persistent storage
    //!!! (*ldataH)->audioPosition = 0;

    // Get video info from header
    fileInfo8->hasVideo = kPrTrue;
    fileInfo8->vidInfo.subType     = (csSDK_int32 &)(CodecRegistry::codec().videoFormat());
    fileInfo8->vidInfo.imageWidth  = (*ldataH)->movieReader->width();
    fileInfo8->vidInfo.imageHeight = (*ldataH)->movieReader->height();
    fileInfo8->vidInfo.depth       = 32;  //!!! lack of alpha should change this
    fileInfo8->vidInfo.fieldType   = prFieldsNone;

    fileInfo8->vidInfo.alphaType = alphaNone;

    //!!!if (SDKFileInfo8->vidInfo.depth == 32)
    //!!!{
    //!!!    SDKFileInfo8->vidInfo.alphaType = alphaStraight;
    //!!!}
    //!!!else
    //!!!{
    //!!!    SDKFileInfo8->vidInfo.alphaType = alphaNone;
    //!!!}

    fileInfo8->vidInfo.pixelAspectNum = 1;
    fileInfo8->vidInfo.pixelAspectDen = 1;
    fileInfo8->vidScale               = (csSDK_int32)(*ldataH)->movieReader->frameRateNumerator();
    fileInfo8->vidSampleSize          = (csSDK_int32)(*ldataH)->movieReader->frameRateDenominator();
    fileInfo8->vidDuration            = (int32_t)((*ldataH)->movieReader->numFrames() * fileInfo8->vidSampleSize);

    //!!! // Get audio info from header
    //!!! result = GetInfoAudio(ldataH, fileInfo8);

    (*ldataH)->importerID = fileInfo8->vidInfo.importerID;

    stdParms->piSuites->memFuncs->unlockHandle(reinterpret_cast<char**>(ldataH));

    return result;
}

#if 0
static prMALError 
SDKPreferredFrameSize(
    imStdParms                    *stdparms, 
    imPreferredFrameSizeRec        *preferredFrameSizeRec)
{
    prMALError            result    = malNoError;
    ImporterLocalRec8H    ldataH    = reinterpret_cast<ImporterLocalRec8H>(preferredFrameSizeRec->inPrivateData);

    // Enumerate formats in order of priority, starting from the most preferred format
    switch(preferredFrameSizeRec->inIndex)
    {
        case 0:
            preferredFrameSizeRec->outWidth = (*ldataH)->theFile.width;
            preferredFrameSizeRec->outHeight = (*ldataH)->theFile.height;
            // If we supported more formats, we'd return imIterateFrameSizes to request to be called again
            result = malNoError;
            break;
    
        default:
            // We shouldn't be called for anything other than the case above
            result = imOtherErr;
    }

    return result;
}
#endif

static prMALError 
ImporterGetSourceVideo(
    imStdParms          *stdparms, 
    imFileRef            fileRef, 
    imSourceVideoRec    *sourceVideoRec)
{
    prMALError        result      = malNoError;
    csSDK_int32       theFrame    = 0,
                      rowBytes    = 0;
    imFrameFormat    *frameFormat;
    char             *frameBuffer;

    // Get the privateData handle you stored in imGetInfo
    ImporterLocalRec8H ldataH = reinterpret_cast<ImporterLocalRec8H>(sourceVideoRec->inPrivateData);

    PrTime ticksPerSecond = 0;
    (*ldataH)->TimeSuite->GetTicksPerSecond(&ticksPerSecond);
    double tFrame = (double)sourceVideoRec->inFrameTime / ticksPerSecond;
    theFrame = static_cast<csSDK_int32>(tFrame * (*ldataH)->movieReader->frameRateNumerator() / (*ldataH)->movieReader->frameRateDenominator());

    //!!! cache usually returns 'true', even for frames that haven't been added to it yet
    //!!!
    //!!! // Check to see if frame is already in cache
    //!!!
    //!!! result = (*ldataH)->PPixCacheSuite->GetFrameFromCache((*ldataH)->importerID,
    //!!!                                                         0,
    //!!!                                                         theFrame,
    //!!!                                                         1,
    //!!!                                                         sourceVideoRec->inFrameFormats,
    //!!!                                                         sourceVideoRec->outFrame,
    //!!!                                                         NULL,
    //!!!                                                         NULL);

    // If frame is not in the cache, read the frame and put it in the cache; otherwise, we're done
    //!!! if (result != suiteError_NoError)
    {
        // Get parameters for ReadFrameToBuffer()
        frameFormat = &sourceVideoRec->inFrameFormats[0];
        prRect theRect;
        if (frameFormat->inFrameWidth == 0 && frameFormat->inFrameHeight == 0)
        {
            frameFormat->inFrameWidth = (*ldataH)->movieReader->width();
            frameFormat->inFrameHeight = (*ldataH)->movieReader->height();
        }
        // Windows and MacOS have different definitions of Rects, so use the cross-platform prSetRect
        prSetRect (&theRect, 0, 0, frameFormat->inFrameWidth, frameFormat->inFrameHeight);
        (*ldataH)->PPixCreatorSuite->CreatePPix(sourceVideoRec->outFrame, PrPPixBufferAccess_ReadWrite, frameFormat->inPixelFormat, &theRect);
        (*ldataH)->PPixSuite->GetPixels(*sourceVideoRec->outFrame, PrPPixBufferAccess_ReadWrite, &frameBuffer);

        csSDK_int32 bytesPerFrame = frameFormat->inFrameWidth * frameFormat->inFrameHeight * 4;

        (*ldataH)->movieReader->readVideoFrame(theFrame, (*ldataH)->readBuffer);

        auto decoderParameters = std::make_unique<DecoderParametersBase>(
            FrameDef((*ldataH)->movieReader->width(), (*ldataH)->movieReader->height())
        );
        auto decoder = CodecRegistry::codec().createDecoder(std::move(decoderParameters));
        auto decoderJob = decoder->create();
        decoderJob->decode((*ldataH)->readBuffer);
        decoderJob->convert();

        uint8_t *bgraBottomLeftOrigin = (uint8_t *)frameBuffer;
        int32_t stride;

        // If extra row padding is needed, add it
        (*ldataH)->PPixSuite->GetRowBytes(*sourceVideoRec->outFrame, &stride);

        decoderJob->copyLocalToExternalToExternal(bgraBottomLeftOrigin, stride);

        (*ldataH)->PPixCacheSuite->AddFrameToCache((*ldataH)->importerID,
                                                    0,
                                                    *sourceVideoRec->outFrame,
                                                    theFrame,
                                                    NULL,
                                                    NULL);

        return malNoError;
    }

    //!!! return result;
}


PREMPLUGENTRY DllExport xImportEntry (
    csSDK_int32      selector,
    imStdParms      *stdParms, 
    void            *param1, 
    void            *param2)
{
    prMALError result = imUnsupported;

    switch (selector)
    {
        case imInit:
            result = ImporterInit(stdParms, 
                                  reinterpret_cast<imImportInfoRec*>(param1));
            break;

        // To be demonstrated
        // case imShutdown:

        case imGetPrefs8:
            result = ImporterGetPrefs8(stdParms, 
                                       reinterpret_cast<imFileAccessRec8*>(param1),
                                       reinterpret_cast<imGetPrefsRec*>(param2));
            break;

        // To be demonstrated
        // case imSetPrefs:

        case imGetInfo8:
            result = ImporterGetInfo8(stdParms,
                                      reinterpret_cast<imFileAccessRec8*>(param1), 
                                      reinterpret_cast<imFileInfoRec8*>(param2));
            break;

        case imImportAudio7:
//barf            result = ImporterImportAudio7(stdParms, 
//barf                                          reinterpret_cast<imFileRef>(param1),
//barf                                          reinterpret_cast<imImportAudioRec7*>(param2));
            result = imUnsupported;
            break;

        case imOpenFile8:
            result = ImporterOpenFile8(stdParms,
                reinterpret_cast<imFileRef*>(param1),
                reinterpret_cast<imFileOpenRec8*>(param2));
            break;

        case imQuietFile:
            result = ImporterQuietFile(stdParms,
                reinterpret_cast<imFileRef*>(param1),
                param2);
            break;

        case imCloseFile:
            result = ImporterCloseFile(stdParms,
                reinterpret_cast<imFileRef*>(param1),
                param2);
            break;

        case imGetTimeInfo8:
            result = imUnsupported;
            break;

        case imSetTimeInfo8:
            result = imUnsupported;
            break;

        case imAnalysis:
//barf            result = ImporterAnalysis(stdParms,
//barf                                      reinterpret_cast<imFileRef>(param1),
//barf                                      reinterpret_cast<imAnalysisRec*>(param2));
            result = imUnsupported;
            break;

        case imDataRateAnalysis:
//barf            result = ImporterDataRateAnalysis(stdParms,
//barf                                              reinterpret_cast<imFileRef>(param1),
//barf                                              reinterpret_cast<imDataRateAnalysisRec*>(param2));
            result = imUnsupported;
            break;

        case imGetIndFormat:
            result = ImporterGetIndFormat(stdParms, 
                                          reinterpret_cast<csSDK_size_t>(param1),
                                          reinterpret_cast<imIndFormatRec*>(param2));
            break;

        case imSaveFile8:
            result = imUnsupported;
            break;
            
        case imDeleteFile8:
            result = imUnsupported;
            break;

        case imGetMetaData:
            result = imUnsupported;
            break;

        case imSetMetaData:
            result = imUnsupported;
            break;

        case imGetIndPixelFormat:
//barf            result = ImporterGetIndPixelFormat(stdParms,
//barf                                               reinterpret_cast<csSDK_size_t>(param1),
//barf                                               reinterpret_cast<imIndPixelFormatRec*>(param2));
            result = imUnsupported;
            break;

        // Importers that support the Premiere Pro 2.0 API must return malSupports8 for this selector
        case imGetSupports8:
            result = malSupports8;
            break;

        case imCheckTrim8:
            result = imUnsupported;
            break;

        case imTrimFile8:
            result = imUnsupported;
            break;

        case imCalcSize8:
            result = imUnsupported;
            break;

        case imGetPreferredFrameSize:
//barf            result = ImporterPreferredFrameSize(stdParms,
//barf                                                reinterpret_cast<imPreferredFrameSizeRec*>(param1));
            result = imUnsupported;
            break;

        case imGetSourceVideo:
            result = ImporterGetSourceVideo(stdParms,
                                            reinterpret_cast<imFileRef>(param1),
                                            reinterpret_cast<imSourceVideoRec*>(param2));
            break;

        case imCreateAsyncImporter:
            //!!!#ifdef PRWIN_ENV
            //!!!result = ImporterCreateAsyncImporter(stdParms,
            //!!!                                    reinterpret_cast<imAsyncImporterCreationRec*>(param1));
            //!!!#else
            result =    imUnsupported;
            //!!!#endif
            break;
    }

    return result;
}

