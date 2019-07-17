#ifndef IMPORTER_HPP
#define IMPORTER_HPP

#include <atomic>
#include <memory>
#include <string>

#include	"PrSDKTypes.h"
#include	"PrSDKStructs.h"
#include	"PrSDKImport.h"
#include	"PrSDKErrorSuite.h"
#include	"PrSDKMALErrors.h"
#include	"PrSDKMarkerSuite.h"
#include	"PrSDKClipRenderSuite.h"
#include	"PrSDKPPixCreatorSuite.h"
#include	"PrSDKPPixCacheSuite.h"
#include	"PrSDKMemoryManagerSuite.h"
#include	"PrSDKWindowSuite.h"

#include "codec_registration.hpp"
#include "freelist.hpp"
#include "movie/movie_reader.hpp"

// !!! split into Adobe-specific and non-specific

struct AdobeImporterAPI
{
    AdobeImporterAPI(piSuitesPtr suites);
    ~AdobeImporterAPI();

    PlugMemoryFuncsPtr     memFuncs;
    SPBasicSuite          *BasicSuite;
    PrSDKPPixCreatorSuite *PPixCreatorSuite;
    PrSDKPPixCacheSuite   *PPixCacheSuite;
    PrSDKPPixSuite        *PPixSuite;
    PrSDKTimeSuite        *TimeSuite;
} ;



typedef struct ImporterLocalRec8
{
    ImporterLocalRec8(const std::wstring& filePath_)
        : importerID(-1), filePath(filePath_)
    {
    }

    std::wstring filePath;
	//!!! PrAudioSample audioPosition;
    std::unique_ptr<MovieReader>   movieReader;
    std::vector<uint8_t>           readBuffer;
    UniqueDecoder                  decoder;
    std::unique_ptr<DecoderJob>    decoderJob;

    std::unique_ptr<AdobeImporterAPI> adobe;               // adobe API, suites, importer ID
    csSDK_int32                       importerID{ -1 };    // provided on file open
} *ImporterLocalRec8Ptr, **ImporterLocalRec8H;

// Declare plug-in entry point with C linkage
extern "C" {
PREMPLUGENTRY DllExport xImportEntry (csSDK_int32	selector, 
									  imStdParms	*stdParms, 
									  void			*param1, 
									  void			*param2);
}



struct ImportJobImpl
{
    ImportJobImpl(std::unique_ptr<DecoderJob>& codecJob_) : codecJob(std::move(codecJob_)), iFrame(-1), failed(false) {}

    int32_t iFrame{ -1 };
    std::function<void(const DecoderJob&)> onSuccess;
    std::function<void(const DecoderJob&)> onFail;

    bool failed;  // mark as failed; forward through rest of pipeline without processing
    std::unique_ptr<DecoderJob> codecJob;
    DecodeInput input;
};

class ImporterWorker;

typedef std::unique_ptr<ImportJobImpl> ImportJob;  // either read or decode, depending on the queue its in
typedef std::vector<ImportJob> ImportJobQueue;
typedef std::list<std::unique_ptr<ImporterWorker> > ImportWorkers;

typedef FreeList<ImportJob> ImporterJobFreeList;

// thread-safe reader of ImportJob
class ImporterJobReader
{
public:
    ImporterJobReader(std::unique_ptr<MovieReader> reader);

    void close();  // call ahead of destruction in order to recognise errors

    void push(ImportJob job);
    ImportJob read();  // returns the job that was read

    double utilisation() const { return utilisation_; }

private:

    std::mutex mutex_;
    bool error_;
    ImportJobQueue queue_;
    std::unique_ptr<MovieReader> reader_;
    std::chrono::high_resolution_clock::time_point idleStart_;
    std::chrono::high_resolution_clock::time_point readStart_;

    std::atomic<double> utilisation_;
};

// thread-safe decoder of ImportJob
class ImporterJobDecoder
{
public:
    ImporterJobDecoder(Decoder& decoder);

    void push(ImportJob job);
    ImportJob decode();

    uint64_t nDecodeJobs() const { return nDecodeJobs_; }

private:
    Decoder& decoder_;  // must have thread-safe processing functions

    std::mutex mutex_;
    ImportJobQueue queue_;

    std::atomic<uint64_t> nDecodeJobs_;
};

class ImporterWorker
{
public:
    ImporterWorker(std::atomic<bool>& error, ImporterJobFreeList& freeList, ImporterJobReader& reader, ImporterJobDecoder& decoder);
    ~ImporterWorker();

    static void worker_start(ImporterWorker& worker);

private:
    std::thread worker_;
    void run();

    std::atomic<bool> quit_;
    std::atomic<bool>& error_;
    ImporterJobFreeList& jobFreeList_;
    ImporterJobDecoder& jobDecoder_;
    ImporterJobReader& jobReader_;
};

class Importer {
public:
    Importer(
        std::unique_ptr<MovieReader> reader,
        UniqueDecoder decoder
    );
    ~Importer();

    // users should call close if they wish to handle errors on shutdown - destructors cannot
    // throw, 'close' can and will
    void close();

    void requestFrame(
        int32_t iFrame,
        std::function<void(const DecoderJob&)> onSuccess,
        std::function<void(const DecoderJob&)> onFail);

private:
    bool closed_;
    bool expandWorkerPoolToCapacity() const;
    int concurrentThreadsSupported_;

    mutable std::atomic<bool> error_;
    UniqueDecoder decoder_;

    mutable ImporterJobFreeList jobFreeList_;
    mutable ImporterJobReader jobReader_;
    mutable ImporterJobDecoder jobDecoder_;

    // must be last to ensure they're joined before their dependents are destructed
    mutable ImportWorkers workers_;
};





#endif