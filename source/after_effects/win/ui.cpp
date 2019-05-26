#include "../ui.h"

#include <stdint.h>
#include <vector>


#include <Windows.h>

#include "codec_registration.hpp"

// dialog comtrols
enum {
	OUT_noUI = -1,
	OUT_OK = IDOK,
	OUT_Cancel = IDCANCEL,
	OUT_Quality_Menu = 3
};


// globals
HINSTANCE hDllInstance = NULL;

static WORD	g_item_clicked = 0;

static LRESULT g_Quality = 0;

static BOOL CALLBACK DialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{
    switch (message) 
    { 
		case WM_INITDIALOG:
			do{
				// set up the menu
				HWND menu = GetDlgItem(hwndDlg, OUT_Quality_Menu);

                auto qualities = CodecRegistry::codec()->qualityDescriptions();
                auto quality = qualities.begin();

				for(int i=0; i < qualities.size(); ++i, ++quality)
				{
					SendMessage(menu,( UINT)CB_ADDSTRING, (WPARAM)wParam, (LPARAM)(LPCTSTR)quality->second.c_str() );
					SendMessage( menu,(UINT)CB_SETITEMDATA, (WPARAM)i, (LPARAM)(DWORD)quality->first); // this is the compresion number

					if(quality->first == g_Quality)
						SendMessage( menu, CB_SETCURSEL, (WPARAM)i, (LPARAM)0);
				}
			}while(0);

			return TRUE;
 

        case WM_COMMAND: 
			g_item_clicked = LOWORD(wParam);

            switch (LOWORD(wParam)) 
            { 
                case OUT_OK: 
				case OUT_Cancel:  // do the same thing, but g_item_clicked will be different
					do{
						HWND menu = GetDlgItem(hwndDlg, OUT_Quality_Menu);

						// get the channel index associated with the selected menu item
						LRESULT cur_sel = SendMessage(menu,(UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

						g_Quality = SendMessage(menu,(UINT)CB_GETITEMDATA, (WPARAM)cur_sel, (LPARAM)0);

					}while(0);

					//PostMessage((HWND)hwndDlg, WM_QUIT, (WPARAM)WA_ACTIVE, lParam);
					EndDialog(hwndDlg, 0);
                    //DestroyWindow(hwndDlg); 
                    return TRUE;
            } 
    } 
    return FALSE; 
} 

bool
ui_OutDialog(int &quality, void *platformSpecific)
{
	// set globals
	g_Quality = quality;

    // do dialog
    HWND* hwndOwner = (HWND *)platformSpecific;
    DialogBox(hDllInstance, (LPSTR)"OUTDIALOG", *hwndOwner, (DLGPROC)DialogProc);

	if(g_item_clicked == OUT_OK)
	{
		quality = (int)g_Quality;
		
		return true;
	}
	else
		return false;
}

BOOL WINAPI DllMain(HANDLE hInstance, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		hDllInstance = (HINSTANCE)hInstance;

	return TRUE;   // Indicate that the DLL was initialized successfully.
}
