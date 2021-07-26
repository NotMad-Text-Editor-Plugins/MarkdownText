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

#ifndef PLUGINDEFINITION_H
#define PLUGINDEFINITION_H

#include "PluginInterface.h"
#include "InsituDebug.h"
#include <vector>

class MarkDownTextDlg;

//-------------------------------------//
//-- STEP 1. DEFINE YOUR PLUGIN NAME --//
//-------------------------------------//
// Here define your plugin name
//
const TCHAR NPP_PLUGIN_NAME[] = TEXT("Markdown Text");

//-----------------------------------------------//
//-- STEP 2. DEFINE YOUR PLUGIN COMMAND NUMBER --//
//-----------------------------------------------//
//
// Here define the number of your plugin commands
//
enum menuList
{
	menuPreviewCurr = 0,
	menuOption,

	menuSeparator0,

	menuBolden ,
	menuItalic ,
	menuUnderLine,

	//menuSeparator2,
	menuTools,

	menuSeparator3,
	menuPause,
	menuChained,
	menuSync,
	menuSettings,

	menuCount,
};

//const int nbFunc = menuCount;
// 270*300
#define RecordConentMax  81000
extern TCHAR currFile[MAX_PATH];// 当前窗口文件名
//
// Initialization of your plugin data
// It will be called while plugin loading
//
void pluginInit(HANDLE hModule);

//
// Cleaning of your plugin
// It will be called while plugin unloading
//
void pluginCleanUp();

//
//Initialization of your plugin commands
//
void commandMenuInit();

//
//Clean up your plugin commands allocation (if any)
//
void commandMenuCleanUp();

extern MarkDownTextDlg _MDText;

extern bool pinMenu;

__declspec(selectany) bool NPPRunning = false;

__declspec(selectany) TCHAR			g_ModulePath[MAX_PATH]{0};
__declspec(selectany) TCHAR			path_buffer[MAX_PATH]{0};
__declspec(selectany) TCHAR			last_actived[MAX_PATH]{0};
__declspec(selectany) TCHAR			last_updated[MAX_PATH]{0};

//__declspec(selectany)  std::vector<FuncItem> funcItems;
__declspec(selectany)  FuncItem* funcItems;
__declspec(selectany)  CHAR** PluginMenuStrIds;
__declspec(selectany)  NppData nppData;
__declspec(selectany)  HANDLE				g_hModule;

__declspec(selectany) bool			legacy;
__declspec(selectany) bool			bForcePreview=false;
__declspec(selectany) int			UISettings=0;
__declspec(selectany) HWND curScintilla=0;

__declspec(selectany) FuncItem* funcMenu;
__declspec(selectany) FuncItem* funcSync;
__declspec(selectany) FuncItem* funcUpdate;

extern int ToggleUIBool(int pos, bool reverse);
extern bool GetUIBool(int pos);
extern bool GetUIBool(int pos, bool reverse);
extern bool GetUIBoolReverse(int pos);

void CheckMenu(FuncItem* funcItem, bool val);

#define MAX_PATH_HALF 128
#endif //PLUGINDEFINITION_H
