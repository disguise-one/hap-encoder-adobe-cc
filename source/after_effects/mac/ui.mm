#include "../ui.h"

#include "codec_registration.hpp"
#import <Cocoa/Cocoa.h>

@interface AEExportSettingsViewController : NSViewController
@end
@implementation AEExportSettingsViewController
@end

static bool configureDialog()
{
    auto qualities = CodecRegistry::codec()->qualityDescriptions();
    auto quality = qualities.begin();

	return true;
}

bool
ui_OutDialog(int &quality, void *platformSpecific)
{

	AEExportSettingsViewController *controller = [[AEExportSettingsViewController alloc] initWithNibName:nil bundle:nil];

	// set globals
	// g_Quality = quality;

    // do dialog
    // HWND* hwndOwner = (HWND *)platformSpecific;
    // DialogBox(hDllInstance, (LPSTR)"OUTDIALOG", *hwndOwner, (DLGPROC)DialogProc);

	// if(g_item_clicked == OUT_OK)
	// {
	// 	quality = (int)g_Quality;
		
	// 	return true;
	// }
	// else
	// 	return false;
	
    return false;
}
