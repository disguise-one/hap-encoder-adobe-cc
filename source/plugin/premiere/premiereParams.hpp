#pragma once

#include <PrSDKExport.h>
#include <PrSDKMALErrors.h>

prMALError generateDefaultParams(exportStdParms *stdParms, exGenerateDefaultParamRec *generateDefaultParamRec);
prMALError postProcessParams(exportStdParms *stdParmsP, exPostProcessParamsRec *postProcessParamsRecP);
prMALError getParamSummary(exportStdParms *stdParmsP, exParamSummaryRec *summaryRecP);
prMALError paramButton(exportStdParms *stdParmsP, exParamButtonRec *getFilePrefsRecP);
prMALError validateParamChanged(exportStdParms *stdParmsP, exParamChangedRec *validateParamChangedRecP);
