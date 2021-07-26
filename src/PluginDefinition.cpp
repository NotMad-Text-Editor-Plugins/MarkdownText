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

#include <stdlib.h>
#include <time.h>
#include <shlwapi.h>
#include "MDTextDlg.h"
#include "Scintilla.h"

#include "OptionsDlg.h"

#include "WarningDlg.hpp"

OptionsDlg* pFrame;

MarkDownTextDlg _MDText;

extern bool WindowOpaqueMsg;

bool pinMenu = false;

// toggle the UI configuration boolean. |pos| flag position. |reverse| if set, then default to true.
int ToggleUIBool(int pos, bool reverse)
{
	int mask = 1<<pos;
	bool val = !(UISettings&mask);
	UISettings&=~mask;
	if(val)
	{
		UISettings|=mask;
	}
	return reverse?!val:val;
}

bool GetUIBool(int pos, bool reverse)
{
	return reverse?GetUIBoolReverse(pos):GetUIBool(pos);
}


// get the UI configuration boolean, default to false. |pos| flag position.
bool GetUIBool(int pos)
{
	int mask = 1<<pos;
	return UISettings&mask;
}

// get the UI configuration boolean, but reversed. |pos| flag position.
bool GetUIBoolReverse(int pos)
{
	int mask = 1<<pos;
	return !(UISettings&mask);
}

void pluginCleanUp()
{
	_MDText.saveParameters();
}


void commandMenuCleanUp()
{
	if(funcItems)
	{
		for (size_t i = 0,len=sizeof(funcItems); i < len; i++)
		{
			delete funcItems[i]._pShKey;
		}
		delete[] funcItems;
		funcItems=NULL;
	}
}

void ToggleMDPanel()
{
	if(!NPPRunning && (GetUIBoolReverse(5)||GetUIBool(6)))
	{// if not runnig and auto-run/never-run is checked, reject the initialization
		// todo if auto-run is checked, maybe check the extension name and just don't reject?
		_MDText.bRunRequested = true;
		return;
	}
	_MDText.setParent( nppData._nppHandle );
	if ( !_MDText.isCreated() )
	{
		_MDText.create();
		_MDText.display();
	}
	else {
		bool NeedShowDlg = !_MDText.isVisible();
		_MDText.display(NeedShowDlg);
	}
	//if(!_MDText.isClosed()) _MDText.RefreshWebview();
}

void PreviewCurrentFile() {
	LONG_PTR bid = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
	if(!_MDText.isVisible()){
		ToggleMDPanel();
	}
	if(_MDText.lastBid!=bid)
	{
		bForcePreview=true;
		_MDText.display(true);
		_MDText.RefreshWebview();
		bForcePreview=false;
	}
}

void WrapTextWith(char* padStart, char* padEnd)
{
	int currentEdit=0;
	int padLenSt=strlen(padStart);
	int padLenEd=strlen(padEnd);
	SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
	auto currentSci = currentEdit?nppData._scintillaSecondHandle:nppData._scintillaMainHandle;

	unsigned p1_ = SendMessage(currentSci, SCI_GETSELECTIONSTART, 0, 0);
	unsigned p2_ = SendMessage(currentSci, SCI_GETSELECTIONEND, 0, 0);
	bool hasSel=p2_>p1_;
	if(!hasSel && !legacy)
	{
		SendMessage(nppData._nppHandle, NPPM_SELCARET, 0, 0);
		p1_ = SendMessage(currentSci, SCI_GETSELECTIONSTART, 0, 0);
		p2_ = SendMessage(currentSci, SCI_GETSELECTIONEND, 0, 0);
		hasSel=p2_>p1_;
	}
	if(hasSel)
	{
		unsigned sellen = SendMessage(currentSci, SCI_GETSELTEXT, 0, 0)-1;
		unsigned textLen = SendMessage(currentSci, SCI_GETTEXTLENGTH, 0, 0);
		unsigned idealSTL = sellen;

		int p1 = p1_-padLenSt;
		unsigned p2 = p2_+padLenEd;
		unsigned bufferLen=sellen+padLenSt+padLenEd+2;
		char *txUCS2 = new char[bufferLen];
		if(txUCS2)
		{
			memset(txUCS2, 0, bufferLen);
			auto writeAddr=txUCS2;
			if(p1<0) 
			{
				writeAddr-=p1;
				idealSTL+=p1;
				p1=0;
			}
			if(p2>=textLen) 
			{
				idealSTL+=textLen-1-p2;
				p2=textLen;
			}
			//size_t len = SendMessage(currentSci, SCI_GETSELTEXT, 0, (LPARAM)(txUCS2+padLen))-1;
			Sci_TextRange tr{0};
			tr.chrg.cpMin = p1;
			tr.chrg.cpMax = p2;
			tr.lpstrText = writeAddr;

			size_t len = SendMessage(currentSci, SCI_GETTEXTRANGE, 0, (LPARAM)&tr)-1;

			if(len<=sellen+padLenSt+padLenEd)
			{ 
				len=padLenSt+sellen;
				if(strncmp(txUCS2, padStart, padLenSt)==0&&strncmp(txUCS2+len, padEnd, padLenEd)==0)
				{ // 取消加粗
					txUCS2[len]='\0';
					SendMessage(currentSci, SCI_SETTARGETSTART, p1, 0);
					SendMessage(currentSci, SCI_SETTARGETEND, p2, 0);
					SendMessage(currentSci, SCI_REPLACETARGET, sellen, (LPARAM)(txUCS2+padLenSt));
					SendMessage(currentSci, SCI_SETSEL, p1, p1+sellen);
				}
				else
				{ // 加粗
					memcpy(txUCS2, padStart, padLenSt);
					memcpy(txUCS2+len, padEnd, padLenEd);

					len+=padLenEd;
					SendMessage(currentSci, SCI_SETTARGETSTART, p1_, 0);
					SendMessage(currentSci, SCI_SETTARGETEND, p2_, 0);
					SendMessage(currentSci, SCI_REPLACETARGET, len, (LPARAM)txUCS2);
					SendMessage(currentSci, SCI_SETSEL, p1_+padLenSt, p1_+padLenSt+sellen);
				}
				
			}
			
			delete[] txUCS2;
		}
	}
}

void BoldenText()
{
	WrapTextWith("**", "**");
}

void TiltText()
{
	WrapTextWith("*", "*");
}

void UnderlineText()
{
	WrapTextWith("<u>", "</u>");
}

void voidFunc()
{
}

void CheckMenu(FuncItem* funcItem, bool val)
{
	auto menu = ::GetMenu(nppData._nppHandle);
	::CheckMenuItem(menu, funcItem->_cmdID, MF_BYCOMMAND | (static_cast<BOOL>(val) ? MF_CHECKED : MF_UNCHECKED));
}

void PauseUpdate()
{
	CheckMenu(funcUpdate, ToggleUIBool(3, false));
	if(!GetUIBool(3))
	{
		SCNotification note{};
		note.nmhdr.code=SCN_MODIFIED;
		note.length=1;
		note.modificationType = SC_MOD_DELETETEXT;
		beNotified(&note);
	}
}

void ChainedUpdate()
{
	if (legacy&&!GetUIBool(8))
	{
		WarnDlg* wdlg = new WarnDlg(TEXT("txt.xml"));
		wdlg->Create(nppData._nppHandle
			, _MDText.GetLocalWText("not_sup", TEXT("Not supported !")).c_str()
			, WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION , WS_EX_DLGMODALFRAME );
		wdlg->CenterWindow();
		wdlg->ShowModal(nppData._nppHandle);
	}
	CheckMenu(&funcItems[8], ToggleUIBool(8, false));
}

void SyncScroll()
{
	_MDText.GlobalOnPvMnChecked(0, 260);
}

HWND handle;

void Settings()
{
	if(!pFrame) 
	{
		pFrame = new OptionsDlg();
		pFrame->Create(nppData._nppHandle
			, TEXT("MarkdownText")
			, WS_POPUP|WS_CLIPSIBLINGS|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MAXIMIZEBOX|WS_MINIMIZEBOX
			, WS_EX_CONTROLPARENT|WS_EX_WINDOWEDGE|WS_EX_DLGMODALFRAME);
		// WS_EX_DLGMODALFRAME WS_EX_CONTROLPARENT|WS_EX_WINDOWEDGE
		pFrame->CenterWindow();
		pFrame->ShowWindow(true);
	}
}

// Initialize Duilib UI Resources.
void InitResource()
{	
	CDuiString strResourcePath = CPaintManagerUI::GetInstancePath();
	switch(CPaintManagerUI::GetResourceType())
	{
		case UILIB_FILE:
		case UILIB_ZIP:
		{
			// debug only, peeking into the source folder.
			strResourcePath += _T("..\\..\\..\\..\\plugins\\MarkdownText\\res\\skin\\");
			CPaintManagerUI::SetResourcePath(strResourcePath.GetData());
			if (CPaintManagerUI::GetResourceType()==UILIB_ZIP)
			{
				CPaintManagerUI::SetResourceZip(_T("MDRes.zip"), true);
			}
			break;
		}
		case UILIB_ZIPRESOURCE:
		{
			// read from embedded zip resource in the dll.
			CPaintManagerUI::SetResourcePath(strResourcePath.GetData());
			HRSRC hResource = ::FindResource(CPaintManagerUI::GetResourceDll(), MAKEINTRESOURCE(IDR_ZIPRES), _T("ZIPRES"));
			if( hResource != NULL ) {
				DWORD dwSize = 0;
				HGLOBAL hGlobal = ::LoadResource(CPaintManagerUI::GetResourceDll(), hResource);
				if( hGlobal != NULL ) {
					dwSize = ::SizeofResource(CPaintManagerUI::GetResourceDll(), hResource);
					if( dwSize > 0 ) {
						CPaintManagerUI::SetResourceZip((LPBYTE)::LockResource(hGlobal), dwSize);
					}
				}
				::FreeResource(hResource);
			}
		}
		break;
	}
}

void commandMenuInit()
{
	// Initialization of your plugin commands
	// Firstly we get the parameters from your plugin config file (if any)
	// get path of plugin configuration
	_MDText.readParameters();
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

	PluginMenuStrIds = new CHAR*[]{
		"_vcf",
		"_pnl",
		NULL,
		"_b",
		"_i",
		"_u",
		//NULL,
		"_tls",
		NULL,
		"_pp",
		"_cp",
		"_ss",
		"_opt",
		NULL,
	};

	funcItems = new FuncItem []{
		 {TEXT("View Current File"), PreviewCurrentFile, menuPreviewCurr, false, new ShortcutKey{0,0,0,NULL}}
		,{TEXT("Markdown Text Panel"), ToggleMDPanel, menuOption, false, new ShortcutKey{1,0,1,0x4D}} // VK_M
		,{TEXT("-SEPARATOR-"), NULL, NULL, false} 
		,{TEXT("Bolden"), BoldenText, menuBolden, false, new ShortcutKey{0,0,0,NULL}} 
		,{TEXT("Italic"), TiltText, menuItalic, false, new ShortcutKey{0,0,0,NULL}} 
		,{TEXT("Underline"), UnderlineText, menuUnderLine, false, new ShortcutKey{0,0,0,NULL}} 
		//,{TEXT("-SEPARATOR-"), NULL, NULL, false} 
		,{TEXT("Tools"), voidFunc, menuTools, false, new ShortcutKey{0,0,0,NULL}} 
		,{TEXT("-SEPARATOR-"), NULL, NULL, false} 
		,{TEXT("Pause Update"), PauseUpdate, menuPause, GetUIBool(3), new ShortcutKey{0,0,0,NULL}} 
		,{TEXT("Chained Update"), ChainedUpdate, menuChained, GetUIBool(8), new ShortcutKey{0,0,0,NULL}} 
		,{TEXT("Sync Scroll"), SyncScroll, menuSync, GetUIBoolReverse(0), new ShortcutKey{0,0,0,NULL}} 
		,{TEXT("Options…"), Settings, menuSettings, false, new ShortcutKey{0,0,0,NULL}} 
		,NULL
	};
	funcMenu=&funcItems[1];
	funcUpdate=&funcItems[7];
	funcSync=&funcItems[8];

	CPaintManagerUI::SetInstance((HINSTANCE)g_hModule);
	CPaintManagerUI::SetResourceDll((HINSTANCE)g_hModule);

	CPaintManagerUI::SetResourceType(UILIB_ZIPRESOURCE);
	//CPaintManagerUI::SetResourceType(UILIB_ZIP);
	CPaintManagerUI::SetResourceType(UILIB_FILE);

	InitResource();

	_MDText.getLocaliseMap();
}