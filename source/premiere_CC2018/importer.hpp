#ifndef IMPORTER_HPP
#define IMPORTER_HPP

#include <memory>

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
#include "movie/movie_reader.hpp"

#ifdef		PRMAC_ENV
#include	<wchar.h>
#include    <Carbon/Carbon.h>
#endif

typedef struct
{	
	//!!! PrAudioSample audioPosition;
    std::unique_ptr<MovieReader> movieReader;
    DecodeInput                  readBuffer;

    PlugMemoryFuncsPtr     memFuncs;
    csSDK_int32            importerID;
    SPBasicSuite          *BasicSuite;
    PrSDKPPixCreatorSuite *PPixCreatorSuite;
    PrSDKPPixCacheSuite   *PPixCacheSuite;
    PrSDKPPixSuite        *PPixSuite;
    PrSDKTimeSuite        *TimeSuite;
} ImporterLocalRec8, *ImporterLocalRec8Ptr, **ImporterLocalRec8H;

// Declare plug-in entry point with C linkage
extern "C" {
PREMPLUGENTRY DllExport xImportEntry (csSDK_int32	selector, 
									  imStdParms	*stdParms, 
									  void			*param1, 
									  void			*param2);

}

#endif