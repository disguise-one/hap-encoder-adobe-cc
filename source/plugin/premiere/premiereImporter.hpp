#ifndef PREMIERE_IMPORTER_HPP
#define PREMIERE_IMPORTER_HPP

#include <atomic>
#include <memory>
#include <string>
#include <list>

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
#include "importer.hpp"
#include "movie_reader.hpp"

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
        : filePath(filePath_)
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

#endif
