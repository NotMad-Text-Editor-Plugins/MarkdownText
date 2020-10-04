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
#include "time.h"

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
	//wkeWebView newWindow = wkeCreateWebWindow(WKE_WINDOW_TYPE_POPUP, NULL, features->x, features->y, features->width, features->height);
	//wkeShowWindow(newWindow, true);
	//return newWindow;
	wkeLoadURL(webWindow, (CHAR*)url);
	return 0;
}

const char InternalResHead[] = "mdbr://";
const char InternalResHead1[] = "http://mdbr/";

CHAR* loadPluginAsset(const char* path, DWORD & dataLen)
{
	CHAR ResPath[MAX_PATH];
	::GetModuleFileNameA((HINSTANCE)g_hModule, ResPath, MAX_PATH);
	::PathRemoveFileSpecA(ResPath);
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

bool onLoadUrlBegin(wkeWebView webView, void* param, const char* url, void *job)
{
	if(strncmp(url, InternalResHead, 7)==0)
	{
		auto path = url+7;
		const utf8* decodeURL = wkeUtilDecodeURLEscape(path);
		if(decodeURL)
		{
			if(strstr(decodeURL, "..")) // security check
			{
				return false;
			}
			DWORD dataLen;
			auto buffer = loadPluginAsset(path, dataLen);
			if(buffer)
			{
				wkeNetSetData(job, buffer, dataLen);
				return true;
			}
		}
	}
	return false;
}

void onLoadUrlEnd(wkeWebView webView, void* param, const char *url, void *job, void* buf, int len)
{
	//wkeRunJS();
	return;
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

CHAR* GetDocTex(size_t & docLength, LONG_PTR bid)
{
	int curScintilla;
	SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&curScintilla);
	auto currrentSc = curScintilla?nppData._scintillaSecondHandle:nppData._scintillaMainHandle;

	if(!_MDText.mWebView_1 && bid && !legacy)
	{
		LONG_PTR DOCUMENTPTR = SendMessage(nppData._nppHandle, NPPM_GETDOCUMENTPTR, bid, bid);
		docLength = SendMessage(currrentSc, SCI_GETTEXTLENGTH, DOCUMENTPTR, DOCUMENTPTR);
		auto raw_data = (CHAR*)SendMessage(currrentSc, SCI_GETRAWTEXT, DOCUMENTPTR, DOCUMENTPTR);
		if(docLength&&raw_data)
		{
			return raw_data;
		}
	}
	
	docLength = SendMessage(currrentSc, SCI_GETTEXTLENGTH, 0, 0);
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
	ScintillaGetText(currrentSc, universal_buffer, 0, docLength);
	universal_buffer[docLength] = '\0';
	//return 0;
	return universal_buffer;
}

//synchronous callback
jsValue WKE_CALL_TYPE GetDocText(jsExecState es, void* param)
{
	//if (0 == jsArgCount(es))
	//	return jsUndefined();
	//jsValue arg0 = jsArg(es, 0);
	//if (!jsIsString(arg0))
	//	return jsUndefined();
	//
	//std::string path;
	//path = jsToTempString(es, arg0);
	//if ("runEchars" == path) {
	//	createECharsTest();
	//} else if ("wkeBrowser" == path) {
	//	wkeBrowserMain(nullptr, nullptr, nullptr, TRUE);
	//}
	//
	//path += "\n";
	//OutputDebugStringA(path.c_str());

	size_t len;
	auto ret=jsString(es, GetDocTex(len, 0));
	//::MessageBox(NULL, TEXT("111"), TEXT(""), MB_OK);
	return ret;
	//return jsString(es, "# Hello `md.html` World!");
}

LRESULT WINAPI testWindowProc(
	__in HWND hWnd,
	__in UINT msg,
	__in WPARAM wParam,
	__in LPARAM lParam)
{
	LRESULT result = 0;
	mbWebView view = (mbWebView)::GetProp(hWnd, L"mb");
	if (!view)
		return ::DefWindowProc(hWnd, msg, wParam, lParam);

	switch (msg) {
	case WM_NCDESTROY:
		if (::GetProp(hWnd, L"subView")) {
			RemoveProp(hWnd, L"subView");
		}

		mbDestroyWebView(view);
		return 0;

	case WM_ERASEBKGND:
		return TRUE;

	case WM_PAINT:
	{
		if (WS_EX_LAYERED == (WS_EX_LAYERED & GetWindowLong(hWnd, GWL_EXSTYLE)))
			break;
		//mbRepaintIfNeeded(view);

		PAINTSTRUCT ps = { 0 };
		HDC hdc = ::BeginPaint(hWnd, &ps);

		RECT rcClip = ps.rcPaint;

		RECT rcClient;
		::GetClientRect(hWnd, &rcClient);

		RECT rcInvalid = rcClient;
		if (rcClip.right != rcClip.left && rcClip.bottom != rcClip.top)
			::IntersectRect(&rcInvalid, &rcClip, &rcClient);

		int srcX = rcInvalid.left - rcClient.left;
		int srcY = rcInvalid.top - rcClient.top;
		int destX = rcInvalid.left;
		int destY = rcInvalid.top;
		int width = rcInvalid.right - rcInvalid.left;
		int height = rcInvalid.bottom - rcInvalid.top;

		if (0 != width && 0 != height) {
			HDC hMbDC = mbGetLockedViewDC(view);
			::BitBlt(hdc, destX, destY, width, height, hMbDC, srcX, srcY, SRCCOPY);
			mbUnlockViewDC(view);
		}

		::EndPaint(hWnd, &ps);
		return 1;
		break;
	}
	case WM_SIZE:
	{
		RECT rc = { 0 };
		::GetClientRect(hWnd, &rc);
		int width = rc.right - rc.left;
		int height = rc.bottom - rc.top;

		::mbResize(view, width, height);
		// mbRepaintIfNeeded(view);
		::mbWake(view);

		return 0;
	}
	case WM_KEYDOWN:
	{
		unsigned int virtualKeyCode = wParam;
		unsigned int flags = 0;
		if (HIWORD(lParam) & KF_REPEAT)
			flags |= MB_REPEAT;
		if (HIWORD(lParam) & KF_EXTENDED)
			flags |= MB_EXTENDED;

		if (mbFireKeyDownEvent(view, virtualKeyCode, flags, false))
			return 0;
		break;
	}
	case WM_KEYUP:
	{
		unsigned int virtualKeyCode = wParam;
		unsigned int flags = 0;
		if (HIWORD(lParam) & KF_REPEAT)
			flags |= MB_REPEAT;
		if (HIWORD(lParam) & KF_EXTENDED)
			flags |= MB_EXTENDED;

		if (mbFireKeyUpEvent(view, virtualKeyCode, flags, false))
			return 0;
		break;
	}
	case WM_CHAR:
	{
		unsigned int charCode = wParam;
		unsigned int flags = 0;
		if (HIWORD(lParam) & KF_REPEAT)
			flags |= MB_REPEAT;
		if (HIWORD(lParam) & KF_EXTENDED)
			flags |= MB_EXTENDED;

		if (mbFireKeyPressEvent(view, charCode, flags, false))
			return 0;
		break;
	}
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
	{
		if (msg == WM_LBUTTONDOWN || msg == WM_MBUTTONDOWN || msg == WM_RBUTTONDOWN) {
			if (::GetFocus() != hWnd)
				::SetFocus(hWnd);
			::SetCapture(hWnd);
		} else if (msg == WM_LBUTTONUP || msg == WM_MBUTTONUP || msg == WM_RBUTTONUP) {
			ReleaseCapture();
		}

		int x = LOWORD(lParam);
		int y = HIWORD(lParam);

		unsigned int flags = 0;

		if (wParam & MK_CONTROL)
			flags |= MB_CONTROL;
		if (wParam & MK_SHIFT)
			flags |= MB_SHIFT;

		if (wParam & MK_LBUTTON)
			flags |= MB_LBUTTON;
		if (wParam & MK_MBUTTON)
			flags |= MB_MBUTTON;
		if (wParam & MK_RBUTTON)
			flags |= MB_RBUTTON;

		if (mbFireMouseEvent(view, msg, x, y, flags))
			return 0;
		break;
	}
	case WM_CONTEXTMENU:
	{
		POINT pt;
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);

		if (pt.x != -1 && pt.y != -1)
			ScreenToClient(hWnd, &pt);

		unsigned int flags = 0;

		if (wParam & MK_CONTROL)
			flags |= MB_CONTROL;
		if (wParam & MK_SHIFT)
			flags |= MB_SHIFT;

		if (wParam & MK_LBUTTON)
			flags |= MB_LBUTTON;
		if (wParam & MK_MBUTTON)
			flags |= MB_MBUTTON;
		if (wParam & MK_RBUTTON)
			flags |= MB_RBUTTON;

		//if (mbFireContextMenuEvent(view, pt.x, pt.y, flags))
		//	return 0;


		break;
	}
	case WM_MOUSEWHEEL:
	{
		POINT pt;
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		::ScreenToClient(hWnd, &pt);

		int delta = GET_WHEEL_DELTA_WPARAM(wParam);

		unsigned int flags = 0;

		if (wParam & MK_CONTROL)
			flags |= MB_CONTROL;
		if (wParam & MK_SHIFT)
			flags |= MB_SHIFT;

		if (wParam & MK_LBUTTON)
			flags |= MB_LBUTTON;
		if (wParam & MK_MBUTTON)
			flags |= MB_MBUTTON;
		if (wParam & MK_RBUTTON)
			flags |= MB_RBUTTON;

		if (mbFireMouseWheelEvent(view, pt.x, pt.y, delta, flags))
			return 0;
		break;
	}
	case WM_SETFOCUS:
		mbSetFocus(view);
		return 0;

	case WM_KILLFOCUS:
		mbKillFocus(view);
		return 0;

	case WM_SETCURSOR:
		if (mbFireWindowsMessage(view, hWnd, WM_SETCURSOR, 0, 0, &result))
			return result;
		break;

	case WM_IME_STARTCOMPOSITION: {
		if (mbFireWindowsMessage(view, hWnd, WM_IME_STARTCOMPOSITION, 0, 0, &result))
			return result;
		break;
	}
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

BOOL regWndClass(LPCTSTR lpcsClassName, DWORD dwStyle)
{
	WNDCLASS wndclass = { 0 };

	wndclass.style = dwStyle;
	wndclass.lpfnWndProc = testWindowProc;
	wndclass.cbClsExtra = 200;
	wndclass.cbWndExtra = 200;
	wndclass.hInstance = ::GetModuleHandle(NULL);
	wndclass.hIcon = NULL;
	//wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = lpcsClassName;

	::RegisterClass(&wndclass);
	return TRUE;
}

BOOL unregWndClass(LPCTSTR lpcsClassName)
{
	::UnregisterClass(lpcsClassName, ::GetModuleHandle(NULL));
	return TRUE;
}

void MB_CALL_TYPE handlePaintUpdatedCallback(mbWebView webView, void* param, const HDC hdc, int x, int y, int cx, int cy)
{
	HWND hWnd = (HWND)param;
	BOOL callOk = FALSE;
	if (WS_EX_LAYERED == (WS_EX_LAYERED & GetWindowLong(hWnd, GWL_EXSTYLE))) {
		RECT rectDest;
		::GetWindowRect(hWnd, &rectDest);

		SIZE sizeDest = { rectDest.right - rectDest.left, rectDest.bottom - rectDest.top };
		POINT pointDest = { 0, 0 }; // { rectDest.left, rectDest.top };
		POINT pointSource = { 0, 0 };

		BITMAP bmp = { 0 };
		HBITMAP hBmp = (HBITMAP)::GetCurrentObject(hdc, OBJ_BITMAP);
		::GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp);

		sizeDest.cx = bmp.bmWidth;
		sizeDest.cy = bmp.bmHeight;

		HDC hdcScreen = GetDC(hWnd);

		BLENDFUNCTION blend = { 0 };
		blend.BlendOp = AC_SRC_OVER;
		blend.SourceConstantAlpha = 255;
		blend.AlphaFormat = AC_SRC_ALPHA;
		callOk = ::UpdateLayeredWindow(hWnd, hdcScreen, nullptr, &sizeDest, hdc, &pointSource, RGB(0xFF, 0xFF, 0xFF), &blend, ULW_ALPHA);
		if (!callOk) {
			HDC hdcMemory = ::CreateCompatibleDC(hdcScreen);
			HBITMAP hbmpMemory = ::CreateCompatibleBitmap(hdcScreen, sizeDest.cx, sizeDest.cy);
			HBITMAP hbmpOld = (HBITMAP)::SelectObject(hdcMemory, hbmpMemory);

			::BitBlt(hdcMemory, 0, 0, sizeDest.cx, sizeDest.cy, hdc, 0, 0, SRCCOPY | CAPTUREBLT);

			::BitBlt(hdc, 0, 0, sizeDest.cx, sizeDest.cy, hdcMemory, 0, 0, SRCCOPY | CAPTUREBLT); //!

			callOk = ::UpdateLayeredWindow(hWnd, hdcScreen, nullptr, &sizeDest, hdcMemory, &pointSource, RGB(0xFF, 0xFF, 0xFF), &blend, ULW_ALPHA);

			::SelectObject(hdcMemory, (HGDIOBJ)hbmpOld);
			::DeleteObject((HGDIOBJ)hbmpMemory);
			::DeleteDC(hdcMemory);
		}

		::ReleaseDC(hWnd, hdcScreen);
	} else {
		RECT rc = { x, y, x + cx, y + cy };
		::InvalidateRect(hWnd, &rc, FALSE);
	}
}

void MB_CALL_TYPE onRunJs(mbWebView webView, void* param, mbJsExecState es, mbJsValue v)
{
	const utf8* str = mbJsToString(es, v);

	OutputDebugStringA("onRunJs:");
	OutputDebugStringA(str);
	OutputDebugStringA("\n");
}

BOOL MB_CALL_TYPE handleLoadUrlBegin(mbWebView webView, void* param, const char* url, void *job)
{
	if(strncmp(url, InternalResHead, 7)==0)
	{
		auto path = url+7;
		//const utf8* decodeURL = wkeUtilDecodeURLEscape(path);
		const utf8* decodeURL = mbUtilDecodeURLEscape(path);
		if(decodeURL)
		{
			if(strstr(decodeURL, "..")) // security check
			{
				return false;
			}
			DWORD dataLen;
			auto buffer = loadPluginAsset(path, dataLen);
			if(buffer)
			{
				mbNetSetData(job, buffer, dataLen);
				return true;
			}
		}
	}
	return false;
}

void MB_CALL_TYPE handleDocumentReady(mbWebView webView, void* param, mbWebFrameHandle frameId)
{
	OutputDebugStringA("HandleDocumentReady\n");
	mbShowWindow(webView, TRUE);
	mbRunJs(webView, mbWebFrameGetMainFrame(webView), "return window.onNativeRunjs('I am runjs');", TRUE, onRunJs, nullptr, nullptr);
}


void MB_CALL_TYPE handleLoadingFinish(mbWebView webView, void* param, mbWebFrameHandle frameId, const utf8* url, mbLoadingResult result, const utf8* failedReason)
{
	//if(result == MB_LOADING_SUCCEEDED)
	//::mbNetGetFavicon(webView, HandleFaviconReceived, param);
	OutputDebugStringA("handleLoadingFinish \n");
}

#define kClassWindow L"TestMbWindow"

mbWebView MB_CALL_TYPE handleCreateView(mbWebView webView, void* param, mbNavigationType navigationType, const utf8* url, const mbWindowFeatures* windowFeatures)
{
	if(1) {
		mbLoadURL(webView, url);
		return 0;
	}
	mbWebView view = mbCreateWebView();
	HWND hWnd = ::CreateWindowEx(WS_EX_APPWINDOW, kClassWindow, NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE, windowFeatures->x, windowFeatures->y, windowFeatures->width, windowFeatures->height, NULL, NULL, ::GetModuleHandle(NULL), NULL);
	::SetProp(hWnd, L"mb", (HANDLE)view);
	::SetProp(hWnd, L"subView", (HANDLE)TRUE);
	::mbSetHandle(view, hWnd);
	::mbOnPaintUpdated(view, handlePaintUpdatedCallback, hWnd);
	::mbOnLoadingFinish(view, handleLoadingFinish, (void*)view);
	::mbOnCreateView(view, handleCreateView, (void*)view);
	::mbSetNavigationToNewWindowEnable(view, true);
	::mbSetCspCheckEnable(view, false);


	RECT rc = { 0 };
	::GetClientRect(hWnd, &rc);
	::mbResize(view, rc.right, rc.bottom);

	//mbShowWindow(view, TRUE);
	return view;
}

void MB_CALL_TYPE onJsQuery(mbWebView webView, void* param, mbJsExecState es, int64_t queryId, int customMsg, const utf8* request)
{
	if(customMsg==0x666)
	{
		size_t len;
		auto res=GetDocTex(len, 0);
		mbResponseQuery(webView, queryId, customMsg, res);
		delete[] res;
	}
}

void STR2LONGPTR(TCHAR* STR, LONG_PTR & LONGPTR)
{
	int len = lstrlen(STR);
	bool intOpened=false;
	for(int i=0;i<len;i++) {
		int intVal = STR[i]-'0';
		int valval = intVal>=0&&intVal<=9||STR[i]=='-';
		if(!intOpened)
		{
			intOpened=valval;
		}
		else if(!valval)
		{
			break;
		}
		if(intOpened)
		{
			LONGPTR = LONGPTR*10+intVal;
		}
	}
}

#ifdef  UNICODE
void LONGPTR2STR(TCHAR* STR, LONG_PTR LONGPTR)
{
	TCHAR* start=STR;
	while(LONGPTR)
	{
		*(STR++)='0'+(LONGPTR%10);
		LONGPTR/=10;
	}
	*STR='\0';
	wcsrev(start);
}
#endif

BJSCV* GetDocText1(LONG_PTR funcName, int argc, LONG_PTR argv, int sizeofBJSCV)
{
	//::MessageBoxA(NULL, ("GetDocText1"), (""), MB_OK);
	LONG_PTR bid=0;
	int structSize=0;
	if(argc==1)
	{
		char* args = bwParseCefV8Args(argv, structSize);
		if(structSize)
		{
			BJSCV* val = (BJSCV*)(args+0*structSize);
			if(val->value_type==typeString)
			{
				STR2LONGPTR((TCHAR*)val->charVal, bid);
			}
			// testJs('Hah')
		}
	}
	size_t len;
	return new BJSCV{typeString, 0, GetDocTex(len, bid)};
}

void onBrowserPrepared(bwWebView browserPtr)
{
	bwInstallJsNativeToWidget(browserPtr, "GetDocText1", GetDocText1);
	_MDText.mWebView_2 = browserPtr;
	_MDText.currentKernal = (LONG_PTR)browserPtr;
	_MDText.hBrowser = bwGetHWNDForBrowser(browserPtr);
	::SendMessage(_MDText.getHSelf(), WM_SIZE, 0, 0);
	_MDText.RefreshWebview();
}

url_intercept_result* InterceptBrowserWidget(std::string url)
{
	if(url=="https://tests/home") {
		return new url_intercept_result{(CHAR*)"Markdown Text", 14, 200, (CHAR*)"OK"};
	}

	if(strncmp(url.data(), InternalResHead1, 12)==0)
	{
		auto path = url.data()+12;
		if(path)
		{
			if(strstr(path, "..")) // security check
			{
				return false;
			}
			DWORD dataLen;
			auto buffer = loadPluginAsset(path, dataLen);
			if(buffer)
			{
				return new url_intercept_result{buffer, dataLen, 200, (CHAR*)"OK"};
			}
		}
	}
	return nullptr;
}

clock_t browser_deferred_create_time=0;

void MarkDownTextDlg::display(bool toShow){
	DockingDlgInterface::display(toShow);

	setClosed(false);

	if(toShow && currentKernal==0) 
	{
		int kernalType=-1; // -1_auto 0_wke 1_mb 2_bw
		TCHAR WKPath[MAX_PATH]={0};
		TCHAR WKPath1[MAX_PATH]={0};
		::GetModuleFileName((HINSTANCE)g_hModule, WKPath, MAX_PATH);
		::PathRemoveFileSpec(WKPath);
		int error_code=0;
		if(PathFileExists(WKPath))
		{ // ThriveEngines !
			// BrowserWidget
			bool prefer_bw = kernalType==2||kernalType==-1;
			bool browser_deferred_creating=false;
			if(!currentKernal && prefer_bw)
			{
				clock_t time = clock();
				if(browser_deferred_create_time && time-browser_deferred_create_time<550)
				{
					browser_deferred_creating=1;
				}
				if(!browser_deferred_creating)
				{
					browser_deferred_create_time = time;
					lstrcpy(WKPath1, WKPath);
					::PathAppend(WKPath1, L"..\\BrowserWidget\\cefclient.dll");
					if(PathFileExists(WKPath1))
					{
						if(bwInit(WKPath1) && bwCreateBrowser({_hSelf, "https://tests/home", onBrowserPrepared, InterceptBrowserWidget}))
						{
							browser_deferred_creating=1;
							//::MessageBox(NULL, TEXT("111"), TEXT(""), MB_OK);
						}
					}
				}
			}
			if(!browser_deferred_creating)
			{
				::PathAppend(WKPath, L"miniblink_x64.dll");
				if(PathFileExists(WKPath))
				{
					// mb
					if(!currentKernal && kernalType!=0)
					{
						mbSetMbMainDllPath(WKPath);
						::GetModuleFileName((HINSTANCE)g_hModule, WKPath1, MAX_PATH);
						::PathRemoveFileSpec(WKPath1);
						::PathAppend(WKPath1, L"mb_x64.dll");
						if(PathFileExists(WKPath1))
						{
							mbSetMbDllPath(WKPath1);
							mbSettings settings;
							memset(&settings, 0, sizeof(settings));
							//settings.mask = MB_ENABLE_NODEJS;
							mbInit(&settings);
							{
								mWebView_1 = mbCreateWebView();
								if(currentKernal=mWebView_1)
								{
									regWndClass(kClassWindow, CS_HREDRAW | CS_VREDRAW);
									hBrowser = ::CreateWindowEx(0 , kClassWindow , NULL
										, WS_CHILD , 0 , 0 , 840 , 680 , _hSelf , NULL , ::GetModuleHandle(NULL), NULL);
									::SetProp(hBrowser, L"mb", (HANDLE)mWebView_1);
									mbSetHandle(mWebView_1, hBrowser);
									mbOnPaintUpdated(mWebView_1, handlePaintUpdatedCallback, hBrowser);
									mbOnLoadUrlBegin(mWebView_1, handleLoadUrlBegin, (void*)mWebView_1);
									mbOnDocumentReady(mWebView_1, handleDocumentReady, (void*)mWebView_1);
									mbOnLoadingFinish(mWebView_1, handleLoadingFinish, (void*)mWebView_1);
									mbOnCreateView(mWebView_1, handleCreateView, (void*)mWebView_1);
									mbSetNavigationToNewWindowEnable(mWebView_1, 1);
									mbSetCspCheckEnable(mWebView_1, false);
									mbMoveToCenter(mWebView_1);
									mbOnJsQuery(mWebView_1, onJsQuery, (void*)1);
								}
							}
						}
					}
					// wke
					if(!currentKernal && kernalType<=1)
					{
						wkeSetWkeDllPath(WKPath);
						if(wkeInitialize()) 
						{
							mWebView = wkeCreateWebWindow(WKE_WINDOW_TYPE_CONTROL, _hSelf , 0, 0, 640, 480); 
							if (currentKernal=(intptr_t)mWebView)
							{
								hBrowser = wkeGetWindowHandle(mWebView);
								//setMoveWindowArea(0, 0, 640, 30); // 设置窗口可拖动区域，用于无边框窗体
								//wkeSetWindowTitleW(mWebView, NPP_PLUGIN_NAME);
								wkeOnDidCreateScriptContext(mWebView, onDidCreateScriptContextCallback, this);
								wkeOnWindowClosing(mWebView, handleWindowClosing, this);
								wkeOnWindowDestroy(mWebView, handleWindowDestroy, this);
								wkeOnDocumentReady(mWebView, handleDocumentReady, this);
								wkeOnTitleChanged(mWebView, handleTitleChanged, this);
								wkeOnCreateView(mWebView, onCreateView, this);
								wkeOnLoadUrlBegin(mWebView, onLoadUrlBegin, this);
								wkeOnLoadUrlEnd(mWebView, onLoadUrlEnd, this);
								wkeSetDebugConfig(mWebView, "decodeUrlRequest", nullptr);
								wkeJsBindFunction("GetDocText", &GetDocText, nullptr, 1);
							} else {
								error_code = 101;
							}
						} else {
							error_code = 100;
						}
					}
				}
			}
			if(currentKernal)
			{
				RefreshWebview();
				if(mWebView) {
					wkeShowWindow(mWebView, TRUE);
				} else if(mWebView_1){
					mbShowWindow(mWebView_1, TRUE);
				}
			}
		}
	}

	//::SendMessage( _hSelf, SELF_REFRESH, 0, 1);
};

void MarkDownTextDlg::setClosed(bool toClose) {
	_isClosed = toClose;
	::CheckMenuItem(::GetMenu(nppData._nppHandle), 
		funcItem[menuOption]._cmdID, MF_BYCOMMAND | (toClose?MF_UNCHECKED:MF_CHECKED));
}

std::string page_data="<!doctype html><meta charset=\"utf-8\"><script src=\"http://mdbr/main.js\"></script><body><script>window.APMD(GetDocText1(''));</script></body>";
//std::string page_data="";

void LONGPTR2STR(CHAR* STR, LONG_PTR LONGPTR)
{
	CHAR* start=STR;
	while(LONGPTR)
	{
		*(STR++)='0'+(LONGPTR%10);
		LONGPTR/=10;
	}
	*STR='\0';
	strrev(start);
}

bool STRENDWITH(TCHAR* strA,CHAR* strB)
{
	int valen = lstrlen(strA);
	int pc = strlen(strB);
	int to = valen-pc;
	int po = 0;
	// Note: toffset might be near -1>>>1.
	if (to < 0) {
		return false;
	}
	while (--pc >= 0) {
		if (strA[to++] != strB[po++]) {
			return false;
		}
	}
	return true;
}

void MarkDownTextDlg::RefreshWebview(bool fromEditor) {
	if(currentKernal&&NPPRunning)
	{
		LONG_PTR bid = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
		if(mWebView) {
			wkeLoadHTML(mWebView, "<!doctype html><meta charset=\"utf-8\"> <script src=\"mdbr://main.js\"></script><body><script>window.update=function(){APMD(GetDocText(''))};update();</script></body>");
		} else if(mWebView_1) {
			mbLoadHtmlWithBaseUrl(mWebView_1, "<!doctype html><meta charset=\"utf-8\"> <script src=\"mdbr://main.js\"></script><body><script>function onNative(msg,rsp){if(msg==0x666)window.APMD(rsp)}window.mbQuery(0x666,\"\",onNative);</script></body>", "file://");
		} else if(mWebView_2) {
			// 
			CHAR* page_id = new CHAR[64];  // LIBCEF 需要拟构网址。 传文件名，只传ID吧。 http://tests/MDT/{bid}/index.html
			int st,ed;
			strcpy(page_id, "MDT/");
			LONGPTR2STR(page_id+(st=strlen(page_id)), bid);
			strcpy(page_id+(ed=strlen(page_id)), "/index.html");
			if(fromEditor && STRENDWITH(bwGetUrl(mWebView_2), page_id))
			{ // soft update
				bwExecuteJavaScript(mWebView_2, "update()");
			}
			else
			{
				//todo extension check
				//todo doc length check
				CHAR* page_content = new CHAR[256];
				char* CustomRoutine = "MDViewer";
				if(CustomRoutine)
				{
					strcpy(page_content, "<!doctype html><meta charset=\"utf-8\"><body><script>window.update=function(){window.APMD(GetDocText1('");
				}
				else
				{
					strcpy(page_content, "<!doctype html><meta charset=\"utf-8\"><script src=\"http://mdbr/main.js\" onload=init(this)></script><body><script>window.APMD(GetDocText1('");
				}

				auto nxt_st=page_content+strlen(page_content);
				strncpy(nxt_st, page_id+st, ed-st);
				nxt_st+=ed-st;

				if(CustomRoutine)
				{
					strcpy(nxt_st, "'));}</script><body><script src=\"http://mdbr/");
					strcpy(page_content+strlen(page_content), CustomRoutine);
					strcpy(page_content+strlen(page_content), "/main.js\" onload=init(this)></script></body>");
				}
				else
				{
					strcpy(nxt_st, "'));</script></body>");
				}

				bwLoadStrData(mWebView_2, page_id, page_content, 0);
			}
			delete[] page_id;
			//bwLoadStrData(mWebView_2, url_, "<!doctype html><meta charset=\"utf-8\"><script src=\"http://mdbr/main.js\"></script><body><script>window.APMD(GetDocText1(0));</script></body>", 0);
		}
	}
}

void MarkDownTextDlg::refreshDlg(bool updateList, bool fromEditor) {
	bool AlwaysRefreshBtns=0;
	if (isCreated() && isVisible())
	{
		//::SendMessage( _hSelf, SELF_REFRESH, AlwaysRefreshBtns, updateList);
		if(!AlwaysRefreshBtns&&!updateList) {
			hasChanged=1;
		}
		RefreshWebview(fromEditor);
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
			if(currentKernal)
			{
				//if(mWebView) {
				//	wkeMoveWindow(mWebView,rc.left, rc.top+toolbarHeight, rc.right, rc.bottom-toolbarHeight);
				//	//::MoveWindow(wkeGetWindowHandle(mWebView), rc.left, rc.top, rc.right, rc.bottom,TRUE);
				//} else if(mWebView_1 && hBrowser) {
				//}
				if(hBrowser)
				{
					::MoveWindow(hBrowser, rc.left, rc.top+toolbarHeight, rc.right, rc.bottom-toolbarHeight,1);
				}
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