//this file is part of notepad++
//Copyright (C) 2011 AustinYoung<pattazl@gmail.com>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginDefinition.h"
#include "menuCmdID.h"

//
// put the headers you need here
//
#include <stdlib.h>
#include <time.h>
#include <shlwapi.h>
#include "MDTextDlg.h"


MarkDownTextDlg _MDText;

extern int BufferIdBeforeClick;

extern bool WindowOpaqueMsg;

bool pinMenu = false;

HWND curScintilla=0;

long MarkColor = DefaultColor;
long SaveColor = DefaultSaveColor;

#ifdef UNICODE 
	#define generic_itoa _itow
#else
	#define generic_itoa itoa
#endif

FuncItem funcItem[nbFunc];
bool menuState[nbFunc];
int IconID[nbFunc];
// The data of Notepad++ that you can use in your plugin commands
NppData nppData;
HANDLE				g_hModule;
toolbarIcons		g_TBPrevious{0,0,0x666,0,IDI_ICON_PREV,IDI_ICON_PREV_ACT,IDI_ICON_PREV_OFF,IDB_BITMAP1};

TCHAR iniFilePath[MAX_PATH];
//bool SaveRecording = false;

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE hModule)
{
	// Initialize dialog
	_MDText.init((HINSTANCE)hModule, NULL);
	g_hModule = hModule;

	//InitializeCriticalSection(&criCounter); 
}

// Here do the clean up, save the parameters if any
void pluginCleanUp()
{
	//DeleteCriticalSection(&criCounter);
	//if(1) return;
	TCHAR str[500]={0};	
	//wsprintf(str,TEXT("%d"),SaveColor);
	//::WritePrivateProfileString(sectionName, strSaveColor, str, iniFilePath);

}

// Here do the clean up (especially for the shortcut)
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
	delete funcItem[menuPrevious]._pShKey;
	delete funcItem[menuNext]._pShKey;
	delete funcItem[menuChgPrevious]._pShKey;
	delete funcItem[menuChgNext]._pShKey;
	delete funcItem[menuOption]._pShKey;
	delete funcItem[menuInCurr]._pShKey;
	delete funcItem[menuNeedMark]._pShKey;

	//::WritePrivateProfileString(sectionName, strRecordContent, NULL, iniFilePath);
}

//----------------------------------------------//
//-- ASSOCIATED FUNCTIONS START------------------//
//----------------------------------------------//

void ToggleHistoryPanel()
{
	_MDText.setParent( nppData._nppHandle );
	tTbData data = {0};

	if ( !_MDText.isCreated() )
	{
		WindowOpaqueMsg = 0;
		_MDText.create( &data );
		WindowOpaqueMsg = 1;
		// define the default docking behaviour
		data.uMask          = DWS_DF_CONT_RIGHT | DWS_ICONTAB;
		data.pszModuleName = _MDText.getPluginFileName();
		// the dlgDlg should be the index of funcItem where the current function pointer is
		data.dlgID = menuOption;
		data.hIconTab       = ( HICON )::LoadImage( _MDText.getHinst(),
			MAKEINTRESOURCE( IDI_ICON1 ), IMAGE_ICON, 14, 14,
			LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT );
		::SendMessage( nppData._nppHandle, NPPM_DMMREGASDCKDLG, 0, ( LPARAM )&data );

		_MDText.display();
		::SendMessage( nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[menuOption]._cmdID, true );
	} else {
		bool NeedShowDlg = !_MDText.isVisible();

		_MDText.display(NeedShowDlg);

		::SendMessage( nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[menuOption]._cmdID, NeedShowDlg );
	}
}

void ShowAbout()
{
	::MessageBox(nppData._nppHandle, TEXT(" You can use Ctrl+ - jump to previous cursor position \n You can use Ctrl+Shift+ - jump to next cursor position \n You can use Ctrl+Alt+ Z jump to previous changed position \n You can use Ctrl+Alt+ Y jump to next changed position \n 'Auto clear when close'- Will remove the file's record when file closed.\n 'Always record'- Will always record the position even after you jumped.\n 'Save record when App exit'- Record data when application exit and it will be loaded in next run \n 'In Curr'- If checked, navigate only in current file\n 'Mark'- If checked, modified line will be marked by bookmark or color\n 'Mark Color/Save Color'- Available if not select mark with bookmark, you could mark with different symbol.  \n\nBundled with Textrument (v0.1.1). \n\n (Save/Restore Currently unavailable!)  Version: Original Author: Austin Young<pattazl@gmail.com>"), TEXT("About Location Navigate"), MB_OK);
}

//----------------------------------------------//
//-- ASSOCIATED FUNCTIONS END-------------------//
//----------------------------------------------//

bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
	if (index >= nbFunc)
		return false;

	if (!pFunc)
		return false;

	lstrcpy(funcItem[index]._itemName, cmdName);
	funcItem[index]._pFunc = pFunc;
	funcItem[index]._init2Check = check0nInit;
	funcItem[index]._pShKey = sk;

	return true;
}

void commandMenuInit()
{
	// Initialization of your plugin commands
	// Firstly we get the parameters from your plugin config file (if any)
	// get path of plugin configuration
	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)iniFilePath);

	// if config path doesn't exist, we create it
	if (PathFileExists(iniFilePath) == FALSE)
	{
		::CreateDirectory(iniFilePath, NULL);
	}

	// make your plugin config file full file path name
	PathAppend(iniFilePath, configFileName);

	// get the parameter value from plugin config
	MarkColor = ::GetPrivateProfileInt(sectionName, strMarkColor, DefaultColor, iniFilePath);
	SaveColor = ::GetPrivateProfileInt(sectionName, strSaveColor, DefaultSaveColor, iniFilePath);
	//--------------------------------------------//
	//-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
	//--------------------------------------------//
	// with function :
	// setCommand(int index,                      // zero based number to indicate the order of command
	//            TCHAR *commandName,             // the command name that you want to see in plugin menu
	//            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
	//            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
	//            bool check0nInit                // optional. Make this menu item be checked visually
	//            );
	#define VK_OEM_MINUS      0xBD
	ShortcutKey *PreviousKey = new ShortcutKey{1,0,0,VK_OEM_MINUS};
	ShortcutKey *NextKey = new ShortcutKey{1,0,1,VK_OEM_MINUS};
	ShortcutKey *PreChgKey = new ShortcutKey{1,1,0,0x5A};//VK_Z
	ShortcutKey *NextChgKey = new ShortcutKey{1,1,0,0x59};//VK_Y
	ShortcutKey *optionsKey = new ShortcutKey{1,1,1,VK_OEM_MINUS};
	ShortcutKey *AutoKey = new ShortcutKey{1,0,1,VK_F9};
	ShortcutKey *ManualKey = new ShortcutKey{0,0,0,VK_F9};
	ShortcutKey *ClearRecordsKey = new ShortcutKey{1,1,1,VK_F9};
	ShortcutKey *incurrKey = new ShortcutKey{0,1,0,VK_OEM_MINUS};
	ShortcutKey *markKey = new ShortcutKey{1,1,0,0x4D};// VK_M

	setCommand(menuOption, TEXT("Show List and Option"), ToggleHistoryPanel, optionsKey, false);

	setCommand(menuSeparator0, TEXT("-SEPARATOR-"),NULL, NULL, false);

}