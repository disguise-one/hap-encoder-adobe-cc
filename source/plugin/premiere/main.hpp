#pragma once

#include <PrSDKImport.h>
#include <PrSDKExport.h>
#include <PrSDKTypes.h>
#include <PrSDKMALErrors.h>

extern "C" {
	DllExport PREMPLUGENTRY xSDKExport(csSDK_int32 selector, exportStdParms *stdParms, void *param1, void *param2);
}

prMALError startup(exportStdParms *stdParms, exExporterInfoRec *infoRec);
prMALError beginInstance(exportStdParms *stdParmsP, exExporterInstanceRec *instanceRecP);
prMALError endInstance(exportStdParms *stdParmsP, exExporterInstanceRec *instanceRecP);
prMALError queryOutputSettings(exportStdParms *stdParmsP, exQueryOutputSettingsRec *outputSettingsP);
prMALError fileExtension(exportStdParms *stdParmsP, exQueryExportFileExtensionRec *exportFileExtensionRecP);
prMALError doExport(exportStdParms *stdParms, exDoExportRec *exportInfoP);
