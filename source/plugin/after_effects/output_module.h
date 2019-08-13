#include "AEConfig.h"

#include "entry.h"

#ifdef AE_OS_WIN
	#include <windows.h>
#endif

#include "AE_IO.h"
#include "AE_Macros.h"
#include "AE_EffectCBSuites.h"
#include "AEGP_SuiteHandler.h"
#include "AEFX_SuiteHandlerTemplate.h"

// This entry point is exported through the PiPL (.r file)
extern "C" DllExport AEGP_PluginInitFuncPrototype EntryPointFunc;

A_Err
ConstructFunctionBlock(
	AEIO_FunctionBlock4	*funcs);
