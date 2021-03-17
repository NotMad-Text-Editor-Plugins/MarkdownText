/*
* Copyright 2020 Encapsulate miniblink, libcef and webview2 in one c++ file.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

// entry : see readme

#include "windowsx.h"
#include "MDTextDlg.h"
#include "PluginDefinition.h"
#include "time.h"
#include "../../NativeLang/src/NativeLang_def.h"
#include <ProfileStd.h>
#include "ArticlePresenter.h"
#include "SU.h"

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

void MarkDownTextDlg::doScintillaScroll(int ln)
{
	int curScintilla;
	SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&curScintilla);
	auto currrentSc = curScintilla?nppData._scintillaSecondHandle:nppData._scintillaMainHandle;
	SendMessage(currrentSc, SCI_SETFIRSTVISIBLELINE, lastSyncLn=ln, 0);
}

CHAR* MarkDownTextDlg::loadSourceAsset(uptr_t bid, const char* pathA, DWORD & dataLen)
{
	TCHAR path[MAX_PATH];
	MultiByteToWideChar (CP_ACP, 0, pathA, strlen (pathA) + 1, path, 256) ;
	TCHAR SrcPath[MAX_PATH]={0};
	if(!bid) {
		bid = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
	}
	if(bid) {
		::SendMessage(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, (WPARAM)bid, (LPARAM)SrcPath);
		::PathRemoveFileSpec(SrcPath);
		::PathAppend(SrcPath, path);

		if(PathFileExists(SrcPath)) 
		{
			HANDLE hFile = CreateFile(SrcPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (INVALID_HANDLE_VALUE == hFile) { 
				return NULL;
			}
			DWORD fileSizeHigh;
			dataLen = ::GetFileSize(hFile, &fileSizeHigh);
			DWORD numberOfBytesRead = 0;
			CHAR* buffer = new CHAR[dataLen];
			if(buffer && ::ReadFile(hFile, buffer, dataLen, &numberOfBytesRead, nullptr))
			{
				::CloseHandle(hFile);
				return buffer;
			} 
			::CloseHandle(hFile);
		}
	}
	return NULL;
}

CHAR* MarkDownTextDlg::loadPluginAsset(const char* path, DWORD & dataLen)
{
	CHAR ResPath[MAX_PATH];
#if 1 // debug resource path
	strcpy(ResPath, "D:\\Code\\FigureOut\\chrome\\extesions\\MarkdownEngines\\");
#else
	::GetModuleFileNameA((HINSTANCE)g_hModule, ResPath, MAX_PATH);
	::PathRemoveFileSpecA(ResPath);
#endif
	::PathAppendA(ResPath, path);
	if(PathFileExistsA(ResPath)) 
	{
		HANDLE hFile = CreateFileA(ResPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE == hFile) { 
			return NULL;
		}
		DWORD fileSizeHigh;
		dataLen = ::GetFileSize(hFile, &fileSizeHigh);
		DWORD numberOfBytesRead = 0;
		CHAR* buffer = new CHAR[dataLen];
		if(buffer && ::ReadFile(hFile, buffer, dataLen, &numberOfBytesRead, nullptr))
		{
			::CloseHandle(hFile);
			return buffer;
		} 
		::CloseHandle(hFile);
	}
	return NULL;
}

UINT ScintillaGetText(HWND hWnd, char *text, INT start, INT end)
{
	Sci_TextRange tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText  = text;
	return (UINT)::SendMessage(hWnd, SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr));
}


CHAR*	universal_buffer = new CHAR[4];
int buffer_cap=4;


LONG_PTR nextPowerOfTwo(size_t v)
{
	size_t vbk = v;
	v--;
	int mv=1;
#ifdef _WIN64
	for(;mv<=32;mv<<=1)
#else
	for(;mv<=16;mv<<=1)
#endif
	{
		v |= v >> mv;
	}
	v++;
	if(v<vbk)
	{
		v=vbk;
	}
	return v;
}

CHAR* MarkDownTextDlg::GetDocTex(size_t & docLength, LONG_PTR bid, bool * shouldDelete)
{
	int curScintilla;
	SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&curScintilla);
	auto currrentSc = curScintilla?nppData._scintillaSecondHandle:nppData._scintillaMainHandle;

	if(shouldDelete) {
		*shouldDelete = false;
	}

	if(currentkernelType!=MINILINK_TYPE && bid && false) //  && !legacy
	{
		// fast-read the original memory data
		LONG_PTR DOCUMENTPTR = SendMessage(nppData._nppHandle, NPPM_GETDOCUMENTPTR, bid, bid);
		docLength = SendMessage(currrentSc, SCI_GETTEXTLENGTH, DOCUMENTPTR, DOCUMENTPTR);
		auto raw_data = (CHAR*)SendMessage(currrentSc, SCI_GETRAWTEXT, DOCUMENTPTR, DOCUMENTPTR);
		if(raw_data)
		if(docLength)
		{
			return raw_data;
		}
		else 
		{
			return " ";
		}
	}
	// slow copy-read
	
	docLength = SendMessage(currrentSc, SCI_GETTEXTLENGTH, 0, 0);
	if(!docLength) {
		if(currentkernelType==MINILINK_TYPE) {
			return new CHAR[4]{' ', '0'};
		} else {
			return " ";
		}
	}
	CHAR* buffer;
#ifdef ManageMem
	if(_MDText.mWebView_1 || docLength-1>buffer_cap)
	{
		if(!_MDText.mWebView_1)
		{
			delete[] universal_buffer;
		}
		int cap = nextPowerOfTwo(docLength);
		universal_buffer = new CHAR[cap];
		buffer_cap = cap;
	}
#else
	if(shouldDelete) {
		*shouldDelete = true;
	}
	buffer = new CHAR[docLength+1];
	//buffer = (char*)HeapAlloc(GetProcessHeap(), 0, docLength+1);
#endif
	ScintillaGetText(currrentSc, buffer, 0, docLength);
	buffer[docLength] = '\0';
	//return 0;
	return buffer;
}

void MarkDownTextDlg::syncWebToline(bool force)
{
	if(force || GetUIBoolReverse(0) && GetUIBoolReverse(1))
	{
		int curScintilla;
		SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&curScintilla);
		auto currrentSc = curScintilla?nppData._scintillaSecondHandle:nppData._scintillaMainHandle;
		int line = SendMessage(currrentSc, SCI_GETFIRSTVISIBLELINE, 0, 0);
		if(!force && lastSyncLn==line)
			return;
		CHAR jsSync[64]="syncLn(";
		itoa(line, jsSync+7, 10);
		strcpy(jsSync+strlen(jsSync), ")");
		if(mWebView0) {
			mWebView0->EvaluateJavascript(jsSync);
		}
		lastSyncLn=line;
	}
}

// Global variables
HINSTANCE hInst;

// Initize various browser controls here.
void MarkDownTextDlg::display(bool toShow){
	DockingDlgInterface::display(toShow);

	setClosed(!toShow);

	if(toShow && !mWebView0) 
	{
		presenter.initWebViewImpl(kernelType, this);
	}

	//::SendMessage( _hSelf, SELF_REFRESH, 0, 1);
};

void MarkDownTextDlg::setClosed(bool toClose) {
	_isClosed = toClose;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcMenu->_cmdID, !toClose);
}

void MarkDownTextDlg::AppendPageResidue(char* nxt_st) {
	if(CustomRoutine[0])
	{
		strcpy(nxt_st, CustomRoutine);
		nxt_st+=strlen(CustomRoutine);
		strcpy(nxt_st, "/");
		nxt_st+=1;
	}
	strcpy(nxt_st, "main.js\" onload=init(this)></script></body>");
}

HMENU hMenuEngines=0;

HMENU hMenuZoom=0;

bool bAutoSwitchEngines=0;

std::vector<char*> markdown_ext;

std::vector<char*> html_ext;

bool checkFileExt(vector<char*> ext) { 
	auto len = lstrlen(last_actived);
	TCHAR x;
	for(char* eI:ext) {
		auto elen = strlen(eI);
		if(elen==1&&eI[0]=='*') return true;
		if(len>elen) {
			int i=elen-1;
			while(i>=0&&((x=last_actived[len-elen+i])==eI[i]||i>0&&x==toupper(eI[i])))--i;
			if(i<0) {
				return true;
			}
		}
	}
	return false;
}

bool MarkDownTextDlg::checkRenderMarkdown() { 
	if(bForcePreview)
		return 1;
	if(markdown_ext.size()==0) {
		markdown_ext.push_back(".md");
		markdown_ext.push_back(".md.html");
		markdown_ext.push_back(".svg");
		markdown_ext.push_back(".markdown");
	}
	return checkFileExt(markdown_ext);
}

bool MarkDownTextDlg::checkRenderHtml() { 
	if(bForcePreview)
		return 1;
	if(html_ext.size()==0) {
		html_ext.push_back(".html");
	}
	return checkFileExt(html_ext);
}

bool checkRenderAscii() { 
	return false;
}

void releaseEnginesMenu()
{
	if(hMenuEngines)
	{
		DestroyMenu(hMenuEngines);
		hMenuEngines=0;
	}
}

bool isMDEngineActive() 
{
	return _MDText.CustomRoutineIdx==0;
}

bool isHTMLEngineActive() 
{
	return _MDText.CustomRoutineIdx==1;
}

bool isAsciiEngineActive() 
{
	return _MDText.CustomRoutineIdx==2;
}

void MarkDownTextDlg::RefreshWebview(int source) {
	//TCHAR buffer[256]={0};
	//wsprintf(buffer,TEXT("RefreshWebview source=%d"), source, _MDText.CustomRoutineIdx);
	//::SendMessage(nppData._nppHandle, NPPM_SETSTATUSBAR, STATUSBAR_DOC_TYPE, (LPARAM)buffer);
	if(mWebView0&&NPPRunning)
	{
		LONG_PTR bid = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
		//CustomRoutine = "MDViewer";
		bool fromEditor = source==1;
		char b1=fromEditor&&lastBid==bid, b2=lastBid!=bid;
		//bool autoSwitch = bAutoSwitchEngines&&!fromEditor;
		bool autoSwitch = 1&&source==0;
		if(autoSwitch)
		{
			int toIdx = checkRenderMarkdown()?0:checkRenderHtml()?1:checkRenderAscii()?2:-1;
			if(toIdx>=0&&toIdx!=_MDText.CustomRoutineIdx)
			{
				_MDText.CustomRoutineIdx=toIdx;
				releaseEnginesMenu();
			}
		}
		bool isMarkdown = isMDEngineActive();
		bool isHtml = isHTMLEngineActive();
		//bool isAscii = isAsciiActive();
		mWebView0->updateArticle(bid, _MDText.CustomRoutineIdx, b1, b2);
	}
}

void MarkDownTextDlg::refreshDlg(bool updateList, bool fromEditor) {
	if (isCreated() && isVisible())
	{
		RefreshWebview(fromEditor);
		hasChanged=0;
	} else {
		hasChanged=1;
	}
};


ToolBarButtonUnit PrivateToolBarIconList[] = {
	{IDM_EX_TOGGLE, ICO_EX_TOGGLE, ICO_EX_TOGGLE, ICO_EX_TOGGLE, 0 }, 
	{IDM_EX_DOWN, ICO_EX_DOWN, ICO_EX_DOWN, ICO_EX_DOWN, 0 }, 
	{IDM_EX_UP, ICO_EX_UP, ICO_EX_UP, ICO_EX_UP, 0 }, 
	{IDM_EX_REFRESH, ICO_EX_Refresh, ICO_EX_Refresh, ICO_EX_Refresh, 0 }, 
	{IDM_EX_DELTA, ICO_EX_DELTA, ICO_EX_DELTA, ICO_EX_DELTA, 0 }, 
	{IDM_EX_ZOI, ICO_EX_ZOI, ICO_EX_ZOI, ICO_EX_ZOI, 0 }, 
	{IDM_EX_ZOO, ICO_EX_ZOO, ICO_EX_ZOO, ICO_EX_ZOO, 0 }, 
	{IDM_EX_BOLDEN, ICO_EX_BOLDEN, ICO_EX_BOLDEN, ICO_EX_BOLDEN, 0 }, 
	{IDM_EX_ITALIC, ICO_EX_ITALIC, ICO_EX_ITALIC, ICO_EX_ITALIC, 0 }, 
	{IDM_EX_DEV, ICO_EX_DEV, ICO_EX_DEV, ICO_EX_DEV, 0 }, 
	{IDM_EX_LOCATE, ICO_EX_LOCATE, ICO_EX_LOCATE, ICO_EX_LOCATE, 0 }, 
};

#define ListBoxToolBarSize sizeof(PrivateToolBarIconList)/sizeof(ToolBarButtonUnit)

//	Note: On change, keep sure to change order of IDM_EX_... also in function GetNameStrFromCmd
LPTSTR ListBoxToolBarToolTip[] = {
	TEXT("Options"),
	TEXT("Go Back"),
	TEXT("Go Forward"),
	TEXT("Refresh"),
	TEXT("Alter Engine"),
	TEXT("Zoom In"),
	TEXT("Zoom Out"),
	TEXT("Bold"),
	TEXT("Italic"),
	TEXT("DevTools"),
	TEXT("Sync-Scroll"),
};

LPTSTR ListBoxToolBarToolTip_HAN[] = {
	TEXT("选项"),
	TEXT("后退"),
	TEXT("前进"),
	TEXT("刷新"),
	TEXT("切换引擎"),
	TEXT("放大"),
	TEXT("缩小"),
	TEXT("粗体"),
	TEXT("斜体"),
	TEXT("开发工具"),
	TEXT("同步滚动"),
};

void MarkDownTextDlg::OnToolBarRequestToolTip( LPNMHDR nmhdr )
{
	// Tooltip request of toolbar
	LPTOOLTIPTEXT lpttt;

	lpttt = (LPTOOLTIPTEXT)nmhdr;
	lpttt->hinst = _hInst;

	// Specify the resource identifier of the descriptive
	// text for the given button.
	int resId = (int)lpttt->hdr.idFrom;
	int ToolTipIndex = resId - PrivateToolBarIconList[0]._cmdID;

	auto tooltips=ZH_CN?ListBoxToolBarToolTip_HAN:ListBoxToolBarToolTip;
	TCHAR ToolTipText[MAX_PATH];
	int len = NLGetText( (HINSTANCE)g_hModule, nppData._nppHandle, tooltips[ToolTipIndex], ToolTipText, sizeof(ToolTipText) );
	if ( len == 0 )
	{
		lpttt->lpszText = tooltips[ToolTipIndex];
	}
	else
	{
		lpttt->lpszText = ToolTipText;
	}
}

int BufferIdBeforeClick=0;

TCHAR strHint[500]={0};

int MarkDownTextDlg::getToolbarCommand(POINT &pointer) {
	TBBUTTON tempBtn;
	RECT rect;
	ScreenToClient(toolBar.getHSelf(), &pointer);

	int size = ::SendMessage(toolBar.getHSelf(), TB_BUTTONCOUNT, 0, 0);
	int tc=-1;
	for(int i=0;i<size;i++) {
		::SendMessage(toolBar.getHSelf(), TB_GETITEMRECT, i, reinterpret_cast<LPARAM>(&rect));

		if(PtInRect(&rect, pointer)) {
			auto wh=rect.right-rect.left;
			pointer.x = rect.left-wh*2.5;
			pointer.y = rect.bottom+wh*0.12;
			ClientToScreen(toolBar.getHSelf(), &pointer);
			::SendMessage(toolBar.getHSelf(), TB_GETBUTTON, i, reinterpret_cast<LPARAM>(&tempBtn));
			return tempBtn.idCommand;
		}
	}
	return 0;
}

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
		case WM_CONTEXTMENU:
		{
			if(IsChild(toolBar.getHParent(), (HWND)wParam)) {
				//::MessageBox(NULL, TEXT("111"), TEXT(""), MB_OK);
				POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
				int CMDID = getToolbarCommand(pt);
				if(CMDID)
				{
					OnToolBarCommand(CMDID, 1, &pt);
				}
			}
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
			toolBar.init( _hInst, _hSelf, TB_SMALL, PrivateToolBarIconList, ListBoxToolBarSize );
			//toolBar.init( _hInst, ListBoxPanel.getHSelf(), TB_STANDARD, ListBoxToolBarButtons, ListBoxToolBarSize );
			toolBar.display();
			//toolBar.enlarge();
			ListBoxPanel.SetToolbar( &toolBar );
			toolBar.setCheck(IDM_EX_LOCATE, GetUIBoolReverse(0));
		} break;
		case WM_SIZE:
		case WM_MOVE:
		{
			RECT rc;
			getClientRect(rc);
			int toolbarHeight=toolBar.getHeight();//28;
			if(hBrowser)
			{
				if (currentkernelType==WEBVIEW2_TYPE) {
					mWebView0->notifyWindowSizeChanged(rc);
				}
				::MoveWindow(hBrowser, rc.left, rc.top+toolbarHeight, rc.right, rc.bottom-toolbarHeight,1);
			}
			rc.bottom=toolbarHeight;
			ListBoxPanel.reSizeTo(rc);
			redraw();
		} break;
		case WM_NOTIFY: 
		{
			LPNMHDR	nmhdr	= (LPNMHDR)lParam;
			if (nmhdr->hwndFrom == _hParent)
			{
				switch (LOWORD(nmhdr->code))
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
			else if ( nmhdr->code == TTN_GETDISPINFO )
			{
				OnToolBarRequestToolTip(nmhdr);

				return TRUE;
			}
			else if ( nmhdr->code == RBN_CHEVRONPUSHED )
			{
				NMREBARCHEVRON * lpnm = (NMREBARCHEVRON*)nmhdr;
				if (lpnm->wID == REBAR_BAR_TOOLBAR)
				{
					POINT pt;
					pt.x = lpnm->rc.left;
					pt.y = lpnm->rc.bottom;
					ClientToScreen( nmhdr->hwndFrom, &pt );
					OnToolBarCommand( toolBar.doPopop( pt ) );
					return TRUE;
				}
				break;
			}
			break;
		}
	}
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}

bool getMenuItemNeedsKeep(int mid) {
	switch(mid) {
		case menuBolden:
		case menuPause:
		case menuItalic:
		case menuUnderLine:
		case menuOption:
		return true;
	}
	return false;
}

bool getMenuItemChecked(int mid) {
	switch(mid) {
		case menuUnderLine:
			return false;
	}
	return false;
}

void SwitchEngines(int idx);

void GlobalOnPvMnChecked(HMENU hMenu, int idx);

void simulToolbarMenu(HMENU pluginMenu, RECT *rc, HWND _hSelf, std::vector<FuncItem> & items){
	int cmd = TrackPopupMenu(pluginMenu, TPM_RETURNCMD, rc->left,  rc->top, 0, _hSelf, NULL);

	if(cmd) {
		for(int idx=0,len=items.size();idx<len;idx++) {
			if(items[idx]._cmdID==cmd) {
				if(items[idx]._pFunc==(PFUNCPLUGINCMD)SwitchEngines) {
					SwitchEngines(idx-MDCRST);
				}
				else if(items[idx]._pFunc==(PFUNCPLUGINCMD)GlobalOnPvMnChecked) {
					GlobalOnPvMnChecked(pluginMenu, items[idx]._cmdID);
				}
				else
				{
					items[idx]._pFunc();
				}
				if(pinMenu && getMenuItemNeedsKeep(idx) /*|| idx==nbFunc-2*/) {
					simulToolbarMenu(pluginMenu, rc, _hSelf, items);
				}
			}
		}
	}
}

extern HANDLE				g_hModule;

void PrivateTrackPopup(HWND _hSelf, HMENU pluginMenu, std::vector<FuncItem> items, int CMDID) 
{
	if(pluginMenu) {
		RECT rc, rcImg;
		GetWindowRect(_hSelf, &rc);
		POINT pt;
		GetCursorPos(&pt);
		if(_MDText.getToolbarCommand(pt)==CMDID)
		{
			if(CMDID!=IDM_EX_TOGGLE)
			{
				rc.left=pt.x;
			}
			rc.top=pt.y;
		}
		else
		{
			rc.top+=_MDText.toolBar.getHeight();
		}
		//rc.left=10;
		simulToolbarMenu(pluginMenu, &rc, _hSelf, items);
	}
}

HMENU buildPluginPrivateMenu(std::vector<FuncItem> funcItem)
{
	HMENU pluginMenu = ::CreatePopupMenu();
	unsigned short j = 0, len=funcItem.size();
	for ( ; j < len ; ++j)
	{
		FuncItem & fI = funcItem[j];
		if (fI._pFunc == NULL)
		{
			::InsertMenu(pluginMenu, j, MF_BYPOSITION | MF_SEPARATOR, 0, TEXT(""));
			continue;
		}
		generic_string itemName = fI._itemName;
		::InsertMenu(pluginMenu, j, MF_BYPOSITION, fI._cmdID, itemName.c_str());
		if (fI._init2Check)
			::CheckMenuItem(pluginMenu, fI._cmdID, MF_BYCOMMAND | MF_CHECKED);
	}
	return pluginMenu;
}

void happy(){}

int requestedInvalidSwitch=-1;

bool MDEngineScanned;

void MarkDownTextDlg::saveParameters()
{
	int core=requestedInvalidSwitch>0?requestedInvalidSwitch-1:kernelType;
	PutProfInt("BrowserKernel", core);
	PutProfInt("EngineType", CustomRoutineIdx);
	PutProfString("MDEngine", MDRoutine);
	PutProfString("HTMLEngine", HTMLRoutine);
	PutProfString("ADEngine", ADRoutine);
	PutProfInt("UISettings", UISettings);
	saveProf(g_ModulePath, configFileName);
}

void MarkDownTextDlg::readParameters()
{
	loadProf(g_ModulePath, configFileName);
	kernelType=GetProfInt("BrowserKernel", -1);
	if(kernelType<0||kernelType>3)
	{
		kernelType=-1;
	}
	std::string* val;
	if(val=GetProfString("MDEngine"))
	{
		strcpy(MDRoutine, val->data());
	}
	if(val=GetProfString("HTMLEngine"))
	{
		strcpy(HTMLRoutine, val->data());
	}
	if(val=GetProfString("ADEngine"))
	{
		strcpy(ADRoutine, val->data());
	}
	// Alter the lib path
	if(val=GetProfString("Libcef"))
	{
		auto path=val->data();
		if(PathFileExistsA(path))
		{
			TCHAR* libPath = new TCHAR[MAX_PATH];
			MultiByteToWideChar(CP_ACP, 0, path, -1, libPath, MAX_PATH);
			PathAppend(libPath, TEXT("cefclient.dll"));
			if(PathFileExists(libPath))
			{
				LibPath=libPath;
			}
			else
			{
				delete[] libPath;
			}
		}
	}
	UISettings=GetProfInt("UISettings", 0);
	int inval=GetProfInt("EngineType", -1);
	if(inval>2||inval<-1) {
		inval=-1;
	} else {
		char* srt2cp = inval==0?MDRoutine:inval==1?HTMLRoutine:ADRoutine;
		strcpy(CustomRoutine, srt2cp);
	}
	CustomRoutineIdx=inval;
}

BOOL CALLBACK removeAllChildren(HWND hwndChild, LPARAM lParam)
{
	if(hwndChild==_MDText.getHSelf()||hwndChild==_MDText.getHParent()){
		return 1;
	}
	if(!IsChild((HWND)lParam, hwndChild))
	{
		ShowWindow(hwndChild, SW_HIDE);
		CloseWindow(hwndChild);
		DestroyWindow(hwndChild);
		return 1;
	}
	return 1;
}

void removeAllChildExceptOne(HWND hwnd, HWND ex) {
	EnumChildWindows(hwnd, removeAllChildren, (LPARAM)ex);
}

// Switch Browser Implemetation.
void MarkDownTextDlg::destoryWebViews(bool exit)
{
	if(mWebView0)
	{
		mWebView0->DestroyWebView(exit);
		mWebView0 = 0;
	}
	if(!exit && IsWindow(hBrowser))
	{
		CloseWindow(hBrowser);
		DestroyWindow(hBrowser);
	}
}

void MarkDownTextDlg::switchEngineByIndex(int id)
{
	if(currentkernelType!=id||(GetKeyState(VK_CONTROL) & 0x8000))
	{
		if(id<2&&presenter.wke_mb&&(id+1)^presenter.wke_mb)
		{ // The miniblink kernels are not well designed in this aspect. It requires restarting of the editor.
			requestedInvalidSwitch=id+1;
			if(::MessageBox(nppData._nppHandle, TEXT("Restart is required to switch between wke and mb\r\n\r\nContinue?")
				, TEXT("Need Restart！"), MB_YESNO|MB_DEFBUTTON2)==IDYES)
			{
				//before restart, save parameters in advance.
				saveParameters();
				TCHAR path[MAX_PATH];
				GetModuleFileName(NULL, path, MAX_PATH);
				bool admin=SendMessage(nppData._nppHandle, NPPM_ISADMIN, 0, 0);
				auto ret = (size_t)::ShellExecute(nppData._nppHandle, admin?TEXT("runas"):TEXT("open"), path, TEXT("-multiInst"), 0, SW_SHOW);
				if (ret >= 32)
				{
					::SendMessage(nppData._nppHandle, WM_CLOSE, 0, 0);
				}
			}
			return;
		}
		if(requestedInvalidSwitch)
			requestedInvalidSwitch=0;
		//todo check kernel exists
		RequestedSwitch=true;
		destoryWebViews();
		// remain strange window stubs after destory previous ? window
		//removeAllChildExceptOne(_hSelf, GetParent(toolBar.getHParent())); 
		browser_deferred_create_time=0;
		hBrowser=0;
		kernelType=id;
		lastBid=0;
		display(1);
		for(int i=0;i<=4;i++)
			CheckMenuItem(hMenuEngines, i+1, MF_BYPOSITION|(currentkernelType==i?MF_CHECKED:MF_UNCHECKED));
		SendMessage(_hSelf, WM_SIZE, 0, 0);
		RequestedSwitch=false;
	}
}

void engineToWke(){ _MDText.switchEngineByIndex(0); }

void engineToMb(){ _MDText.switchEngineByIndex(1); }

void engineToChromium(){ _MDText.switchEngineByIndex(2); }

void engineToWebview2(){ _MDText.switchEngineByIndex(3); }

std::vector<FuncItem> ZOOMER;
std::vector<FuncItem> EngineSwicther;
std::vector<wstring> MDEngines;
HMENU hMenuLocate=0;
std::vector<FuncItem> LocateScroll;
#include <iostream>

using namespace std;

bool FindMarkdownEngines(TCHAR* path) {
	//see https://stackoverflow.com/questions/67273/how-do-you-iterate-through-every-file-directory-recursively-in-standard-c#answer-67336
	MDEngines.clear();
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA ffd;
	wstring spec;
	TCHAR tmpPath[MAX_PATH];
	lstrcpy(tmpPath, path);
	auto len=lstrlen(path);

	PathAppend(path, TEXT("//*"));

	hFind = FindFirstFile(path, &ffd);
	if (hFind == INVALID_HANDLE_VALUE)  {
		return false;
	}

	do {
		if (wcscmp(ffd.cFileName, L".") != 0 &&  wcscmp(ffd.cFileName, L"..") != 0) {
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				tmpPath[len]='\0';
				PathAppend(tmpPath, ffd.cFileName);
				PathAppend(tmpPath, TEXT("main.js"));
				if(PathFileExists(tmpPath))
				{
					MDEngines.push_back(ffd.cFileName);
				}
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	if (GetLastError() != ERROR_NO_MORE_FILES) {
		FindClose(hFind);
		return false;
	}

	FindClose(hFind);
	hFind = INVALID_HANDLE_VALUE;

	return true;
}

void CheckMenu(FuncItem* funcItem, bool val)
{
	auto menu = ::GetMenu(nppData._nppHandle);
	::CheckMenuItem(menu, funcItem->_cmdID, MF_BYCOMMAND | (static_cast<BOOL>(val) ? MF_CHECKED : MF_UNCHECKED));
}

void GlobalOnPvMnChecked(HMENU hMenu, int idx) {
	switch(idx) {
		// IDM_EX_LOCATE
		case 260:
		{
			bool val=ToggleUIBool(0, true);
			CheckMenu(funcSync, val);
			if(_MDText.isCreated())
			{
				_MDText.toolBar.setCheck(IDM_EX_LOCATE, val);
			}
		}
		break;
		case 261:
		case 262:
		{
			::CheckMenuItem(hMenu, static_cast<UINT>(idx), MF_BYCOMMAND | (static_cast<BOOL>(ToggleUIBool(idx==261?1:2, true)) ? MF_CHECKED : MF_UNCHECKED));
		}
		break;
		case 263:
		{
			::SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)last_updated);
		}
		break;
		case 264:
			if(_MDText.mWebView0) {
				_MDText.mWebView0->EvaluateJavascript("doScintillo(1)");
			}
		break;
		case 265:
			_MDText.syncWebToline(true);
		break;
	}
}

// Switch internal or custom Markdown/HMTL/ASCIIDoc renderer.
void SwitchEngines(int idx) {
	if(idx<-2) // reset custom engines.
	{
		MDEngineScanned=false;
		MDEngines.clear();
		return;
	}
	else if(idx==-2) // to use internal HTML engine
	{
		_MDText.CustomRoutineIdx=1;
		_MDText.CustomRoutine[0]='\0';
	}
	else if(idx==-1) // to use internal Markdown engine
	{
		_MDText.CustomRoutineIdx=0;
		_MDText.CustomRoutine[0]='\0';
		_MDText.MDRoutine[0]='\0';
	}
	else if(idx>=0&&idx<MDEngines.size()) // to use custom engines
	{
		auto data2set = _MDText.CustomRoutine;
		WideCharToMultiByte(CP_ACP, 0, MDEngines[idx].data(), -1, data2set, MAX_PATH, NULL, NULL);
		if(strncmp_casei(_MDText.CustomRoutine, "ASCII", 5)) {
			// to treat as AsciiDoc engine.
			_MDText.CustomRoutineIdx=2;
			strcpy(_MDText.ADRoutine, data2set);
		} else {
			// to treat as Markdown engine.
			_MDText.CustomRoutineIdx=0;
			strcpy(_MDText.MDRoutine, data2set);
		}
	}
	// update menu checks
	for(int i=-2,len=MDEngines.size();i<len;i++)
	{
		CheckMenuItem(hMenuEngines, i+MDCRST, MF_BYPOSITION|(i==idx?MF_CHECKED:MF_UNCHECKED));
	}
	_MDText.lastBid=0;
	bForcePreview=1;
	_MDText.RefreshWebview(2);
	bForcePreview=0;
}

void ResetZoom(){
	if(_MDText.mWebView0) {
		_MDText.mWebView0->ResetZoom();
	}
}

extern void BoldenText();
extern void TiltText();
extern void UnderlineText();

// |source| 0=click 1=context_menu
void MarkDownTextDlg::OnToolBarCommand(UINT CMDID, char source, POINT* pt)
{ 
	switch ( CMDID ) {
		case IDM_EX_BOLDEN:
		{
			if(source==0)
				BoldenText();
		}
		return;
		case IDM_EX_DEV:
		{
			if(source==0)
			{
				if(mWebView0) {
					// todo concat the path
					mWebView0->ShowDevTools(L"C:\\tmp\\miniblink-20200824\\front_end\\inspector.html");
				}
			}
		}
		return;
		case IDM_EX_ITALIC:
		{
			if(source==0)
				TiltText();
		}
		return;
		case IDM_EX_TOGGLE:
		{
			HMENU hMenuPlugin = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETPLUGINMENU, 0 , (LPARAM)g_hModule);

			if(!hMenuPlugin) {
				//pluginMenu = (HMENU)::GetMenu(nppData._nppHandle);
				if(!hMenuPlugin) {

					//::MessageBox(NULL, TEXT("111"), TEXT(""), MB_OK);

				}
			}
			PrivateTrackPopup(_hSelf, hMenuPlugin, funcItems, CMDID);
		}
		return;
		case IDM_EX_DOWN:
			if(source==0&&mWebView0){
				mWebView0->GoBack();
			}
		return;
		case IDM_EX_UP:
			if(mWebView0) {
				mWebView0->GoForward();
			}
		return;
		case IDM_EX_REFRESH:
			if(source==0 && mWebView0)
			{
				RefreshWebview();
			}
		return;
		case IDM_EX_ZOO:
			if(source==0)
			{
				if(mWebView0) {
					mWebView0->ZoomOut();
				}
				return;
			}
		case IDM_EX_ZOI:
		{
			if(source==0)
			{
				if(mWebView0) {
					mWebView0->ZoomIn();
				}
			}
			else if(source==1)
			{
				if(ZOOMER.size()==0)
				{
					ZOOMER.resize(1);
					ZOOMER.at(0)={TEXT(""), ResetZoom, 161, false, 0};
					lstrcpy(ZOOMER.at(0)._itemName, ZH_CN?TEXT("重置"):TEXT("Reset"));
				}
				if(hMenuZoom==0)
				{
					hMenuZoom = buildPluginPrivateMenu(ZOOMER);
				}
				PrivateTrackPopup(_hSelf, hMenuZoom, ZOOMER, CMDID);
			}
		}
		return;
		case IDM_EX_DELTA:
		{
			//todo localize
			if(EngineSwicther.size()==0)
			{
				EngineSwicther.resize(MDCRST);
				EngineSwicther.at(0)={TEXT(""), happy, 61, false, 0};
				lstrcpy(EngineSwicther.at(0)._itemName, ZH_CN?TEXT("切换浏览器内核："):TEXT("Switch Browser Kernel :"));
				EngineSwicther.at(1)={TEXT("Miniblink-wke"), engineToWke, 62, false, 0};
				EngineSwicther.at(2)={TEXT("Miniblink-mb"), engineToMb, 63, false, 0};
				EngineSwicther.at(3)={TEXT("Chromium-Embeded ( Recommended )"), engineToChromium, 64, false, 0};
				EngineSwicther.at(4)={TEXT("Webview2"), engineToWebview2, 65, false, 0};
				EngineSwicther.at(5)={TEXT(""), 0, 0, false, 0};
				EngineSwicther.at(6)={TEXT(""), (PFUNCPLUGINCMD)SwitchEngines, 66, false, 0};
				lstrcpy(EngineSwicther.at(6)._itemName, ZH_CN?TEXT("切换渲染引擎："):TEXT("Switch Markdown Engine :"));
				EngineSwicther.at(7)={TEXT("HTML"), (PFUNCPLUGINCMD)SwitchEngines, 67, false, 0};
				EngineSwicther.at(8)={TEXT("md.html"), (PFUNCPLUGINCMD)SwitchEngines, 68, false, 0};
			}
			bool rebuildMenu = hMenuEngines==0||!MDEngineScanned;
			if(hMenuEngines==0)
			{
				hMenuEngines = buildPluginPrivateMenu(EngineSwicther);
			}
			if(!MDEngineScanned)
			{
				TCHAR scriptPath[MAX_PATH];
				GetModuleFileName((HMODULE)g_hModule, scriptPath, MAX_PATH);
				PathRemoveFileSpec(scriptPath);
				FindMarkdownEngines(scriptPath);
				MDEngineScanned=1;
			}
			if(rebuildMenu)
			{
				EngineSwicther.resize(MDCRST+MDEngines.size());
				while(GetMenuItemCount(hMenuEngines)>MDCRST)
					RemoveMenu(hMenuEngines, MDCRST, MF_BYPOSITION);
				int foundCheck=0;
				if(CustomRoutine[0])
				{
					MultiByteToWideChar(CP_ACP, 0, CustomRoutine, -1, path_buffer, MAX_PATH);
				}
				else if(isHTMLEngineActive()) {
					CheckMenuItem(hMenuEngines, MDCRST-2, MF_BYPOSITION|MF_CHECKED);
				}
				else // 默认引擎 md.html
				{
					CheckMenuItem(hMenuEngines, MDCRST-1, MF_BYPOSITION|MF_CHECKED);
				}
				for(int i=0,ii,len=MDEngines.size();i<len;i++)
				{
					ii=MDCRST+i;
					EngineSwicther.at(ii)={TEXT(""), (PFUNCPLUGINCMD)SwitchEngines, 60+MDCRST+i, false, 0};
					auto data=MDEngines[i].data();
					lstrcpy(EngineSwicther.at(ii)._itemName, data);
					::InsertMenu(hMenuEngines, ii, MF_BYPOSITION, EngineSwicther.at(ii)._cmdID, data);
					if(CustomRoutine[0]&&!foundCheck&&lstrcmp(path_buffer, data)==0)
					{
						foundCheck=ii;
					}
				}
				if(foundCheck)
				{
					CheckMenuItem(hMenuEngines, foundCheck, MF_BYPOSITION|MF_CHECKED);
				}
			}
			CheckMenuItem(hMenuEngines, currentkernelType+1, MF_BYPOSITION|MF_CHECKED);
			PrivateTrackPopup(_hSelf, hMenuEngines, EngineSwicther, CMDID);
		}
		return;
		case IDM_EX_LOCATE:
		{
			if(source==0)
			{
				GlobalOnPvMnChecked(0, 260);
			}
			else if(source==1)
			{
				if(LocateScroll.size()==0)
				{
					LocateScroll.resize(5);
					int i=0;
					LocateScroll.at(i++)={TEXT("Sync Text -> Webview"), (PFUNCPLUGINCMD)GlobalOnPvMnChecked, 261, false, 0};
					LocateScroll.at(i++)={TEXT("-->  Sync Now (&D)"), (PFUNCPLUGINCMD)GlobalOnPvMnChecked, 265, false, 0};
					LocateScroll.at(i++)={TEXT("Sync Text <- Webview"), (PFUNCPLUGINCMD)GlobalOnPvMnChecked, 262, false, 0};
					LocateScroll.at(i++)={TEXT("<--  Sync Now (&A)"), (PFUNCPLUGINCMD)GlobalOnPvMnChecked, 264, false, 0};
					LocateScroll.at(i++)={TEXT("Locate current file"), (PFUNCPLUGINCMD)GlobalOnPvMnChecked, 263, false, 0};
				}
				if(hMenuLocate==0)
				{
					hMenuLocate = buildPluginPrivateMenu(LocateScroll);
					::CheckMenuItem(hMenuLocate, static_cast<UINT>(261), MF_BYCOMMAND | (static_cast<BOOL>(GetUIBoolReverse(1) ? MF_CHECKED : MF_UNCHECKED)));
					::CheckMenuItem(hMenuLocate, static_cast<UINT>(262), MF_BYCOMMAND | (static_cast<BOOL>(GetUIBoolReverse(2)) ? MF_CHECKED : MF_UNCHECKED));
				}
				PrivateTrackPopup(_hSelf, hMenuLocate, LocateScroll, CMDID);
			}
		}
		return;
	}
}