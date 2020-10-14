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
#pragma once
#ifndef LNHISTORY_DLG_H
#define LNHISTORY_DLG_H

#include "DockingDlgInterface.h"
#include "PluginDefinition.h"
#include "SelfCtrl.h"
#include "resource.h"
#include <deque>
#include "menuCmdID.h"
#include "ToolbarPanel.h"
#include <wke.h>
#include <mb.h>
#include <BrowserUI.h>

#include "stdafx.h"
#include "CheckFailure.h"
#include <shlwapi.h>

#define SELF_REFRESH WM_USER+9

using namespace std;

const TCHAR sectionName[] = TEXT("Setting");
const TCHAR strMarkColor[] = TEXT("MarkColor");
const TCHAR strSaveColor[] = TEXT("SaveColor");
const TCHAR configFileName[] = TEXT("MarkDownText.ini");

extern bool legacy ;

extern HWND curScintilla;
//extern CRITICAL_SECTION criCounter;

extern void ClearLocationList();

bool SetPosByIndex(int delta, bool doit=true);
void EnableTBButton(menuList flagIndex, bool state, bool force=false);
#define TB_ENABLEBUTTON         (WM_USER + 1)
class MarkDownTextDlg : public DockingDlgInterface
{
public :
	MarkDownTextDlg() : DockingDlgInterface(IDD_LOCATIONNAVIGATE) {};

    virtual void display(bool toShow = true); 

	virtual void setClosed(bool toClose);

	void RefreshWebview(bool fromEditor=false);

	void refreshDlg(bool updateList, bool fromEditor);

	void setParent(HWND parent2set){
		_hParent = parent2set;
	};
	void saveParameters();
	void readParameters();
	void switchEngineByIndex(int id);

	wkeWebView mWebView=0;
	mbWebView mWebView_1=0;
	bwWebView mWebView_2=0;
	wil::com_ptr<ICoreWebView2> mWebView_3;
	wil::com_ptr<ICoreWebView2Environment> m_webViewEnvironment;
	wil::com_ptr<ICoreWebView2Controller> webviewController;
	intptr_t currentkernel=0;
	HWND hBrowser=nullptr;
	char CustomRoutine[MAX_PATH_HALF]={0};
	TCHAR* LibPath=nullptr;
	LONG_PTR lastBid=0;
	ToolBar toolBar;
protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	SelfCtrl _color,_savecolor;
public :
	int kernelType=-1; // -1_auto 0_wke 1_mb 2_bw 3_WV2
	int currentkernelType=0; // 0_wke 1_mb 2_bw 3_WV2
	int skFlags=0;
	bool hasChanged;
	ToolbarPanel ListBoxPanel;
	//Window ListBoxWrap;
	int getToolbarCommand(POINT &pointer);
	void OnToolBarRequestToolTip( LPNMHDR nmhdr );
	void OnToolBarCommand( UINT Cmd, char source=0, POINT* pt=nullptr);
	void AppendPageResidue(char* appendSt);
	void syncWebToline(bool force=false);
	int lastSyncLn=0;
};

#endif //LNHISTORY_DLG_H
