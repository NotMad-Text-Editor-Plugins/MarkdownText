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

#define SELF_REFRESH WM_USER+9

using namespace std;

const TCHAR sectionName[] = TEXT("Setting");
const TCHAR strMarkColor[] = TEXT("MarkColor");
const TCHAR strSaveColor[] = TEXT("SaveColor");
const TCHAR configFileName[] = TEXT("MarkDownText.ini");
extern TCHAR iniFilePath[MAX_PATH];


extern bool legacy ;

extern HWND curScintilla;
//extern CRITICAL_SECTION criCounter;
extern FuncItem funcItem[nbFunc];
extern int IconID[nbFunc];
extern NppData nppData;

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

	void RefreshWebview(bool fromEditor=0);

	void refreshDlg(bool updateList, bool fromEditor);

	void setParent(HWND parent2set){
		_hParent = parent2set;
	};
	wkeWebView mWebView=0;
	mbWebView mWebView_1=0;
	bwWebView mWebView_2=0;
	intptr_t currentKernal=0;
	HWND hBrowser=nullptr;
	char* CustomRoutine=nullptr;
	LONG_PTR lastBid=0;
protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	SelfCtrl _color,_savecolor;
private :
	int skFlags=0;
	bool hasChanged;
	ToolbarPanel ListBoxPanel;
	//Window ListBoxWrap;
	ToolBar toolBar;
	void OnToolBarCommand( UINT Cmd );
	void AppendPageResidue(char* appendSt);
};

#endif //LNHISTORY_DLG_H
