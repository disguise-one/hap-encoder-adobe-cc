#include <codecvt>
#include <new>

#include "async_importer.hpp"
#include "codec_registration.hpp"
#include "importer.hpp"
#include "prstring.hpp"

using namespace std::chrono_literals;

// nuisance
std::string to_string(const std::wstring& fromUTF16)
{
    //setup converter
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;

    return converter.to_bytes(fromUTF16);
}

//!!! if importerID can be removed rename this to AdobeSuites
AdobeImporterAPI::AdobeImporterAPI(piSuitesPtr piSuites)
{
    memFuncs = piSuites->memFuncs;
    BasicSuite = piSuites->utilFuncs->getSPBasicSuite();
    if (BasicSuite)
    {
        BasicSuite->AcquireSuite(kPrSDKPPixCreatorSuite, kPrSDKPPixCreatorSuiteVersion, (const void**)&PPixCreatorSuite);
        BasicSuite->AcquireSuite(kPrSDKPPixCacheSuite, kPrSDKPPixCacheSuiteVersion, (const void**)&PPixCacheSuite);
        BasicSuite->AcquireSuite(kPrSDKPPixSuite, kPrSDKPPixSuiteVersion, (const void**)&PPixSuite);
        BasicSuite->AcquireSuite(kPrSDKTimeSuite, kPrSDKTimeSuiteVersion, (const void**)&TimeSuite);
    }
    else
        throw std::runtime_error("no BasicSuite available");
}

AdobeImporterAPI::~AdobeImporterAPI()
{
    BasicSuite->ReleaseSuite(kPrSDKPPixCreatorSuite, kPrSDKPPixCreatorSuiteVersion);
    BasicSuite->ReleaseSuite(kPrSDKPPixCacheSuite, kPrSDKPPixCacheSuiteVersion);
    BasicSuite->ReleaseSuite(kPrSDKPPixSuite, kPrSDKPPixSuiteVersion);
    BasicSuite->ReleaseSuite(kPrSDKTimeSuite, kPrSDKTimeSuiteVersion);
}

ImporterJobReader::ImporterJobReader(std::unique_ptr<MovieReader> reader)
    : reader_(std::move(reader)),
    utilisation_(1.),
    error_(false)
{
}

void ImporterJobReader::push(ImportJob job)
{
    std::lock_guard<std::mutex> guard(mutex_);
    queue_.push_back(std::move(job));
}

ImportJob ImporterJobReader::read()
{
    std::unique_lock<std::mutex> try_guard(mutex_, std::try_to_lock);

    if (try_guard.owns_lock())
    {
        try {
            // attempt to read in frame order
            auto earliest = std::min_element(queue_.begin(), queue_.end(),
                [](const auto& lhs, const auto& rhs) { return (*lhs).iFrame < (*rhs).iFrame; });
            if (earliest != queue_.end()) {
                ImportJob job = std::move(*earliest);
                queue_.erase(earliest);

                // start idle timer first time we try to read to avoid falsely including setup time
                if (idleStart_ == std::chrono::high_resolution_clock::time_point())
                    idleStart_ = std::chrono::high_resolution_clock::now();

                readStart_ = std::chrono::high_resolution_clock::now();
                
                try {
                    reader_->readVideoFrame(job->iFrame, job->input.buffer);
                    auto readEnd = std::chrono::high_resolution_clock::now();

                    // filtered update of utilisation_
                    if (readEnd != idleStart_)
                    {
                        auto totalTime = (readEnd - idleStart_).count();
                        auto readTime = (readEnd - readStart_).count();
                        const double alpha = 0.9;
                        utilisation_ = (1.0 - alpha) * utilisation_ + alpha * ((double)readTime / totalTime);
                    }
                    idleStart_ = readEnd;
                }
                catch (...)
                {
                    job->failed = true;
                }
                return job;
            }
        }
        catch (...) {
            error_ = true;
            throw;
        }
    }

    return nullptr;
}

ImporterJobDecoder::ImporterJobDecoder(Decoder& decoder)
    : decoder_(decoder), nDecodeJobs_(0)
{
}

void ImporterJobDecoder::push(ImportJob job)
{
    std::lock_guard<std::mutex> guard(mutex_);
    queue_.push_back(std::move(job));
    nDecodeJobs_++;
}

ImportJob ImporterJobDecoder::decode()
{
    ImportJob job;

    {
        std::lock_guard<std::mutex> guard(mutex_);
        auto earliest = std::min_element(queue_.begin(), queue_.end(),
            [](const auto& lhs, const auto& rhs) { return (*lhs).iFrame < (*rhs).iFrame; });
        if (earliest != queue_.end()) {
            job = std::move(*earliest);
            queue_.erase(earliest);
            nDecodeJobs_--;
        }
    }

    if (job && !job->failed)
    {
        try {
            job->codecJob->decode(job->input.buffer);
            job->codecJob->convert();
        }
        catch (...)
        {
            job->failed = true;
        }
    }

    return job;
}

void ImporterJobReader::close()
{
    reader_.reset(0);
}

ImporterWorker::ImporterWorker(std::atomic<bool>& error, ImporterJobFreeList& freeList, ImporterJobReader& reader, ImporterJobDecoder& decoder)
    : quit_(false), error_(error), jobFreeList_(freeList), jobReader_(reader), jobDecoder_(decoder)
{
    worker_ = std::thread(worker_start, std::ref(*this));
}

ImporterWorker::~ImporterWorker()
{
    quit_ = true;
    worker_.join();
}

// static public interface for std::thread
void ImporterWorker::worker_start(ImporterWorker& worker)
{
    worker.run();
}

// private
void ImporterWorker::run()
{
    while (!quit_)
    {
        try {
            ImportJob job = jobReader_.read();

            if (!job)
            {
                std::this_thread::sleep_for(2ms);
            }
            else
            {
                // submit it to be decoded (frame may be out of order)
                jobDecoder_.push(std::move(job));

                // dequeue any decodes
                do
                {
                    ImportJob decoded = jobDecoder_.decode();

                    if (decoded)
                    {
                        if (decoded->failed)
                        {
                            decoded->onFail(*decoded->codecJob);
                        }
                        else
                        {
                            decoded->onSuccess(*decoded->codecJob);
                        }
                        jobFreeList_.free(std::move(decoded));
                        break;
                    }

                    std::this_thread::sleep_for(1ms);
                } while (!error_);  // an error in a different thread will abort this thread's need to read
            }
        }
        catch (...)
        {
            //!!! should copy the exception and rethrow in main thread when it joins
            error_ = true;
            throw;
        }
    }
}



Importer::Importer(
    std::unique_ptr<MovieReader> movieReader,
    UniqueDecoder decoder)
    : closed_(false),
      decoder_(std::move(decoder)), jobDecoder_(*decoder_),
      jobFreeList_(std::function<ImportJob()>([&]() {
        return std::make_unique<ImportJob::element_type>(decoder_->create());
      })),
      jobReader_(std::move(movieReader)),
      error_(false)
{
    concurrentThreadsSupported_ = std::thread::hardware_concurrency() + 1;  // we assume at least 1 thread will be blocked by io read

    // assume 4 threads + 1 serialising will not tax the system too much before we figure out its limits
    size_t startingThreads = std::min(std::size_t{ 5 }, concurrentThreadsSupported_);
    for (size_t i = 0; i < startingThreads; ++i)
    {
        workers_.push_back(std::make_unique<ImporterWorker>(error_, jobFreeList_, jobReader_, jobDecoder_));
    }
}

Importer::~Importer()
{
    try
    {
        close();
    }
    catch (...)
    {
        //!!! not much we can do now;
        //!!! users should call 'close' themselves if they need to catch errors
    }
}

void Importer::close()
{
    if (!closed_)
    {
        // we don't want to retry closing on destruction if we throw an exception
        closed_ = true;

        // close the file.
        jobReader_.close();

        // wait for last jobs to complete. The last one does the last read. If something
        // fails it will abort the others.
        {
            ImportWorkers empty;  // this must be destructed before reader_.close()
            std::swap(workers_, empty);
        }

        if (error_)
            throw std::runtime_error("error on close");
    }
}

void Importer::requestFrame(int32_t iFrame,
                            std::function<void(const DecoderJob&)> onSuccess,
                            std::function<void(const DecoderJob&)> onFail)
{
    // throttle the caller - if the queue is getting too long we should wait
    while ((jobDecoder_.nDecodeJobs() >= workers_.size() - 1)
        && !expandWorkerPoolToCapacity()  // if we can, expand the pool
        && !error_)
    {
        // otherwise wait for an opening
        std::this_thread::sleep_for(2ms);
    }

    // worker threads can die while reading or encoding (eg dud file)
    // this is the most likely spot where the error can be noted by the main thread
    // TODO: should intercept and alert with the correct error reason
    if (error_)
        throw std::runtime_error("error while Importing");

    ImportJob job = jobFreeList_.allocate();

    job->iFrame = iFrame;
    job->onSuccess = onSuccess;
    job->onFail = onFail;
    job->failed = false;

    jobReader_.push(std::move(job));
}

// returns true if pool was expanded
bool Importer::expandWorkerPoolToCapacity() const
{
    bool isNotThreadLimited = workers_.size() < concurrentThreadsSupported_;
    bool isNotInputLimited = jobReader_.utilisation() < 0.99;
    bool isNotBufferLimited = true;  // TODO: get memoryUsed < maxMemoryCapacity from Adobe API

    if (isNotThreadLimited && isNotInputLimited && isNotBufferLimited) {
        workers_.push_back(std::make_unique<ImporterWorker>(error_, jobFreeList_, jobReader_, jobDecoder_));
        return true;
    }
    return false;
}






// helper to create movie reader wrapped around an imFileRef that the Adobe SDK
// wishes to manage
static std::pair<std::unique_ptr<MovieReader>, HANDLE> createMovieReader(const std::wstring& filePath)
{
    HANDLE fileRef = CreateFileW(filePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    // Check to see if file is valid
    if (fileRef == INVALID_HANDLE_VALUE)
    {
        auto error = GetLastError();

        throw std::runtime_error(std::string("could not open ")
                                 + to_string(filePath) + " - error " + std::to_string(error));
    }

    // fileSize is *only* needed by the seek wrapper for ffmpeg
    int64_t fileSize;
    LARGE_INTEGER ffs;
    if (GetFileSizeEx(fileRef, &ffs)) {
        fileSize = ffs.QuadPart;
    }
    else {
        fileSize = -1; // this is expected by seek wrapper as meaning "I can't do that"
    }

    return std::pair<std::unique_ptr<MovieReader>, HANDLE>(
        std::make_unique<MovieReader>(
            CodecRegistry::codec()->videoFormat(),
            fileSize,
            [&, fileRef](const uint8_t* buffer, size_t size) {
                DWORD bytesReadLu;
                BOOL ok = ReadFile(fileRef,
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
                    fileRef,
                    distanceToMove,
                    NULL,
                    FILE_BEGIN);

                if (!ok)
                    throw std::runtime_error("could not read");

                return 0;
            },
            [&](const char *msg) {
                //!!! report here
            },
            [fileRef]() {
                CloseHandle(fileRef);
                return 0;
            }
        ),
        fileRef);
}

static prMALError 
ImporterInit(
    imStdParms        *stdParms, 
    imImportInfoRec    *importInfo)
{
    importInfo->setupOnDblClk = kPrFalse;
    importInfo->canSave = kPrFalse;

    // imDeleteFile8 is broken on MacOS when renaming a file using the Save Captured Files dialog
    // So it is not recommended to set this on MacOS yet (bug 1627325)

    importInfo->canDelete = kPrFalse;
    importInfo->dontCache = kPrFalse;		// Don't let Premiere cache these files
    importInfo->hasSetup = kPrFalse;		// Set to kPrTrue if you have a setup dialog
    importInfo->keepLoaded = kPrFalse;		// If you MUST stay loaded use, otherwise don't: play nice
    importInfo->priority = 100;
    importInfo->canTrim = kPrFalse;
    importInfo->canCalcSizes = kPrFalse;
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
        new (*localRecH) ImporterLocalRec8(fileOpenRec8->fileinfo.filepath);
    }

    // open the file
    try {
        auto readerAndHandle = createMovieReader((*localRecH)->filePath);
        (*localRecH)->movieReader = std::move(readerAndHandle.first);
        *fileRef = readerAndHandle.second;

        FileFormat fileFormat = CodecRegistry::fileFormat();
        fileOpenRec8->fileinfo.filetype = reinterpret_cast<csSDK_int32&>(fileFormat);

        (*localRecH)->importerID = fileOpenRec8->inImporterID;

        FrameFormat format((CodecRegistry::isHighBitDepth() ? ChannelFormat_U16_32k : ChannelFormat_U8) // we're going to try to deliver something in keeping with the codec's high bit depth
            | FrameOrigin_BottomLeft
            | ChannelLayout_BGRA      // bgra
        );

        auto decoderParameters = std::make_unique<DecoderParametersBase>(
            FrameDef((*localRecH)->movieReader->width(), (*localRecH)->movieReader->height(),
                     format
                    )
            );
        (*localRecH)->decoder = CodecRegistry::codec()->createDecoder(std::move(decoderParameters));
        (*localRecH)->decoderJob = (*localRecH)->decoder->create();
    }
    catch (...)
    {
        result = imBadFile;
    }

    return result;
}


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
    ImporterLocalRec8H localRecH = (ImporterLocalRec8H)privateData;
    (*localRecH)->movieReader.reset(0);
    *fileRef = imInvalidHandleValue;

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

    // Remove the privateData handle.
    // CLEANUP - Destroy the handle we created to avoid memory leaks
    if (ldataH && *ldataH) //!!!  && (*ldataH)->BasicSuite)  either it was constructed, or its null.
    {
        // !!! only reason to call this is because it zeroes imFileRef
        ImporterQuietFile(stdParms, imFileRef, privateData);

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
    char platformXten[256]  = "MOV\0\0";

    switch(index)
    {
        //    Add a case for each filetype.
        
    case 0:
        FileFormat fileFormat = CodecRegistry::fileFormat();
        indFormatRec->filetype = reinterpret_cast<csSDK_int32&>(fileFormat);

        indFormatRec->canWriteTimecode    = kPrTrue;

        #ifdef PRWIN_ENV
        strcpy_s(indFormatRec->FormatName, sizeof (indFormatRec->FormatName), CodecRegistry::fileFormatName().c_str());                 // The long name of the importer
        strcpy_s(indFormatRec->FormatShortName, sizeof (indFormatRec->FormatShortName), CodecRegistry::fileFormatShortName().c_str());        // The short (menu name) of the importer
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

static prMALError
ImporterGetIndPixelFormat(
    imStdParms			*stdParms,
    csSDK_size_t		idx,
    imIndPixelFormatRec	*SDKIndPixelFormatRec)
{
    prMALError	result = malNoError;
    ImporterLocalRec8H	ldataH = reinterpret_cast<ImporterLocalRec8H>(SDKIndPixelFormatRec->privatedata);

    switch (idx)
    {
    case 0:
        SDKIndPixelFormatRec->outPixelFormat = CodecRegistry().isHighBitDepth()
            ? PrPixelFormat_BGRA_4444_16u // PrPixelFormat_BGRA_4444_32f
            : PrPixelFormat_BGRA_4444_8u;
        break;

    default:
        result = imBadFormatIndex;
        break;
    }
    return result;
}

// nuisance
static std::wstring to_wstring(const std::string& str)
{
    return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.from_bytes(str);
}

static prMALError
ImporterGetSubTypeNames(
    imStdParms     *stdParms,
    csSDK_int32 fileType,
    imSubTypeDescriptionRec **subTypeDescriptionRec)
{
    prMALError    result = malNoError;

    csSDK_int32 supportedFileFormat = reinterpret_cast<csSDK_int32&>(CodecRegistry::fileFormat());

    if (fileType = supportedFileFormat)
    {
        *subTypeDescriptionRec = (imSubTypeDescriptionRec *)stdParms->piSuites->memFuncs->newPtrClear(sizeof(imSubTypeDescriptionRec));

        csSDK_int32 videoFormat = reinterpret_cast<csSDK_int32&>(CodecRegistry::videoFormat());
        (*subTypeDescriptionRec)->subType = videoFormat;

        // should use the subtype format here, but we don't break out codec subtypes atm
        copyConvertStringLiteralIntoUTF16(to_wstring(CodecRegistry::fileFormatShortName()).c_str(),
                                          (*subTypeDescriptionRec)->subTypeName);
    }
    else
    {
        result = imBadFormatIndex;
    }
    return result;
}



prMALError
GetInfoAudio(
    ImporterLocalRec8H    ldataH,
    imFileInfoRec8        *SDKFileInfo8)
{
    prMALError returnValue = malNoError;

    if((*ldataH)->movieReader->hasAudio())
    {
        SDKFileInfo8->hasAudio                = kPrTrue;

        auto audioDef = (*ldataH)->movieReader->audioDef();

        SDKFileInfo8->audInfo.numChannels = audioDef.numChannels;
        SDKFileInfo8->audInfo.sampleRate = audioDef.sampleRate;
        std::map<std::pair<int, AudioEncoding>, PrAudioSampleType> bpsAndSignToPrAudioSampleType{
            {{1, AudioEncoding_Signed_PCM}, kPrAudioSampleType_8BitTwosInt},
            {{2, AudioEncoding_Signed_PCM}, kPrAudioSampleType_16BitInt},
            {{4, AudioEncoding_Signed_PCM}, kPrAudioSampleType_32BitInt},
            {{1, AudioEncoding_Unsigned_PCM}, kPrAudioSampleType_8BitInt},
            {{2, AudioEncoding_Unsigned_PCM}, kPrAudioSampleType_16BitInt},
            {{4, AudioEncoding_Unsigned_PCM}, kPrAudioSampleType_32BitInt}
        }; // these are used for display purposes

        SDKFileInfo8->audInfo.sampleType = bpsAndSignToPrAudioSampleType[{audioDef.bytesPerSample, audioDef.encoding}];
        SDKFileInfo8->audDuration = (*ldataH)->movieReader->numAudioFrames();
    }
    else
    {
        SDKFileInfo8->hasAudio = kPrFalse;
    }
    return returnValue;
}


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

    #ifdef PRWIN_ENV
    fileInfo8->vidInfo.supportsAsyncIO            = kPrTrue;
    #elif defined PRMAC_ENV
    fileInfo8->vidInfo.supportsAsyncIO            = kPrFalse;
    #endif
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
    (*ldataH)->adobe = std::make_unique<AdobeImporterAPI>(stdParms->piSuites);

    // Get video info from header
    fileInfo8->hasVideo = kPrTrue;
    fileInfo8->vidInfo.subType     = (csSDK_int32 &)(CodecRegistry::codec()->videoFormat());
    fileInfo8->vidInfo.imageWidth  = (*ldataH)->movieReader->width();
    fileInfo8->vidInfo.imageHeight = (*ldataH)->movieReader->height();
    fileInfo8->vidInfo.depth       = 32;  //!!! lack of alpha should change this
    fileInfo8->vidInfo.fieldType   = prFieldsNone;

    fileInfo8->vidInfo.alphaType = alphaStraight;

    fileInfo8->vidInfo.pixelAspectNum = 1;
    fileInfo8->vidInfo.pixelAspectDen = 1;
    fileInfo8->vidScale               = (csSDK_int32)(*ldataH)->movieReader->frameRateNumerator();
    fileInfo8->vidSampleSize          = (csSDK_int32)(*ldataH)->movieReader->frameRateDenominator();
    fileInfo8->vidDuration            = (int32_t)((*ldataH)->movieReader->numFrames() * fileInfo8->vidSampleSize);

    // Get audio info from header
    result = GetInfoAudio(ldataH, fileInfo8);

    stdParms->piSuites->memFuncs->unlockHandle(reinterpret_cast<char**>(ldataH));

    return result;
}

template<typename T>
static void deinterleave(const T* source, int nChannels, int nSamples, float **dest)
{
    double half_range = 0.5 * (std::numeric_limits<T>::max() - std::numeric_limits<T>::min());
    double scale = 1.0 / half_range;
    double offset = -scale * std::numeric_limits<T>::min() - 1;

    for (auto channel = 0; channel != nChannels; ++channel)
    {
        const T* s = &source[channel];
        float *d = dest[channel];
        for (int i = 0; i < nSamples; ++i) {
            *d = *s * scale + offset;
            s += nChannels;
            ++d;
        }
    }
}

static prMALError
ImporterImportAudio7(
    imStdParms *stdParms,
    imFileRef   fileRef,
    imImportAudioRec7 *importAudioRec7)
{
    prMALError result = malNoError;
    ImporterLocalRec8H ldataH = NULL;
    ldataH = reinterpret_cast<ImporterLocalRec8H>(importAudioRec7->privateData);
    
    // !!! near end we're asked to read beyond the duration we provided in GetInfoAudio :(
    // !!! don't fail in that case
    auto numFrames = (*ldataH)->movieReader->numAudioFrames();
    auto safePosition = std::min(importAudioRec7->position, numFrames);
    auto safeSize = std::min((int64_t)importAudioRec7->size, numFrames - safePosition);

    std::vector<uint8_t> buffer;
    (*ldataH)->movieReader->readAudio(safePosition, safeSize, buffer);

    auto audioDef = (*ldataH)->movieReader->audioDef();
    int numChannels = audioDef.numChannels;
    int sampleRate = audioDef.sampleRate;
    int bytesPerSample = audioDef.bytesPerSample;
    AudioEncoding encoding = audioDef.encoding;

    if ((bytesPerSample == 1) && (encoding == AudioEncoding_Signed_PCM)) {
        deinterleave((int8_t*)&buffer[0], numChannels, safeSize, importAudioRec7->buffer);
    }
    else if ((bytesPerSample == 2) && (encoding == AudioEncoding_Signed_PCM)) {
        deinterleave((int16_t*)&buffer[0], numChannels, safeSize, importAudioRec7->buffer);
    }
    else if ((bytesPerSample == 4) && (encoding == AudioEncoding_Signed_PCM)) {
        deinterleave((int32_t*)&buffer[0], numChannels, safeSize, importAudioRec7->buffer);
    }
    else if ((bytesPerSample == 1) && (encoding == AudioEncoding_Unsigned_PCM)) {
        deinterleave((uint8_t*)&buffer[0], numChannels, safeSize, importAudioRec7->buffer);
    }
    else if ((bytesPerSample == 2) && (encoding == AudioEncoding_Unsigned_PCM)) {
        deinterleave((uint16_t*)&buffer[0], numChannels, safeSize, importAudioRec7->buffer);
    }
    else if ((bytesPerSample == 4) && (encoding == AudioEncoding_Unsigned_PCM)) {
        deinterleave((uint32_t*)&buffer[0], numChannels, safeSize, importAudioRec7->buffer);
    }

    // !!! assign 0's for dud read
    if (importAudioRec7->size > safeSize)
    {
        for (size_t iChannel = 0; iChannel < numChannels; ++iChannel) {
            auto channel = importAudioRec7->buffer[iChannel];
            std::fill(channel + safeSize, channel + importAudioRec7->size, 0.0);
        }
    }
    // !!!

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
    imFrameFormat    *frameFormat;
    char             *frameBuffer;

    // Get the privateData handle you stored in imGetInfo
    ImporterLocalRec8H ldataH = reinterpret_cast<ImporterLocalRec8H>(sourceVideoRec->inPrivateData);

    PrTime ticksPerSecond = 0;
    (*ldataH)->adobe->TimeSuite->GetTicksPerSecond(&ticksPerSecond);
    int64_t num = sourceVideoRec->inFrameTime * (*ldataH)->movieReader->frameRateNumerator();
    int64_t den = ticksPerSecond * (*ldataH)->movieReader->frameRateDenominator();
    int32_t iFrame = (int32_t)(num / den);


    //!!! cache usually returns 'true', even for frames that haven't been added to it yet
    //!!! !!! this is probably due to the importerID being incorrect, as it was previously
    //!!! !!! being set GetInfo8 from uninitialised data; should probably be set at FileOpen
    //!!! !!!
    //!!!
    //!!! // Check to see if frame is already in cache
    //!!!
    //!!! result = (*ldataH)->PPixCacheSuite->GetFrameFromCache((*ldataH)->importerID,
    //!!!                                                         0,
    //!!!                                                         iFrame,
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
        prRect rect;
        if (frameFormat->inFrameWidth == 0 && frameFormat->inFrameHeight == 0)
        {
            frameFormat->inFrameWidth = (*ldataH)->movieReader->width();
            frameFormat->inFrameHeight = (*ldataH)->movieReader->height();
        }
        // Windows and MacOS have different definitions of Rects, so use the cross-platform prSetRect
        prSetRect (&rect, 0, 0, frameFormat->inFrameWidth, frameFormat->inFrameHeight);
        (*ldataH)->adobe->PPixCreatorSuite->CreatePPix(sourceVideoRec->outFrame, PrPPixBufferAccess_ReadWrite, frameFormat->inPixelFormat, &rect);
        (*ldataH)->adobe->PPixSuite->GetPixels(*sourceVideoRec->outFrame, PrPPixBufferAccess_ReadWrite, &frameBuffer);

        (*ldataH)->movieReader->readVideoFrame(iFrame, (*ldataH)->readBuffer);
        (*ldataH)->decoderJob->decode((*ldataH)->readBuffer);
        (*ldataH)->decoderJob->convert();

        uint8_t *bgraBottomLeftOrigin = (uint8_t *)frameBuffer;
        int32_t stride;

        // If extra row padding is needed, add it
        (*ldataH)->adobe->PPixSuite->GetRowBytes(*sourceVideoRec->outFrame, &stride);

        (*ldataH)->decoderJob->copyLocalToExternal(bgraBottomLeftOrigin, stride);

        //!!! (*ldataH)->PPixCacheSuite->AddFrameToCache((*ldataH)->importerID,
        //!!!                                             0,
        //!!!                                             *sourceVideoRec->outFrame,
        //!!!                                             iFrame,
        //!!!                                             NULL,
        //!!!                                            NULL);

        return malNoError;
    }

    //!!! return result;
}

static prMALError
ImporterCreateAsyncImporter(
    imStdParms					*stdparms,
    imAsyncImporterCreationRec	*asyncImporterCreationRec)
{
    prMALError		result = malNoError;

    // Set entry point for async importer
    asyncImporterCreationRec->outAsyncEntry = xAsyncImportEntry;

    // Create and initialize async importer
    // Deleted during aiClose
    auto ldata = (*reinterpret_cast<ImporterLocalRec8H>(asyncImporterCreationRec->inPrivateData));
    auto movieReaderAndFileRef = createMovieReader(ldata->filePath);
    auto movieReader = std::move(movieReaderAndFileRef.first);
    auto width = movieReader->width();
    auto height = movieReader->height();
    auto numerator = movieReader->frameRateNumerator();
    auto denominator = movieReader->frameRateDenominator();
  
    AsyncImporter *asyncImporter = new AsyncImporter(
        std::make_unique<AdobeImporterAPI>(stdparms->piSuites),
        std::make_unique<Importer>(std::move(movieReader), std::move(ldata->decoder)),
        width, height,
        numerator, denominator
    );

    // Store importer as private data
    asyncImporterCreationRec->outAsyncPrivateData = reinterpret_cast<void*>(asyncImporter);
    return result;
}

PREMPLUGENTRY DllExport xImportEntry (
    csSDK_int32      selector,
    imStdParms      *stdParms, 
    void            *param1, 
    void            *param2)
{
    prMALError result = imUnsupported;

    try {
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
            result = ImporterImportAudio7(stdParms,
                reinterpret_cast<imFileRef>(param1),
                reinterpret_cast<imImportAudioRec7*>(param2));
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

        case imGetSubTypeNames:
            result = ImporterGetSubTypeNames(stdParms,
                reinterpret_cast<csSDK_int32>(param1),
                reinterpret_cast<imSubTypeDescriptionRec**>(param2));
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
            result = ImporterGetIndPixelFormat(stdParms,
                reinterpret_cast<csSDK_size_t>(param1),
                reinterpret_cast<imIndPixelFormatRec*>(param2));
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
            result = ImporterCreateAsyncImporter(stdParms,
                reinterpret_cast<imAsyncImporterCreationRec*>(param1));
            break;
        }
    }
    catch (const std::exception& ex)
    {
        // !!! appears to be no way to provide an import error string
        OutputDebugString((CodecRegistry::codec()->logName() + " error - " + ex.what()).c_str());

        result = malUnknownError;
    }

    return result;
}

