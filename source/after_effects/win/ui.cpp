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
    OUT_SubTypes_Menu = 3,
    OUT_Quality_Menu = 4,
    OUT_ChunkCount_Field = 5
};


// globals
HINSTANCE hDllInstance = NULL;

static WORD	g_item_clicked = 0;

static LRESULT g_SubType = 0;
static LRESULT g_Quality = 0;
static LRESULT g_ChunkCount = 0;

static BOOL CALLBACK DialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{
    switch (message) 
    { 
		case WM_INITDIALOG:
			do{
                const auto& codec = *CodecRegistry::codec();

                bool hasSubTypes(codec.details().subtypes.size() > 0);
                if (hasSubTypes) {
                    // set up the menu
                    HWND menu = GetDlgItem(hwndDlg, OUT_SubTypes_Menu);

                    auto subTypes = codec.details().subtypes;
                    auto subType = subTypes.begin();

                    for (int i = 0; i < subTypes.size(); ++i, ++subType)
                    {
                        SendMessage(menu, (UINT)CB_ADDSTRING, (WPARAM)wParam, (LPARAM)(LPCTSTR)subType->second.c_str());
                        DWORD subTypeVal = reinterpret_cast<DWORD&>(subType->first);
                        SendMessage(menu, (UINT)CB_SETITEMDATA, (WPARAM)i, (LPARAM)subTypeVal); // subtype fourcc

                        if (subTypeVal == g_SubType)
                            SendMessage(menu, CB_SETCURSEL, (WPARAM)i, (LPARAM)0);
                    }
                }
                else
                {
                    //!!! TODO do not show subtypes item
                }

                if (codec.hasQualityForAnySubType())
                {
                    // set up the menu
                    HWND menu = GetDlgItem(hwndDlg, OUT_Quality_Menu);

                    auto qualities = CodecRegistry::codec()->qualityDescriptions();
                    auto quality = qualities.begin();

                    for (int i = 0; i < qualities.size(); ++i, ++quality)
                    {
                        SendMessage(menu, (UINT)CB_ADDSTRING, (WPARAM)wParam, (LPARAM)(LPCTSTR)quality->second.c_str());
                        SendMessage(menu, (UINT)CB_SETITEMDATA, (WPARAM)i, (LPARAM)(DWORD)quality->first); // this is the quality enum

                        if (quality->first == g_Quality)
                            SendMessage(menu, CB_SETCURSEL, (WPARAM)i, (LPARAM)0);
                    }

                    //!!! TODO enable / disable depending upon selected codec subtype
                }
                else
                {
                    //!!! TODO do not show qualities item
                }

                if (codec.details().hasChunkCount)
                {
                    // set up the menu
                    //!!! HWND menu = GetDlgItem(hwndDlg, OUT_ChunkCount_Field);

                    //!!! ChunkCount field setup here
                }
                else
                {
                    //!!! TODO do not show qualities item
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
                        // subType
                        const auto& codec = *CodecRegistry::codec();

                        bool hasSubTypes(codec.details().subtypes.size() > 0);
                        if (hasSubTypes) {
                            HWND menu = GetDlgItem(hwndDlg, OUT_SubTypes_Menu);

                            LRESULT cur_sel = SendMessage(menu, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                            g_SubType = SendMessage(menu, (UINT)CB_GETITEMDATA, (WPARAM)cur_sel, (LPARAM)0);
                        }

                        // quality
                        if (codec.hasQualityForAnySubType())
                        {
                            HWND menu = GetDlgItem(hwndDlg, OUT_Quality_Menu);

                            // get the channel index associated with the selected menu item
                            LRESULT cur_sel = SendMessage(menu, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                            g_Quality = SendMessage(menu, (UINT)CB_GETITEMDATA, (WPARAM)cur_sel, (LPARAM)0);
                        }

                        if (codec.details().hasChunkCount)
                        {
                            //!!! chunk count field here
                            //!!! g_ChunkCount = ?
                            //!!!
                        }
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
ui_OutDialog(CodecSubType& subType, int& quality, int &chunkCount, void *platformSpecific)
{
	// set globals
    g_SubType = reinterpret_cast<DWORD&>(subType);
	g_Quality = quality;
    g_ChunkCount = chunkCount;

    // do dialog
    HWND* hwndOwner = (HWND *)platformSpecific;
    DialogBox(hDllInstance, (LPSTR)"OUTDIALOG", *hwndOwner, (DLGPROC)DialogProc);

	if(g_item_clicked == OUT_OK)
	{
        const auto& codec = *CodecRegistry::codec();

        bool hasSubTypes = (codec.details().subtypes.size() > 0);
        if (hasSubTypes)
            subType = reinterpret_cast<CodecSubType&>(g_SubType);

        if (codec.hasQualityForAnySubType())
            quality = (int)g_Quality;
		
        if (codec.details().hasChunkCount)
            chunkCount = (int)g_ChunkCount;

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
