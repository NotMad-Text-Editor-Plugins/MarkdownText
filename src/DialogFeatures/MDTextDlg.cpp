/*
* Copyright 2020 KnIfER
* Encapsulate miniblink, libcef and webview2 in one c++ file.
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

#include "MDTextDlg.h"
#include "PluginDefinition.h"
#include <ProfileStd.h>
#include "ArticlePresenter.h"
#include "MDTextToolbar.h"
#include "WarningDlg.hpp"
#include "SU.h"

const TCHAR* configFileName = TEXT("MarkDownText.ini");

#define MDCRST 9

UINT ScintillaGetText(HWND hWnd, char *text, INT start, INT end)
{
	Sci_TextRange tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText  = text;
	return (UINT)::SendMessage(hWnd, SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr));
}

void MarkDownTextDlg::doScintillaScroll(int ln)
{
	int currrentSc;
	SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currrentSc);
	curScintilla = currrentSc?nppData._scintillaSecondHandle:nppData._scintillaMainHandle;
	if (ln<0)
	{
		int totalLns = SendMessage(curScintilla, SCI_GETLINECOUNT, 0, 0);
		ln = -ln/100000.f*totalLns;
	}
	ln = SendMessage(curScintilla, SCI_VISIBLEFROMDOCLINE, lastSyncLn=ln, 0);
	SendMessage(curScintilla, SCI_SETFIRSTVISIBLELINE, ln, 0);
}

CHAR* MarkDownTextDlg::loadSourceAsset(uptr_t bid, const char* pathA, DWORD & dataLen, bool * shouldDelete)
{
	TCHAR path[MAX_PATH];
	MultiByteToWideChar (CP_ACP, 0, pathA, strlen (pathA) + 1, path, MAX_PATH-1) ;
	TCHAR SrcPath[MAX_PATH]={0};
	if(!bid) {
		bid = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
	}
	if(bid) {
		::SendMessage(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, (WPARAM)bid, (LPARAM)SrcPath);
		::PathRemoveFileSpec(SrcPath);
		::PathAppend(SrcPath, path);
		::PathCanonicalize(path, SrcPath);
		TCHAR* AssetPath = path;

		if(PathFileExists(AssetPath)) 
		{
			if (GetUIBool(8))
			{
				// try to read in-memory buffer.
				if(!legacy)
				{
					LONG_PTR membid = SendMessage(nppData._nppHandle, NPPM_GETBUFFERIDFROMPATH, (WPARAM)AssetPath, 0);
					if (membid && buffersMap.find(membid)!=buffersMap.end())
					{
						// emplace holder.
						chainedBuffersMap.emplace(membid);
						LONG_PTR DOCUMENTPTR = SendMessage(nppData._nppHandle, NPPM_GETDOCUMENTPTR, membid, membid);
						if (DOCUMENTPTR)
						{
							dataLen = SendMessage(nppData._scintillaMainHandle, SCI_GETTEXTLENGTH, DOCUMENTPTR, DOCUMENTPTR);
							CHAR* raw_data;
							if(dataLen)
							{
								raw_data = (CHAR*)SendMessage(nppData._scintillaMainHandle, SCI_GETRAWTEXT, DOCUMENTPTR, DOCUMENTPTR);
							}
							else 
							{
								dataLen = 1;
								raw_data = " ";
							}
							if (raw_data)
							{
								if(currentkernelType==MINILINK_TYPE)
								{
									CHAR* buffer = new CHAR[dataLen];
									memcpy(buffer, raw_data, dataLen);
									return buffer;
								}
								return raw_data;
							}
						}
					}
				}
				else
				{
					// no fallback. In legacy mode, only the current buffer is available
					//	, which makes chained updating pointless.
				}
			}

			if(shouldDelete) 
			{
				*shouldDelete = true;
			}

			HANDLE hFile = CreateFile(AssetPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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
	//if(false)
	if (!strcmp("main.js", path))
	{
		CHAR* data;
		DWORD len=0;
		if (CPaintManagerUI::ExtractItem(TEXT("main.js"), &data, len)&&len)
		{
			dataLen = len;
			return data;
		}
	}
	if (!strcmp("darkmode.js", path))
	{
		CHAR* data;
		DWORD len=0;
		if (CPaintManagerUI::ExtractItem(TEXT("darkmode.js"), &data, len)&&len)
		{
			dataLen = len;
			return data;
		}
	}
	else if (!strcmp("ui.js", path))
	{
		CHAR* data;
		DWORD len=0;
		if (CPaintManagerUI::ExtractItem(TEXT("ui.js"), &data, len)&&len)
		{
			dataLen = len;
			return data;
		}
	}
	CHAR ResPath[MAX_PATH];
	if(rnd_res && PathFileExistsA(rnd_res))
	{
		strcpy(ResPath, rnd_res);
	}
	else
	{
		::GetModuleFileNameA((HINSTANCE)g_hModule, ResPath, MAX_PATH);
		::PathRemoveFileSpecA(ResPath);
	}
	::PathAppendA(ResPath, path);
	//::MessageBoxA(NULL, ResPath, (""), MB_OK);
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
	int currrentSc;
	SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currrentSc);
	curScintilla = currrentSc?nppData._scintillaSecondHandle:nppData._scintillaMainHandle;

	if(shouldDelete) {
		*shouldDelete = false;
	}

	if(bid && !legacy) //  && !legacy
	{
		if (_MDText.buffersMap.find(bid)!=_MDText.buffersMap.end())
		{
			// fast-read the original memory data
			LONG_PTR DOCUMENTPTR = SendMessage(nppData._nppHandle, NPPM_GETDOCUMENTPTR, bid, bid);
			if (DOCUMENTPTR)
			{
				docLength = SendMessage(nppData._scintillaMainHandle, SCI_GETTEXTLENGTH, DOCUMENTPTR, DOCUMENTPTR);
				CHAR* raw_data;
				if(docLength)
				{
					raw_data = (CHAR*)SendMessage(nppData._scintillaMainHandle, SCI_GETRAWTEXT, DOCUMENTPTR, DOCUMENTPTR);
				}
				else 
				{
					docLength = 1;
					raw_data = " ";
				}
				if (raw_data)
				{
					if(currentkernelType==MINILINK_TYPE)
					{
						CHAR* buffer = new CHAR[docLength];
						memcpy(buffer, raw_data, docLength);
						return buffer;
					}
					return raw_data;
				}
			}
		}
	}
	// slow copy-read
	
	docLength = SendMessage(curScintilla, SCI_GETTEXTLENGTH, 0, 0);
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
	if(shouldDelete) 
	{
		*shouldDelete = true;
	}
	buffer = new CHAR[docLength+1];
	//buffer = (char*)HeapAlloc(GetProcessHeap(), 0, docLength+1);
#endif
	ScintillaGetText(curScintilla, buffer, 0, docLength);
	buffer[docLength] = '\0';
	//return 0;
	return buffer;
}

void MarkDownTextDlg::syncWebToline(bool force)
{
	if(force || GetUIBoolReverse(0) && GetUIBoolReverse(1))
	{
		int currrentSc;
		SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currrentSc);
		curScintilla = currrentSc?nppData._scintillaSecondHandle:nppData._scintillaMainHandle;
		int line = SendMessage(curScintilla, SCI_GETFIRSTVISIBLELINE, 0, 0);
		line = SendMessage(curScintilla, SCI_DOCLINEFROMVISIBLE, line, 0);
		if(!force && lastSyncLn==line)
			return;
		int totalLns = SendMessage(curScintilla, SCI_GETLINECOUNT, 0, 0);
		float percetage=totalLns==0?-1:line*1.f/totalLns;
		CHAR jsSync[64]="syncLn(";
		//itoa(line, jsSync+7, 10);
		//strcpy(jsSync+strlen(jsSync), ")");
		sprintf(jsSync, "syncLn(%d,%.2f)", line, percetage);
		if(mWebView0) {
			mWebView0->EvaluateJavascript(jsSync);
		}
		lastSyncLn=line;
	}
}

// Initize various browser controls here.
void MarkDownTextDlg::display(bool toShow)
{
	DockingDlgInterface::display(toShow);

	setClosed(!toShow);

	if(toShow && !mWebView0) 
	{
		if(presenter.initWebViewImpl(kernelType, this, true))
		{
			displayInstallGuide();
		}
	}

	//::SendMessage( _hSelf, SELF_REFRESH, 0, 1);
};

void MarkDownTextDlg::setClosed(bool toClose) 
{
	_isClosed = toClose;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcMenu->_cmdID, !toClose);
}

void MarkDownTextDlg::AppendPageResidue(char* nxt_st) 
{
	if(RendererTypeIdx>=0&&RendererTypeIdx<=2&&RendererNames[RendererTypeIdx][0])
	{
		strcpy(nxt_st, RendererNames[RendererTypeIdx]);
		nxt_st+=strlen(RendererNames[RendererTypeIdx]);
		strcpy(nxt_st, "/");
		nxt_st+=1;
	}
	strcpy(nxt_st, "main.js\" onload=init(this)></script></body>");
}

// 
// Engine Switch functions
// 

void MarkDownTextDlg::releaseEnginesMenu()
{
	if(hMenuEngines)
	{
		DestroyMenu(hMenuEngines);
		hMenuEngines=0;
	}
}

bool checkFileForExt(vector<string> & ext) 
{ 
	auto len = lstrlen(last_actived);
	TCHAR x;
	for(string & eI:ext) {
		auto elen = eI.length();
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

bool MarkDownTextDlg::checkFileExt(int type) 
{ 
	return checkFileForExt(*all_exts[type]);
}

std::string* MarkDownTextDlg::setLibPathAt(std::vector<std::string*> & paths, int idx, char* newpath, char * key)
{
	char TmpLibPath[MAX_PATH];
	sprintf(TmpLibPath, key, idx+1);
	PutProfString(TmpLibPath, newpath);
	std::string* val=GetProfString(TmpLibPath);
	//::MessageBoxA(NULL, (*val).data(), TmpLibPath, MB_OK);
	paths[idx] = val;
	return val;
}

void MarkDownTextDlg::refreshDarkMode() 
{
	//redraw(true);
	NppDarkMode::refreshDarkMode(getHSelf());

	if (isCreated())
	{
		::SendMessage(getHSelf(), WM_SIZE, 0, 0);
		NppDarkMode::setDarkScrollBar(hBrowser);
	}

	if (mWebView0)
	{
		int editorBgColor = NppDarkMode::isEnabled()?
			SendMessage(nppData._nppHandle, NPPM_GETEDITORDEFAULTBACKGROUNDCOLOR, 0, 0)
			:0;
		CHAR buffer[200]={0};
		sprintf(buffer,"if(!window._RDM) loadJs(\"http://mdbr/darkmode.js\", function(){window._RDM(%d)}); else window._RDM(%d)"
			, editorBgColor, editorBgColor);
		mWebView0->EvaluateJavascript(buffer);
	}
	if(checkFileExt(0))
	{
		LanguageToMarkdown();
	}
	//NppDarkMode::setDarkScrollBar
}

void MarkDownTextDlg::RefreshWebview(int source) 
{
	//LogIs("RefreshWebview source=%d", source, _MDText.RendererTypeIdx);
	if(mWebView0&&NPPRunning)
	{
		LONG_PTR bidBk = lastBid;
		LONG_PTR bid = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
		//CustomRoutine = "MDViewer";
		bool fromEditor = source==1;
		char b1=fromEditor&&lastBid==bid, b2=lastBid!=bid;
		b2 = lastBid!=bid&&!fromEditor;
		if (source==3)
		{
			b2 = true;
		}
		bool bNeedUpdate = b2;
		//bool autoSwitch = bAutoSwitchEngines&&!fromEditor;
		bool autoSwitch = GetUIBoolReverse(7)&&source==0;
		bool bShouldCheck=true;

		if(!fromEditor && !checkFileExt(RendererTypeIdx))
		{
			// 1st priority, auto-switch
			// auto switch render type according to current actived document's file name extension
			// and extensions preset.
			if (autoSwitch)
			{
				for (int toIdx = 0; toIdx < 3; toIdx++)
				{
					if (toIdx!=RendererTypeIdx && checkFileExt(toIdx))
					{
						RendererTypeIdx=toIdx;
						releaseEnginesMenu();
						bShouldCheck = false;
						break;
					}
				}
			}
			// 2nd priority, mannual-switch memory
			if (bShouldCheck)
			{
				auto prefIdx = prefRndTyps.find(bid);
				if (prefIdx!=prefRndTyps.end())
				{
					bool hmRndTyp = RendererTypeIdx==prefIdx->second;
					//if (autoSwitch||hmRndTyp)
					{
						if (!hmRndTyp)
						{
							RendererTypeIdx=prefIdx->second;
							releaseEnginesMenu();
						}
						b2=true;
						bShouldCheck = false;
					}
				}
			}
			if (bShouldCheck)
			{
				b2=false;
			}
			else
			{
				b2=true;
			}
		}
		if(b2 && bShouldCheck)
		{
			// perform the check again if needed.
			b2 = checkFileExt(RendererTypeIdx);
		}
		//if(false)
		if (bNeedUpdate&&!b2)
		{
			if (bForcePreview)
			{
				if (RendererTypeIdx!=defaultRenderer) // &&GetUIBoolReverse(7)
				{
					// apply default renderer type
					RendererTypeIdx = defaultRenderer;
					releaseEnginesMenu();
				}
				prefRndTyps[bid] = RendererTypeIdx;
				//LogIs("RendererTypeIdx %d, %d", RendererTypeIdx, prefRndTyps[bid]);
				b2=true;
			}
		}
		mWebView0->updateArticle(bid, RendererTypeIdx, b1, b2);
		if (lastBid!=bidBk)
		{
			chainedBuffersMap.clear();
		}
	}
}

void MarkDownTextDlg::CheckChaninedUpdate(LONG_PTR BID) 
{
	if (mWebView0 
		&& chainedBuffersMap.find(BID)!=chainedBuffersMap.end())
	{
		mWebView0->updateArticle(lastBid, RendererTypeIdx, false, true);
	}
}

void MarkDownTextDlg::refreshDlg(bool updateList, bool fromEditor) 
{
	if (isCreated() && isVisible())
	{
		RefreshWebview(fromEditor);
		hasChanged=0;
	} else {
		hasChanged=1;
	}
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

	if (ToolTipIndex<0||ToolTipIndex>ListBoxToolBarSize)
	{
		return;
	}

	auto tooltips=ToolBarToolTips;
	auto locText = GetLocalText(getLocaliseMap(), ToolBarToolTipsId[ToolTipIndex]);

	auto tips = tooltips[ToolTipIndex];
	if(locText)
	{
		tips=ToolBarToolTipsTranslations[ToolTipIndex];
		if (!tips)
		{
			tips = ToolBarToolTipsTranslations[ToolTipIndex] = new TCHAR[64];
		}
		int len = MultiByteToWideChar(CP_ACP, 0, locText->c_str(), -1, tips, 63);
		tips[len] = '\0';
	}

	//TCHAR ToolTipText[MAX_PATH];
	//int len = NLGetText( (HINSTANCE)g_hModule, nppData._nppHandle, tips, ToolTipText, sizeof(ToolTipText) );
	//if ( len == 0 )
	//{
		lpttt->lpszText = tips;
	//}
	//else
	//{
	//	lpttt->lpszText = ToolTipText;
	//}
}

int MarkDownTextDlg::getToolbarCommand(POINT &pointer) 
{
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

int mLastW, mLastH;
bool resizing;
bool darkInit = true;

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
		//case WM_USER + 59://NPPM_INTERNAL_REFRESHDARKMODE
		//{
		//	LogIs(2, "NPPM_INTERNAL_REFRESHDARKMODE");
		//}
		//break;
		//case WM_THEMECHANGED:
		//	LogIs(2, "WM_THEMECHANGED");
		//	break;
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
			int W = rc.right - rc.left;
			int H = rc.bottom - rc.top;
			if (W!=0 && H!=0)
			{
				int toolbarHeight=toolBar.getHeight();//28;
				bool showIGUnit = installGuide;// && WM_SIZE==message;
				if (showIGUnit)
				{
					//::SetWindowPos(installGuide->GetHWND(),HWND_TOP,0, 0, 0, 0, SWP_SHOWWINDOW);
					::MoveWindow(installGuide->GetHWND(), rc.left, rc.top+toolbarHeight, rc.right, rc.bottom-toolbarHeight,1);
					if (hBrowser && IsWindowVisible(hBrowser))
					{
						ShowWindow(hBrowser, SW_HIDE);
					}
				}
				if(hBrowser)
				{
					if (currentkernelType==WEBVIEW2_TYPE) {
						mWebView0->notifyWindowSizeChanged(rc);
					}
					//LogIs(2, "MoveWindow %ld %ld %ld %ld", rc.left, rc.top, rc.right, rc.bottom);
					//ShowWindow(hBrowser, SW_SHOW);
					::MoveWindow(hBrowser, rc.left, rc.top+toolbarHeight, rc.right, rc.bottom-toolbarHeight,0);
					if (darkInit)
					{
						//NppDarkMode::setDarkScrollBar(hBrowser);
						darkInit = false;
					}
				}
				rc.bottom=toolbarHeight;
				ListBoxPanel.reSizeTo(rc);
				::UpdateWindow(_hSelf);
				// redraw();
			}
			//mLastW = W;
			//mLastH = H;
		} break;

		//case WM_EXITSIZEMOVE:
		//{
		//	LogIs(2, "resizing %d", resizing);
		//} break;
		//
		//case WM_ENTERSIZEMOVE:
		//{
		//	LogIs(2, "resizing %d", resizing);
		//} break;
		//
		//case WM_SYSCOMMAND :
		//{
		//	switch (wParam & 0xfff0)
		//	{
		//	case SC_SIZE:
		//		LogIs(2, "resizing %d", resizing);
		//		return TRUE;
		//	}
		//}

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
				case DMN_FLOAT:
				case DMN_DOCK:
					{
						//PostMessage(_hSelf, WM_SIZE, 0, 0);
						//::InvalidateRect(hBrowser, nullptr, TRUE);

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
			else if ( nmhdr->code == -1 && installGuide )
			{
				installGuide = nullptr;
				if (hBrowser) // && IsWindowVisible(hBrowser)
				{
					ShowWindow(hBrowser, SW_SHOW);
					if (isShowGuidePredateArticle)
					{
						RefreshWebview(3);
					}
				}
				return TRUE;
			}
			break;
		}
	}
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}

bool getMenuItemNeedsKeep(int mid) 
{
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

bool getMenuItemChecked(int mid) 
{
	switch(mid) {
		case menuPause:
			return GetUIBool(3);
		case menuChained:
			return GetUIBool(8);
		case menuSync:
			return GetUIBoolReverse(0);
		case menuOption:
			return _MDText.isVisible();
	}
	return false;
}

void SwitchEnginesStatic(int idx) {
	_MDText.SwitchEngines(idx);
}

void GlobalOnPvMnCheckedStatic(HMENU hMenu, int idx)
{
	_MDText.GlobalOnPvMnChecked(hMenu, idx);
}

#define ID_PLUGINS_CMD 22000
#define ID_PLUGINS_CMD_LIMIT 22500

void simulToolbarMenu(HMENU pluginMenu, RECT *rc, HWND _hSelf, FuncItem* items)
{
	int cmd = TrackPopupMenu(pluginMenu, TPM_RETURNCMD, rc->left,  rc->top, 0, _hSelf, NULL);

	if(cmd) {
		if (cmd >= ID_PLUGINS_CMD && cmd < ID_PLUGINS_CMD_LIMIT
			|| cmd>=hToolsStartId && cmd<hToolsStartId+hToolsSz
			)
		{
			SendMessage(nppData._nppHandle, WM_COMMAND, cmd, 0);
		} 
		else {
			for(int idx=0,len=sizeof(*items);idx<len;idx++) {
				if(items[idx]._cmdID==cmd) {
					if(items[idx]._pFunc==(PFUNCPLUGINCMD)SwitchEnginesStatic) {
						SwitchEnginesStatic(idx-MDCRST);
					}
					else if(items[idx]._pFunc==(PFUNCPLUGINCMD)GlobalOnPvMnCheckedStatic) {
						GlobalOnPvMnCheckedStatic(pluginMenu, items[idx]._cmdID);
					}
					else
					{
						items[idx]._pFunc();
					}
					break;
				}
			}
		}

		////todo fix
		//if(pinMenu && getMenuItemNeedsKeep(idx) /*|| idx==nbFunc-2*/) {
		//	simulToolbarMenu(pluginMenu, rc, _hSelf, items);
		//}
	}
}

void PrivateTrackPopup(HWND _hSelf, HMENU pluginMenu, FuncItem* items, int CMDID) 
{
	if(pluginMenu) {
		RECT rc;
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

void MarkDownTextDlg::saveParameters()
{
	int core=requestedInvalidSwitch>0?requestedInvalidSwitch-1:kernelType;
	PutProfInt("BrowserType", core);
	PutProfInt("RenderType", RendererTypeIdx);
	PutProfInt("DefaultType", defaultRenderer);
	PutProfString("MDEngine", MDRoutine);
	PutProfString("ADEngine", ADRoutine);
	PutProfInt("UISettings", UISettings);
	PutProfInt("LibCef", LibCefSel);
	PutProfInt("LibWke", LibWkeSel);
	PutProfInt("LibMb", LibMbSel);
	WideCharToMultiByte(CP_ACP, 0, currentLanguageFile.data(), -1, ADRoutine, MAX_PATH_HALF-1, NULL, NULL);
	PutProfString("locale", ADRoutine);
	saveProf(g_ModulePath, configFileName);
}

void MarkDownTextDlg::readLibPaths(int & max, std::vector<std::string*> & LibPaths, char* key, int & sel, char* selkey)
{
	max = 0;
	std::string* val;
	LibPaths.resize(max);
	char TmpLibPath[MAX_PATH_HALF];
	for(int i=0;;i++) {
		sprintf(TmpLibPath, key, i+1);
		if(val=GetProfString(TmpLibPath))
		{
			LibPaths.push_back(val);
			max++;
		}
		else if(i<3)
		{
			LibPaths.push_back(NULL);
			max++;
		}
		else
		{
			break;
		}
	}
	sel = GetProfInt(selkey, 0);
	if(sel>=max) sel=max-1;
	if(sel<=0) sel=0;
}

void StrToExtArr(std::vector<string> & arr, char* data, int dataLen)
{
	arr.clear();
	int cc=0;
	for (int pos = dataLen-2; pos>=-1; pos--)
	{
		if(data[pos]==' '||pos==-1)
		{
			if(pos>0)
			{
				data[pos]='\0';
			}
			arr.push_back(data+pos+1);
			if(data[pos+1]!='.')
			{
				arr[cc]="."+arr[cc];
			}
			cc++;
		}
	}
	for (int i = 0; i < dataLen; i++) 
		if(data[i]=='\0') data[i]=' ';
}

void MarkDownTextDlg::readExtensions(int channel, string * ret)
{
	if(!extCtx) 
	{
		extCtx = new ReadExtContext[] {
			 {"Ext_MD", "md md.html svg markdown"}
			,{"Ext_HTML", "html"}
			,{"Ext_AD", "ascii"}
		};
	}
	string tmp;
	for (int id = 0; id < 3; id++)
	{
		if(ret&&channel==id||channel==-1||channel==id) {
			string* val = GetProfString(extCtx[id].key);
			if(!val||val->length()==0)
			{
				tmp = extCtx[id].defVal;
				val = &tmp;
			}
			if(ret)
			{
				*ret = val->data();
				return;
			}
			else
			{
				StrToExtArr(*all_exts[id], (char*)val->data(), val->length());
			}
		}
	}
}

CHAR* _dataPath = NULL;

void MarkDownTextDlg::readParameters()
{
	loadProf(g_ModulePath, configFileName);
	kernelType=GetProfInt("BrowserType", -1);
	if(kernelType<0||kernelType>3)
	{
		kernelType=-1;
	}
	std::string* val;
	if(val=GetProfString("rnd_res"))
	{
		rnd_res = val->data();
	}
	if(val=GetProfString("MDEngine"))
	{
		strcpy(MDRoutine, val->data());
	}
	if(val=GetProfString("ADEngine"))
	{
		strcpy(ADRoutine, val->data());
	}
	if(val=GetProfString("locale"))
	{
		TCHAR tmp[MAX_PATH_HALF];
		MultiByteToWideChar(CP_ACP, 0, val->data(), -1, tmp, MAX_PATH_HALF-1);
		currentLanguageFile = tmp;
		localeSet=true;
	} else {
		localeSet=false;
	}

	readExtensions(-1, NULL);

	UISettings=GetProfInt("UISettings", 0);
	readLibPaths(maxPathHistory, LibPaths, "LibPath%d", LibCefSel, "LibCef");
	//readLibPaths(maxPathHistory1, WkePaths, "WkePath%d", LibWkeSel, "LibWke");
	readLibPaths(maxPathHistory2, MbPaths, "MbPath%d", LibMbSel, "LibMb");

	int inval=GetProfInt("RenderType", 0);
	if(inval>2||inval<0) {
		inval=0;
	}
	lastPickedRenderer=RendererTypeIdx=inval;

	inval=GetProfInt("DefaultType", 0);
	if(inval>2||inval<0) {
		inval=0;
	}
	defaultRenderer = inval;


	if(val=GetProfString("DataPath"))
	{
		_dataPath = (CHAR*)val->data();
	}
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

void removeAllChildExceptOne(HWND hwnd, HWND ex) 
{
	EnumChildWindows(hwnd, removeAllChildren, (LPARAM)ex);
}

// Switch Browser Implemetation.
void MarkDownTextDlg::destroyWebViews(bool exit)
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

void MarkDownTextDlg::switchWebViewByIndex(int id)
{
	if(currentkernelType!=id||(GetKeyState(VK_CONTROL) & 0x8000))
	{
		darkInit = true;
		if(id<2&&presenter.wke_mb&&(id+1)^presenter.wke_mb)
		{ // The miniblink kernels are not well designed in this aspect. It requires restarting of the editor.
			requestedInvalidSwitch=id+1;
			if(::MessageBox(nppData._nppHandle, TEXT("Restart is required to switch between wke and mb\r\n\r\nContinue?")
				, TEXT("Need Restart！| 需要重启"), MB_YESNO|MB_DEFBUTTON2)==IDYES)
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

		RequestedSwitch=true;
		//removeAllChildExceptOne(_hSelf, GetParent(toolBar.getHParent())); 
		browser_deferred_create_time=0;

		ArticlePresenter* mwold = mWebView0;
		HWND hold = hBrowser;

		if (installGuide)
		{
			installGuide->Close();
		}

		if(presenter.initWebViewImpl(id, this, false)==0)
		{
			lastBid=0;
			kernelType=id;
			if(mwold)
			{
				mwold->DestroyWebView();
			}
			if(IsWindow(hold))
			{
				CloseWindow(hold);
				DestroyWindow(hold);
			}
		}
		display(1);

		for(int i=0;i<=4;i++)
			CheckMenuItem(hMenuEngines, i+1, MF_BYPOSITION|(currentkernelType==i?MF_CHECKED:MF_UNCHECKED));
		SendMessage(_hSelf, WM_SIZE, 0, 0);
		RequestedSwitch=false;
	}
}

void engineToWke(){ _MDText.switchWebViewByIndex(0); }

void engineToMb(){ _MDText.switchWebViewByIndex(1); }

void engineToChromium(){ _MDText.switchWebViewByIndex(2); }

void engineToWebview2(){ _MDText.switchWebViewByIndex(3); }

HMENU GetLegacyPluginMenu() 
{
	HMENU pluginMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU , (LPARAM)0);
	int cc = GetMenuItemCount(pluginMenu);
	int targetLen = lstrlen(NPP_PLUGIN_NAME);
	for (size_t i = 0; i < cc; i++)
	{
		TCHAR tmpTexts[MAX_PATH_HALF];
		int namePathLen = ::GetMenuString(pluginMenu, i, tmpTexts, MAX_PATH_HALF-1, MF_BYPOSITION);
		if(namePathLen==targetLen&&!lstrcmp(NPP_PLUGIN_NAME, tmpTexts))
		{
			return GetSubMenu(pluginMenu, i);
		}
	}
	return 0;
}

HMENU GetPluginMenu() {
	HMENU hMenuPlugin = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETPLUGINMENU, 0 , (LPARAM)g_hModule);

	if(!hMenuPlugin) {
		hMenuPlugin = GetLegacyPluginMenu();
	}

	return hMenuPlugin;
}

// find any folder that contains main.js
bool MarkDownTextDlg::FindMarkdownEngines(TCHAR* path) 
{
	//see https://stackoverflow.com/questions/67273/how-do-you-iterate-through-every-file-directory-recursively-in-standard-c#answer-67336
	TCHAR scriptPath[MAX_PATH];
	GetModuleFileName((HMODULE)g_hModule, scriptPath, MAX_PATH);
	PathRemoveFileSpec(scriptPath);

	// |rnd_res| is the debug path pointing to the local repo of MarkdownEngins. It overrides the plugin path.
	path = scriptPath;
	if(rnd_res && PathFileExistsA(rnd_res))
	{
		MultiByteToWideChar(CP_ACP, 0, rnd_res, -1, path, MAX_PATH-1);
	}

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

std::map<std::string, std::string> & MarkDownTextDlg::getLocaliseMap() 
{
	localizationMap = &localizefile;
	return localizefile;
}

std::string * MarkDownTextDlg::getLocalString(char* name) 
{
	return name?GetLocalText(getLocaliseMap(), name):NULL;
}

void MarkDownTextDlg::localeTextToBuffer(TCHAR* buffer, int cchBuffer, char* name, TCHAR* defVal)
{
	string* val = getLocalString(name);
	if(val)
	{
		int len = MultiByteToWideChar(CP_ACP, 0, val->c_str(), -1, buffer, cchBuffer);
	}
	else if(defVal)
	{
		lstrcpy(buffer, defVal);
	}
}

void MarkDownTextDlg::destroyDynamicMenus()
{
	if(isCreated())
	{
		if(hMenuZoom) 
		{
			if(IsMenu(hMenuZoom)) DestroyMenu(hMenuZoom);
			ZOOMER.clear();
			hMenuZoom=0;
		}
		if(hMenuEngines) 
		{
			if(IsMenu(hMenuEngines)) DestroyMenu(hMenuEngines);
			EngineSwicther.clear();
			hMenuEngines=0;
		}
		if(hMenuLocate) 
		{
			if(IsMenu(hMenuLocate)) DestroyMenu(hMenuLocate);
			LocateScroll.clear();
			hMenuLocate=0;
		}
		if(hToolsMenu) 
		{
			if(IsMenu(hToolsMenu)) DestroyMenu(hToolsMenu);
			hToolsMenu=0;
		}
	}
}

void MarkDownTextDlg::checkAutoRun()
{
	if (!GetUIBoolReverse(5))
	{
		// if auto-run not checked, run directly.
		funcItems[1]._pFunc();
		//DefferedLoadingData* parms = defferedLoad;
		//if (parms)
		//{
		//	LogIs(2, "DefferedLoadingData %ld, %d", parms->bid, parms->articleType);
		//	_MDText.mWebView0->updateArticle(parms->bid, parms->articleType, false, true);
		//	defferedLoad = NULL;
		//	delete parms;
		//}
		return;
	}
	// else check and judge whether to run.
	CHAR* cmd;
	char* ext, * cmdn; 
	int i,j,k,extl,extsl,all_extsl=3,cmdl;
	for (int x = 0; x < 3; x++)
	{
		if(x==0)
		{
			TCHAR*  pszNewPath;
			if(legacy) {
				pszNewPath = path_buffer;
				::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH,0,(LPARAM)pszNewPath);
			} else {
				pszNewPath = (TCHAR*)::SendMessage(nppData._nppHandle,NPPM_GETRAWFULLCURRENTPATH, 0, 0);
			}
			cmdl = lstrlen(pszNewPath);
			if(cmdl>15)
			{
				pszNewPath = pszNewPath + cmdl - 15;
			}
			char tmp[32]{};
			cmdl = WideCharToMultiByte(CP_ACP, 0, pszNewPath, -1, tmp, 31, 0, 0);
			cmd = tmp;
		}
		else
		{
			cmd = GetCommandLineA();
			cmdl = strlen(cmd);
		}
		for (i = cmdl-2; i >=0 ; i--)
		//for (i = 0; i<cmdl; i++)
		{
			cmdn = cmd+i;
			for (j = 0; j < all_extsl; j++)
			{
				auto & exts = *all_exts[j];
				for (k = 0,extsl=exts.size(); k < extsl; k++)
				{
					ext = (char*)exts[k].data();
					extl = strlen(ext);
					if(cmdl-i>=extl&&strnicmp(cmdn, ext, extl)==0)
					{
						char c;
						//if(i+extl>=cmdl-1||(c=cmd[i+extl])==' '||c=='"'||c=='\'')
						{
							funcItems[1]._pFunc();
							return;
						}
					}
				}
			}
		}
	}
}

int funcSz(FuncItem* funcs)
{
	int sz=0;
	while(sz<64)
	{
		if(funcs[sz]._itemName[0]==NULL)
			break;
		sz++;
	}
	return sz;
}

void insertToolsMenu(HMENU hMenuPlugin)
{
	HMENU hToolsMenu = CreatePopupMenu();
	InsertMenu(hToolsMenu, 0,MF_BYPOSITION|MF_POPUP, hToolsStartId, L"HTML 转 Markdown");
	InsertMenu(hToolsMenu, 1,MF_BYPOSITION|MF_POPUP, hToolsStartId+1, L"HTML 转 Markdown（粘贴）");

	ModifyMenu(hMenuPlugin, funcItems[menuTools]._cmdID
		, MF_POPUP | MF_STRING | MF_BYCOMMAND, (UINT_PTR)hToolsMenu , L"工具");

	_MDText.hToolsMenu = hToolsMenu;
}

void MarkDownTextDlg::setLanguageName(wstring & name, bool init) 
{
	if(currentLanguageFile!=name||init)
	{
		currentLanguageFile = name;
		HMENU hMenuPlugin =  GetPluginMenu();
		if(init&&(name.length()==0)) // ||name==TEXT("english.ini")
		{
			insertToolsMenu(hMenuPlugin);
			return;
		}
		TCHAR path[MAX_PATH];
		PathAppend(path, g_ModulePath);
		PathAppend(path, TEXT("localization"));
		PathAppend(path, name.c_str());
		loadLanguge(path);
		destroyDynamicMenus();
		if(hMenuPlugin)
		{
			insertToolsMenu(hMenuPlugin);
			for (size_t pos = 0, len=funcSz(funcItems); pos < len; pos++)
			{
				int cmdID = ::GetMenuItemID(hMenuPlugin, pos);
				if(cmdID) {
					TCHAR* newStr;
					string * str = getLocalString(PluginMenuStrIds[pos]);
					if(str)
					{
						MultiByteToWideChar(CP_ACP, 0, str->data(), -1, path, MAX_PATH-1);
						newStr = path;
					}
					else
					{
						newStr = funcItems[pos]._itemName;
					}
					::ModifyMenu(hMenuPlugin, pos, MF_BYPOSITION, cmdID, newStr);
					if(getMenuItemChecked(pos))
						CheckMenuItem(hMenuPlugin, pos, MF_BYPOSITION|MF_CHECKED);
				}
			}
		}
	}
}

void MarkDownTextDlg::GlobalOnPvMnChecked(HMENU hMenu, int idx) 
{
	switch(idx) {
		// IDM_EX_LOCATE
		case 260:
		{
			bool val=ToggleUIBool(0, true);
			CheckMenu(funcSync, val);
			if(isCreated())
			{
				toolBar.setCheck(IDM_EX_LOCATE, val);
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
			if(mWebView0) {
				mWebView0->EvaluateJavascript("doScintillo(1)");
			}
		break;
		case 265:
			syncWebToline(true);
		break;
	}
}

// Switch internal or custom Markdown/HMTL/ASCIIDoc renderer.

void MarkDownTextDlg::SwitchEngines(int idx) 
{
	if(idx<-2) // reset custom engines.
	{
		MDEngineScanned=false;
		MDEngines.clear();
		return;
	}
	else if(idx==-2) // to use internal HTML engine
	{
		RendererTypeIdx=1;
	}
	else if(idx==-1) // to use internal Markdown engine
	{
		RendererTypeIdx=0;
		MDRoutine[0]='\0';
	}
	else if(idx>=0&&idx<MDEngines.size()) // to use custom engines
	{
		char* data2set;
		auto data = MDEngines[idx].data();
		if(!_tcsnicmp(data, TEXT("ASCII"), 5)) {
			// to treat as AsciiDoc engine.
			data2set = ADRoutine;
			RendererTypeIdx=2;
		} else {
			// to treat as Markdown engine.
			data2set = MDRoutine;
			RendererTypeIdx=0;
		}
		WideCharToMultiByte(CP_ACP, 0, data, -1, data2set, MAX_PATH_HALF-1, NULL, NULL);
	}
	// update menu checks
	for(int i=-2,len=MDEngines.size();i<len;i++)
	{
		CheckMenuItem(hMenuEngines, i+MDCRST, MF_BYPOSITION|(i==idx?MF_CHECKED:MF_UNCHECKED));
	}
	//if (!checkFileExt(RendererTypeIdx))
	{
		// mannual-switch memory
		prefRndTyps[::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0)] = RendererTypeIdx;
	}
	lastPickedRenderer = RendererTypeIdx;
	lastBid=0;
	bForcePreview=1;
	RefreshWebview(2);
	bForcePreview=0;
}

void ResetZoom()
{
	if(_MDText.mWebView0) {
		_MDText.mWebView0->ResetZoom();
	}
}

void MarkDownTextDlg::displayInstallGuide()
{
	if (installGuide)
	{
		installGuide->Close();
		return;
	}
	if (!isVisible())
	{
		isShowGuidePredateArticle = true;
		funcItems[0]._pFunc();
	}
	else
	{
		isShowGuidePredateArticle = false;
	}
	if (!installGuide)
	{
		installGuide = new WarnDlg(TEXT("ig.xml"));
		installGuide->Create(_hSelf, _T("embed")
			, WS_CHILD | WS_VISIBLE
			, 0
		);
		ShowWindow(installGuide->GetHWND(), SW_SHOW);
		::SendMessage(_hSelf, WM_SIZE, 0, 0);
	}
}

wstring MarkDownTextDlg::GetLocalWText(char* name, const TCHAR* defVal)
{
	if (localizationMap)
	{
		std::map<std::string, std::string> & m = getLocaliseMap();
		if(m.size())
		{
			auto idx = m.find(name);
			if(idx!=m.end())
			{	
				TCHAR text[MAX_PATH];
				auto & value = (*idx).second;
				int len = MultiByteToWideChar(CP_ACP, 0, value.c_str(), value.size(), text, MAX_PATH-1);
				text[len]='\0';
				return text;
			}
		}
	}
	return defVal;
}

extern void BoldenText();
extern void TiltText();
extern void UnderlineText();

void MarkDownTextDlg::TurnHtmlToMarkdown(int from)
{

}

extern bool WindowOpaqueMsg;

void MarkDownTextDlg::create()
{
	if (_hSelf==NULL)
	{
		NppDarkMode::initDarkMode();
		_hParent = nppData._nppHandle;
		tTbData data = {0};
		WindowOpaqueMsg = 0;
		DockingDlgInterface::create( &data );
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
	}
}

static UINT_PTR gTimerID = 0;

static void CALLBACK TimerProcFun(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if (gTimerID == idEvent)
	{
		KillTimer(NULL, gTimerID);
		//LogIs(2, "!!!");
		_MDText.RunToolsCommand(0);
		gTimerID = 0;
	}
}

CHAR* js= "var w=window;function toMarkdown(e) {	if(!w.turndownService) {		if(!w.TurndownService) {			var url='http://mdbr/Turndown/dist/turndown.js';			let xhr = new XMLHttpRequest();			xhr.open('GET', url, false);			xhr.send();			var item = document.createElement('script');			item.innerHTML=xhr.responseText;			item.async = false;			document.head.append(item);		}		w.turndownService = new w.TurndownService();	}	return turndownService.turndown(e);}function copyTextV1(text) {	var tmpText = window.document.getElementById('copit'); 	if(!tmpText) {		tmpText = document.createElement('p');		document.body.appendChild(tmpText);		tmpText.id = 'copit';	}	tmpText.focus();	tmpText.innerText=text;	const range = document.createRange();	range.selectNode(tmpText);	const selection = window.getSelection();	if(selection.rangeCount > 0) selection.removeAllRanges();	selection.addRange(range);	document.execCommand('copy');}function toPasteMdoc(e) {	copyTextV1(toMarkdown(e));}  console.log(666); toPasteMdoc('<h1>Turndown Demo</h1>');";

void MarkDownTextDlg::RunToolsCommand(int id)
{
	//if (!mWebView0)
	//{
	//	create();
	//	presenter.initWebViewImpl(kernelType, this, true);
	//	if (mWebView0)
	//	{
	//		gTimerID = ::SetTimer(NULL, 0, 220, TimerProcFun);
	//	}
	//	return;
	//}
	if (mWebView0)
	{
		DWORD len;
		char* data = loadPluginAsset("TurnDown\\transformer.js", len);
		data[len]=0;
		mWebView0->EvaluateJavascript(data);
	}
}

void MarkDownTextDlg::NewDoc(TCHAR* text)
{
	int len = WideCharToMultiByte(CP_ACP, 0, text, -1, 0, 0, 0, 0);
	if (len>=0)
	{
		char* data = new char[len+1];
		WideCharToMultiByte(CP_ACP, 0, text, -1, data, len, 0, 0);
		NewDoc(data);
		delete[] data;
	}

}

bool isDarkUDL(TCHAR* name) 
{
	if (_tcsstr(name, TEXT("ark"))) // [Dd]ark
	{
		return true;
	}
	if (_tcsstr(name, TEXT("lack"))) // [Bb]lack
	{
		return true;
	}
	if (_tcsstr(name, TEXT("bsidian"))) // [Oo]bsidian
	{
		return true;
	}
	return false;
}

void MarkDownTextDlg::LanguageToMarkdown()
{
	HMENU hAppMenu = GetMenu(nppData._nppHandle);
	if (hAppMenu)
	{
		bool isDark = NppDarkMode::isEnabled();
		hAppMenu = GetSubMenu(hAppMenu, 5);
		if (hAppMenu)
		{
			MENUITEMINFO mii; 
			mii.cbSize = sizeof(MENUITEMINFO);
			mii.fMask = MIIM_STRING|MIIM_ID;
			TCHAR tmp[256];
			mii.dwTypeData = tmp;
			int found = 0;
			bool opened = false;
			for (size_t i = 26, len=GetMenuItemCount(hAppMenu); i < len; i++)
			{
				mii.cch=255;
				::GetMenuItemInfo(hAppMenu, i, MF_BYPOSITION, &mii);
				tmp[min(mii.cch+1, 255)]=0;
				//LogIs(3, L"GetMenuItemInfo %d, %s", i, tmp);
				if (_tcsnccmp(tmp, TEXT("Markdown"), 8)==0)
				{ // 轮询全部Markdown开头的。
					if (!opened)
					{
						opened=true;
					}
					if (!found)
					{
						found=mii.wID;
					}
					bool darkUdl = isDarkUDL(tmp+8);
					if (isDark == darkUdl)
					{ // 保持当前的合法选择。
						found=mii.wID;
						if (mii.fType&MFT_RADIOCHECK)
						{ // useless when switching file 
							// | 当切换文件时立即调用是无效的，因为菜单尚未更新。所以要deferred。
							//break;
						}
						//break;
					}
					//LogIs(2, L"LanguageToMarkdown %d %s", isDark, mii.fType&MFT_RADIOCHECK, tmp);
				} else if(opened) {
					break;
				}
			}
			//LogIs(2, L"anguageToMarkdown %s", tmp);
			SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(found, 66), 0);
		}
	}
}

void MarkDownTextDlg::NewDoc(CHAR* val)
{
	SendMessage(nppData._nppHandle, NPPM_SUPRESSSCIMACRO, true, 0);
	int currentEdit=0;
	SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
	auto currentSci = currentEdit?nppData._scintillaSecondHandle:nppData._scintillaMainHandle;

	//SendMessage(currentSci, SCI_SETTARGETSTART, 0, 0);
	//SendMessage(currentSci, SCI_SETTARGETEND, 1, 0);
	//SendMessage(currentSci, SCI_REPLACETARGET, lstrlen(name), (LPARAM)name);

	//SendMessage(currentSci, SCI_ADDTEXT, lstrlen(text), (LPARAM)text);

	//::MessageBox(0, text, TEXT(""), MB_OK);

	SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_FILE_NEW, 66), 0);
	SendMessage(currentSci, SCI_ADDTEXT, strlen(val), (LPARAM)val);
	LanguageToMarkdown();
	if (!isClosed())
	{
		LONG_PTR bid = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
		prefRndTyps[bid] = 0;
		refreshDlg(false, false);
	}
	SendMessage(nppData._nppHandle, NPPM_SUPRESSSCIMACRO, false, 0);
}

int MarkDownTextDlg::getDarkBG()
{
	if (NppDarkMode::isEnabled())
	{
		return 0xff000000;
	} else {
		return 0;
	}
}

// |source| 0=click 1=context_menu
void MarkDownTextDlg::OnToolBarCommand(UINT CMDID, char source, POINT* pt)
{ 
	switch ( CMDID ) {
		case IDM_EX_BOLDEN:
		{
			if(source==0)
				BoldenText();
			// todo SendMessage(nppData._nppHandle, WM_COMMAND, id, 0);
		}
		return;
		case IDM_EX_DEV:
		{
			if(source==0)
			{
				if(mWebView0) {
					// todo concat the path
					mWebView0->ShowDevTools(NULL);
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
			PrivateTrackPopup(_hSelf, GetPluginMenu(), funcItems, CMDID);
		}
		return;
		case IDM_EX_DOWN:
			if (installGuide)
			{
				installGuide->Close();
			}
			else if(source==0&&mWebView0){
				mWebView0->GoBack();
			}
		return;
		case IDM_EX_UP:
			if(mWebView0) {
				mWebView0->GoForward();
			}
		return;
		case IDM_EX_REFRESH:
			if (installGuide)
			{
				installGuide->Close();
			}
			else if(source==0 && mWebView0)
			{
				//RefreshWebview(3);
				mWebView0->Refresh();
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
				lstrcpy(funcItems[0]._itemName, L"213");
				if(ZOOMER.size()==0)
				{
					ZOOMER.resize(1);
					ZOOMER.at(0)={TEXT(""), ResetZoom, 161, false, 0};
					localeTextToBuffer(ZOOMER.at(0)._itemName, 63, "_rst", TEXT("Reset")); // 重置
				}
				if(hMenuZoom==0)
				{
					hMenuZoom = buildPluginPrivateMenu(ZOOMER);
				}
				PrivateTrackPopup(_hSelf, hMenuZoom, ZOOMER.data(), CMDID);
			}
		}
		return;
		case IDM_EX_DELTA:
		{
			if(EngineSwicther.size()==0)
			{
				EngineSwicther.resize(MDCRST);
				EngineSwicther.at(0)={TEXT(""), happy, 61, false, 0};
				//lstrcpy(EngineSwicther.at(0)._itemName, ZH_CN?TEXT("切换浏览器内核："):TEXT("Switch Browser Kernel :"));
				localeTextToBuffer(EngineSwicther.at(0)._itemName, 63, "_sw_bw", TEXT("Switch Browser Kernel :"));
				EngineSwicther.at(1)={TEXT("Miniblink-wke"), engineToWke, 62, false, 0};
				EngineSwicther.at(2)={TEXT("Miniblink-mb"), engineToMb, 63, false, 0};
				EngineSwicther.at(3)={TEXT("Chromium-Embeded"), engineToChromium, 64, false, 0};
				EngineSwicther.at(4)={TEXT("Webview2 ( Recommended )"), engineToWebview2, 65, false, 0};
				EngineSwicther.at(5)={TEXT(""), 0, 0, false, 0};
				EngineSwicther.at(6)={TEXT(""), (PFUNCPLUGINCMD)SwitchEnginesStatic, 66, false, 0};
				//lstrcpy(EngineSwicther.at(6)._itemName, ZH_CN?TEXT("切换渲染引擎："):TEXT(" :"));
				localeTextToBuffer(EngineSwicther.at(6)._itemName, 63, "_sw_rt", TEXT("Switch Markdown Engine :"));
				EngineSwicther.at(7)={TEXT("HTML"), (PFUNCPLUGINCMD)SwitchEnginesStatic, 67, false, 0};
				EngineSwicther.at(8)={TEXT("md.html"), (PFUNCPLUGINCMD)SwitchEnginesStatic, 68, false, 0};
			}
			bool rebuildMenu = hMenuEngines==0||!MDEngineScanned;
			if(hMenuEngines==0)
			{
				hMenuEngines = buildPluginPrivateMenu(EngineSwicther);
			}
			if(!MDEngineScanned)
			{
				FindMarkdownEngines(NULL);
				MDEngineScanned=1;
			}
			if(rebuildMenu)
			{
				EngineSwicther.resize(MDCRST+MDEngines.size());
				while(GetMenuItemCount(hMenuEngines)>MDCRST)
					RemoveMenu(hMenuEngines, MDCRST, MF_BYPOSITION);
				int foundCheck=0;
				bool isUserDefinedRenderer = RendererTypeIdx>=0&&RendererTypeIdx<=2&&RendererNames[RendererTypeIdx][0];
				if(isUserDefinedRenderer)
				{
					MultiByteToWideChar(CP_ACP, 0, RendererNames[RendererTypeIdx], -1, path_buffer, MAX_PATH);
				}
				else if(_MDText.RendererTypeIdx==1) {
					CheckMenuItem(hMenuEngines, MDCRST-2, MF_BYPOSITION|MF_CHECKED);
				}
				else // 默认引擎 md.html
				{
					CheckMenuItem(hMenuEngines, MDCRST-1, MF_BYPOSITION|MF_CHECKED);
				}
				for(int i=0,ii,len=MDEngines.size();i<len;i++)
				{
					ii=MDCRST+i;
					EngineSwicther.at(ii)={TEXT(""), (PFUNCPLUGINCMD)SwitchEnginesStatic, 60+MDCRST+i, false, 0};
					auto data=MDEngines[i].data();
					lstrcpy(EngineSwicther.at(ii)._itemName, data);
					::InsertMenu(hMenuEngines, ii, MF_BYPOSITION, EngineSwicther.at(ii)._cmdID, data);
					if(isUserDefinedRenderer&&!foundCheck&&lstrcmp(path_buffer, data)==0)
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
			PrivateTrackPopup(_hSelf, hMenuEngines, EngineSwicther.data(), CMDID);
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
					LocateScroll.at(i++)={TEXT("Sync Text -> Webview"), (PFUNCPLUGINCMD)GlobalOnPvMnCheckedStatic, 261, false, 0};
					LocateScroll.at(i++)={TEXT("-->  Sync Now (&D)"), (PFUNCPLUGINCMD)GlobalOnPvMnCheckedStatic, 265, false, 0};
					LocateScroll.at(i++)={TEXT("Sync Text <- Webview"), (PFUNCPLUGINCMD)GlobalOnPvMnCheckedStatic, 262, false, 0};
					LocateScroll.at(i++)={TEXT("<--  Sync Now (&A)"), (PFUNCPLUGINCMD)GlobalOnPvMnCheckedStatic, 264, false, 0};
					LocateScroll.at(i++)={TEXT("Locate current file"), (PFUNCPLUGINCMD)GlobalOnPvMnCheckedStatic, 263, false, 0};
					i=0;
					localeTextToBuffer(LocateScroll.at(i++)._itemName, 63, "_s_tw", NULL);
					localeTextToBuffer(LocateScroll.at(i++)._itemName, 63, "_s_tw_", NULL);
					localeTextToBuffer(LocateScroll.at(i++)._itemName, 63, "_s_wt", NULL);
					localeTextToBuffer(LocateScroll.at(i++)._itemName, 63, "_s_wt_", NULL);
					localeTextToBuffer(LocateScroll.at(i++)._itemName, 63, "_s_fd", NULL);

				}
				if(hMenuLocate==0)
				{
					hMenuLocate = buildPluginPrivateMenu(LocateScroll);
					::CheckMenuItem(hMenuLocate, static_cast<UINT>(261), MF_BYCOMMAND | (static_cast<BOOL>(GetUIBoolReverse(1) ? MF_CHECKED : MF_UNCHECKED)));
					::CheckMenuItem(hMenuLocate, static_cast<UINT>(262), MF_BYCOMMAND | (static_cast<BOOL>(GetUIBoolReverse(2)) ? MF_CHECKED : MF_UNCHECKED));
				}
				PrivateTrackPopup(_hSelf, hMenuLocate, LocateScroll.data(), CMDID);
			}
		}
		return;
	}
}
