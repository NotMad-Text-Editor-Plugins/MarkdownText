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

#include "MDTextDlg.h"
#include "PluginDefinition.h"

std::wstring getResourcesPath(const std::wstring& name)
{
	TCHAR ResPath[MAX_PATH];
	::GetModuleFileName(nullptr, ResPath, MAX_PATH);
	::PathRemoveFileSpecW(ResPath);
	::PathAppendW(ResPath, name.c_str());
	return ResPath;
}


void WKE_CALL_TYPE onDidCreateScriptContextCallback(wkeWebView webView, void* param, wkeWebFrameHandle frameId, void* context, int extensionGroup, int worldId)
{

}

// 回调：点击了关闭、返回 true 将销毁窗口，返回 false 什么都不做。
bool handleWindowClosing(wkeWebView webWindow, void* param)
{
	return true;
}

// 回调：窗口已销毁
void handleWindowDestroy(wkeWebView webWindow, void* param)
{
}

// 回调：文档加载成功
void handleDocumentReady(wkeWebView webWindow, void* param)
{
	wkeShowWindow(webWindow, true);
}

// 回调：页面标题改变
void handleTitleChanged(wkeWebView webWindow, void* param, const wkeString title)
{
	wkeSetWindowTitleW(webWindow, wkeGetStringW(title));
}

// 回调：创建新的页面，比如说调用了 window.open 或者点击了 <a target="_blank" .../>
wkeWebView onCreateView(wkeWebView webWindow, void* param, wkeNavigationType navType, const wkeString url, const wkeWindowFeatures* features)
{
	wkeWebView newWindow = wkeCreateWebWindow(WKE_WINDOW_TYPE_POPUP, NULL, features->x, features->y, features->width, features->height);
	wkeShowWindow(newWindow, true);
	return newWindow;
}


bool onLoadUrlBegin(wkeWebView webView, void* param, const char* url, void *job)
{
	return false;
}

void onLoadUrlEnd(wkeWebView webView, void* param, const char *url, void *job, void* buf, int len)
{
	//wkeRunJS();
	return;
}



void MarkDownTextDlg::display(bool toShow){
	DockingDlgInterface::display(toShow);
	if (toShow)
		::SetFocus(::GetDlgItem(_hSelf, IDOK));

	setClosed(false);

	if(toShow && !mWebView) 
	{
		TCHAR NodePath[MAX_PATH]={0};
		::GetModuleFileName((HINSTANCE)g_hModule, NodePath, MAX_PATH);
		::PathRemoveFileSpec(NodePath);
		::PathAppendW(NodePath, L"miniblink_x64.dll");
		wkeSetWkeDllPath(NodePath);

		if(wkeInitialize()) {
			mWebView = wkeCreateWebWindow(WKE_WINDOW_TYPE_CONTROL, _hSelf , 0, 0, 640, 480); 

			if (mWebView)
			{
				//setMoveWindowArea(0, 0, 640, 30); // 设置窗口可拖动区域，用于无边框窗体
				wkeSetWindowTitleW(mWebView, NPP_PLUGIN_NAME);

				wkeOnDidCreateScriptContext(mWebView, onDidCreateScriptContextCallback, this);
				wkeOnWindowClosing(mWebView, handleWindowClosing, this);
				wkeOnWindowDestroy(mWebView, handleWindowDestroy, this);
				wkeOnDocumentReady(mWebView, handleDocumentReady, this);
				wkeOnTitleChanged(mWebView, handleTitleChanged, this);
				wkeOnCreateView(mWebView, onCreateView, this);
				wkeOnLoadUrlBegin(mWebView, onLoadUrlBegin, this);
				wkeOnLoadUrlEnd(mWebView, onLoadUrlEnd, this);
				wkeSetDebugConfig(mWebView, "decodeUrlRequest", nullptr);

				//wkeJsBindFunction("eMsg", &onMsg, nullptr, 5);
				//wkeJsBindFunction("eShellExec", &onShellExec, nullptr, 3);
				//wkeMoveToCenter(mWebView);
				//wkeLoadURLW(mWebView, app->url.c_str());
			}
		}

		if(mWebView) {
			wkeLoadHTML(mWebView, "HeheDaHeheDaHeheDaHeheDaHeheDaHeheDaHeheDaHeheDaHeheDa");
		}
	}

	::SendMessage( _hSelf, SELF_REFRESH, 0, 1);
};

void MarkDownTextDlg::setClosed(bool toClose) {
	_isClosed = toClose;
	::CheckMenuItem(::GetMenu(nppData._nppHandle), 
		funcItem[menuOption]._cmdID, MF_BYCOMMAND | (toClose?MF_UNCHECKED:MF_CHECKED));
}

void MarkDownTextDlg::refreshDlg(bool updateList) {
	bool AlwaysRefreshBtns=1;
	if (isCreated() && isVisible())
	{
		::SendMessage( _hSelf, SELF_REFRESH, AlwaysRefreshBtns, updateList);
		if(!AlwaysRefreshBtns&&!updateList) {
			hasChanged=1;
		}
	} else {
		if(AlwaysRefreshBtns) {
			::SendMessage( _hSelf, SELF_REFRESH, 1, 0);
		} else {
			hasChanged=1;
		}
	}
};


ToolBarButtonUnit ToolBarIconList[] = {
	{IDM_EX_OPTIONS, -1, -1, -1, IDB_EX_OPTIONS }, 
	{IDM_EX_UP, -1, -1, -1, IDB_EX_UP }, 
	{IDM_EX_DOWN, -1, -1, -1, IDB_EX_DOWN }, 
	{IDM_EX_BREFNAME, -1, -1, -1, IDB_EX_BREFNAME }, 
	{IDM_EX_DELETE_ALL, -1, -1, -1, IDB_EX_DELETE_ALL }, 
	{IDM_EX_DELETE, -1, -1, -1, IDB_EX_DELETE }, 
};

#define ListBoxToolBarSize sizeof(ToolBarIconList)/sizeof(ToolBarButtonUnit)

int toolbarHeight=28;

int BufferIdBeforeClick=0;

TCHAR strHint[500]={0};

INT_PTR CALLBACK MarkDownTextDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_COMMAND : 
		{
			if ( (HWND)lParam == toolBar.getHSelf() )
			{
				OnToolBarCommand( LOWORD(wParam) );
				return 0;
			}
			switch (wParam)
			{
				case MAKELONG(IDC_BUTTON_CLEAR,BN_CLICKED):
				{
				}
				break;
			break;
			}
			return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
		}
		break;
		case SELF_REFRESH://WM_TIMER
		{
			
		}
		break;
		case WM_INITDIALOG:
		{
			ListBoxPanel.init( _hInst, _hSelf );
			//ListBoxWrap.init(_hInst, ListBoxPanel.getHSelf());
			//ListBoxPanel.SetChildWindow( &ListBoxWrap );
			toolBar.init( _hInst, _hSelf, TB_STANDARD, ToolBarIconList, ListBoxToolBarSize );
			//toolBar.init( _hInst, ListBoxPanel.getHSelf(), TB_STANDARD, ListBoxToolBarButtons, ListBoxToolBarSize );
			toolBar.display();
			ListBoxPanel.SetToolbar( &toolBar );
		} break;
		case WM_SIZE:
		case WM_MOVE:
		{
			RECT rc;
			getClientRect(rc);

			//rc.top+=100;
			//rc.bottom-=100;

			if(mWebView) {
				wkeMoveWindow(mWebView,rc.left, rc.top+toolbarHeight, rc.right, rc.bottom-toolbarHeight);
				//::MoveWindow(wkeGetWindowHandle(mWebView), rc.left, rc.top, rc.right, rc.bottom,TRUE);
			}

			rc.bottom=toolbarHeight;
			ListBoxPanel.reSizeTo(rc);

			redraw();
		} break;
		case WM_NOTIFY: 
		{
			LPNMHDR	pnmh	= (LPNMHDR)lParam;
			if (pnmh->hwndFrom == _hParent)
			{
				switch (LOWORD(pnmh->code))
				{
				case DMN_CLOSE:
					{
						setClosed(1);
						break;
					}
				default:
					break;
				}
			}
			break;
		}
	}
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}

bool getMenuItemNeedsKeep(int mid) {
	switch(mid) {
		case menuNext:
		case menuPrevious:
		case menuChgNext:
		case menuChgPrevious:
		case menuAutoRecord:
		case menuOption:
		case menuInCurr:
		case menuNeedMark:
		case menuSkipClosed:
		case menuClearOnClose:
		case menuPinMenu:
		case menuPause:
		return true;
	}
	return false;
}

bool getMenuItemChecked(int mid) {
	switch(mid) {
		case menuAutoRecord:
			return false;
	}
	return false;
}

void TrackPopup(HWND _hSelf);

void simulToolbarMenu(HMENU pluginMenu, RECT *rc, HWND _hSelf, bool recreate){
	int cmd = TrackPopupMenu(pluginMenu, TPM_RETURNCMD, rc->left,  rc->top+toolbarHeight, 0, _hSelf, NULL);

	if(cmd) {
		for(int idx=0;idx<nbFunc;idx++) {
			if(funcItem[idx]._cmdID==cmd) {
				funcItem[idx]._pFunc();
				if(pinMenu && getMenuItemNeedsKeep(idx) /*|| idx==nbFunc-2*/) {
					if(recreate) {
						TrackPopup(_hSelf);
					} else {
						simulToolbarMenu(pluginMenu, rc, _hSelf, recreate);
					}
				}
			}
		}
	}
}

extern HANDLE				g_hModule;

void TrackPopup(HWND _hSelf) {

	HMENU pluginMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETPLUGINMENU, 0 , (LPARAM)g_hModule);

	bool recreate=false;

	if(!pluginMenu) {
		//pluginMenu = (HMENU)::GetMenu(nppData._nppHandle);
		if(!pluginMenu) {

			//::MessageBox(NULL, TEXT("111"), TEXT(""), MB_OK);
			pluginMenu = ::CreatePopupMenu();
			unsigned short j = 0;
			for ( ; j < nbFunc ; ++j)
			{
				if (funcItem[j]._pFunc == NULL)
				{
					::InsertMenu(pluginMenu, j, MF_BYPOSITION | MF_SEPARATOR, 0, TEXT(""));
					continue;
				}
				generic_string itemName = funcItem[j]._itemName;
				::InsertMenu(pluginMenu, j, MF_BYPOSITION, funcItem[j]._cmdID, itemName.c_str());
				if (getMenuItemChecked(j))
					::CheckMenuItem(pluginMenu, funcItem[j]._cmdID, MF_BYCOMMAND | MF_CHECKED);
			}
			recreate = true;
		}
	}
	RECT rc;
	GetWindowRect(_hSelf, &rc);
	simulToolbarMenu(pluginMenu, &rc, _hSelf, recreate);
}

void MarkDownTextDlg::OnToolBarCommand( UINT Cmd )
{ 
	switch ( Cmd ) {
		case IDM_EX_OPTIONS:
			TrackPopup(_hSelf);
		return;
	}
}