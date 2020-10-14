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

void doScintillaScroll(int ln)
{
	int curScintilla;
	SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&curScintilla);
	auto currrentSc = curScintilla?nppData._scintillaSecondHandle:nppData._scintillaMainHandle;
	SendMessage(currrentSc, SCI_SETFIRSTVISIBLELINE, _MDText.lastSyncLn=ln, 0);
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
	//wkeWebView newWindow = wkeCreateWebWindow(WKE_WINDOW_TYPE_POPUP, NULL, features->x, features->y, features->width, features->height);
	//wkeShowWindow(newWindow, true);
	//return newWindow;
	wkeLoadURL(webWindow, (CHAR*)url);
	return 0;
}

const char InternalResHead1[] = "http://mdbr/";
const char InternalTESTSResHead1[] = "http://tests/MDT/";


CHAR* loadPluginAsset(const char* path, DWORD & dataLen)
{
	CHAR ResPath[MAX_PATH];
#if 1 // loadPluginAsset
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

bool onLoadUrlBegin(wkeWebView webView, void* param, const char* url, void *job)
{
	if(strncmp(url, InternalResHead1, 12)==0)
	{
		auto path = url+12;
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

jsValue WKE_CALL_TYPE ScintillaScroll1(jsExecState es, void* param)
{
	TCHAR buffer[256]={0};
	wsprintf(buffer,TEXT("position=%s"), 0);
	::SendMessage(nppData._nppHandle, NPPM_SETSTATUSBAR, STATUSBAR_DOC_TYPE, (LPARAM)buffer);

	if(GetUIBoolReverse(0) && GetUIBoolReverse(2) || jsArgCount(es)>1)
	{
		if(jsArgCount(es)<=2)
		{
			jsValue arg0 = jsArg(es, 0);
			if (!jsIsNumber(arg0))
				return jsUndefined();
			LONG_PTR bid = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
			if(_MDText.lastBid==bid)
			{
				int val=jsToInt(es, arg0);
				doScintillaScroll(val);
			}
		}
	}
	return 0;
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

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
		break;
	case WM_DESTROY:
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}

BOOL regWndClassWin(LPCTSTR lpcsClassName, DWORD dwStyle)
{
	WNDCLASS wndclass = { 0 };

	wndclass.style = dwStyle;
	wndclass.lpfnWndProc = WndProc;
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
	if(strncmp(url, InternalResHead1, 12)==0)
	{
		auto path = url+12;
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
	if(customMsg==0x996)
	{
		//::SendMessage(nppData._nppHandle, NPPM_SETSTATUSBAR, STATUSBAR_DOC_TYPE, (LPARAM)L"0x996");
		if(strncmp(request, "scinllo", 7)==0)
		{
			auto number = request+7;
			bool force;
			if(force=number[0]==L'_')
				number++;
			if(GetUIBoolReverse(0) && GetUIBoolReverse(2) || force)
			{
				doScintillaScroll(atoi(number));
			}
		}
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
int LONGPTR2STR(TCHAR* STR, LONG_PTR LONGPTR)
{
	TCHAR* start=STR;
	int cc=0;
	while(LONGPTR)
	{
		*(STR++)='0'+(LONGPTR%10);
		LONGPTR/=10;
		cc++;
	}
	*STR='\0';
	wcsrev(start);
	return cc;
}
#endif

int LONGPTR2STR(CHAR* STR, LONG_PTR LONGPTR)
{
	CHAR* start=STR;
	int cc=0;
	while(LONGPTR)
	{
		*(STR++)='0'+(LONGPTR%10);
		LONGPTR/=10;
		cc++;
	}
	*STR='\0';
	strrev(start);
	return cc;
}

BJSCV* GetDocText1(LONG_PTR funcName, int argc, LONG_PTR argv, int sizeofBJSCV)
{
	//::MessageBoxA(NULL, ("GetDocText1"), (""), MB_OK);
	if(argc<0)
	{
		if(argv==sizeofBJSCV==0)
			delete (BJSCV*)funcName;
		return 0;
	}
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


LRESULT WINAPI testWindowProc1(
	__in HWND hWnd,
	__in UINT msg,
	__in WPARAM wParam,
	__in LPARAM lParam)
{
	LRESULT result = 0;

	switch (msg) {
		case WM_NCDESTROY:
			return 0;

		case WM_ERASEBKGND:
			return TRUE;

		case WM_SIZE:
		{
			return 0;
		}
		case WM_KEYDOWN:
		{
			break;
		}
		case WM_KEYUP:
		{
			::MessageBox(NULL, TEXT("111"), TEXT(""), MB_OK);
			break;
		}
		case WM_CHAR:
		{
			break;
		}
		case WM_MOUSEWHEEL:
		{
			break;
		}
		case WM_SETFOCUS:
			return 0;

		case WM_KILLFOCUS:
			return 0;

		case WM_SETCURSOR:
			break;

		case WM_IME_STARTCOMPOSITION: {
			break;
		}
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

BJSCV* ScintillaScroll(LONG_PTR funcName, int argc, LONG_PTR argv, int sizeofBJSCV)
{
	if(GetUIBoolReverse(0) && GetUIBoolReverse(2) || argc>1)
	{
		int structSize=0;
		if(argc<=2)
		{
			LONG_PTR bid = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
			if(_MDText.lastBid==bid)
			{
				char* args = bwParseCefV8Args(argv, structSize);
				if(structSize)
				{
					BJSCV* val = (BJSCV*)(args+0*structSize);
					if(val->value_type==typeInt)
					{
						doScintillaScroll(val->intVal);
					}
				}
			}
		}
	}
	return 0;
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
		if(_MDText.mWebView) {
			wkeRunJS(_MDText.mWebView, jsSync);
		} else if(_MDText.mWebView_1){
			mbRunJs(_MDText.mWebView_1, mbWebFrameGetMainFrame(_MDText.mWebView_1), jsSync, TRUE, 0, 0, 0);
		} else if(mWebView_2){
			bwExecuteJavaScript(mWebView_2, jsSync);
		} else if(mWebView_3) {
			TCHAR jsSync1[64];
			MultiByteToWideChar(CP_ACP, 0, jsSync, -1, jsSync1, 64);
			mWebView_3->ExecuteScript(jsSync1, 0);
		}
		lastSyncLn=line;
	}
}

void onBrowserPrepared(bwWebView browserPtr)
{
	bwInstallJsNativeToWidget(browserPtr, "GetDocText1", GetDocText1);
	bwInstallJsNativeToWidget(browserPtr, "Scinllo", ScintillaScroll);
	_MDText.mWebView_2 = browserPtr;
	_MDText.currentkernel = (LONG_PTR)browserPtr;
	_MDText.hBrowser = bwGetHWNDForBrowser(browserPtr);
	::SendMessage(_MDText.getHSelf(), WM_SIZE, 0, 0);
	//_MDText.RefreshWebview(); 没有用
	//SetWindowLongPtr(_MDText.hBrowser, GWLP_WNDPROC, (LONG_PTR)testWindowProc1);
}

bool onBrowserFocused(bwWebView browserPtr)
{
	HWND hwnd = bwGetHWNDForBrowser(browserPtr);
	if(hwnd==_MDText.hBrowser) {
		::SendMessage(nppData._nppHandle, NPPM_SETDOCKFOCUS, (WPARAM)hwnd, 0);
	}
	return false;
}

bool onBrowserClose(bwWebView browserPtr)
{
	HWND hwnd = bwGetHWNDForBrowser(browserPtr);
	if(hwnd==_MDText.hBrowser) {
		::SendMessage(nppData._nppHandle, NPPM_CLOSEDOCK, (WPARAM)hwnd, 0);
	}
	return true;
}

unsigned char FromHex(unsigned char x)   
{   
	unsigned char y;  
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;  
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;  
	else if (x >= '0' && x <= '9') y = x - '0';  
	else y=0;
	return y;  
}  

void UrlDecode(char* dest, const char* str)  
{  
	size_t length = strlen(str);
	char* ddd=dest;
	for (size_t i = 0; i < length && ddd-dest+1<MAX_PATH_HALF; i++)  
	{  
		if (str[i] == '+') {
			*ddd++ = ' ';  
		}
		else if (str[i] == '%')  
		{  
			assert(i + 2 < length);  
			unsigned char high = FromHex(str[++i]);  
			unsigned char low = FromHex(str[++i]);  
			*ddd++ = high*16 + low;  
		}
		else if (str[i] == '?')  
		{  
			break;
		}
		else {
			*ddd++ = str[i];  
		}
	}
	*ddd='\0';
}  

void UrlDecode(char* dest, const TCHAR* str)  
{  
	size_t length = lstrlen(str);
	char* ddd=dest;
	for (size_t i = 0; i < length && ddd-dest+1<MAX_PATH_HALF; i++)  
	{  
		if (str[i] == '+') {
			*ddd++ = ' ';  
		}
		else if (str[i] == '%')  
		{  
			assert(i + 2 < length);  
			unsigned char high = FromHex(str[++i]);  
			unsigned char low = FromHex(str[++i]);  
			*ddd++ = high*16 + low;  
		}
		else if (str[i] == '?')  
		{  
			break;
		}
		else {
			*ddd++ = str[i];  
		}
	}
	*ddd='\0';
}  

url_intercept_result* InterceptBrowserWidget(const char* url, const url_intercept_result* ret)
{
	if(!url)
	{
		if(ret)
			delete ret;
		return 0;
	}
	if(strncmp(url, "https://tests/home", 18)==0)
	{
		_MDText.RefreshWebview(false);
		return new url_intercept_result{(CHAR*)"Markdown Text", 14, 200, (CHAR*)"OK"};
	}

	if(strncmp(url, InternalResHead1, 12)==0)
	{
		auto path = url+12;
		if(path)
		{
			if(strstr(path, "..")) // security check
			{
				return false;
			}
			char decodedUrl[MAX_PATH_HALF];
			UrlDecode(decodedUrl, path);

			DWORD dataLen;
			auto buffer = loadPluginAsset(decodedUrl, dataLen);

			if(buffer)
			{
				return new url_intercept_result{buffer, dataLen, 200, (CHAR*)"OK"};
			}
		}
	}
	return nullptr;
}

clock_t browser_deferred_create_time=0;
char wke_mb=0;


// Global variables

// The main window class name.
static TCHAR szWindowClass[] = _T("DesktopApp");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("WebView sample");

HINSTANCE hInst;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Pointer to WebViewController
// Pointer to WebView window


using namespace Microsoft::WRL;
void MarkDownTextDlg::display(bool toShow){
	DockingDlgInterface::display(toShow);

	setClosed(!toShow);

	if(toShow && currentkernel==0) 
	{
		TCHAR WKPath[MAX_PATH]={0};
		TCHAR WKPath1[MAX_PATH]={0};
		::GetModuleFileName((HINSTANCE)g_hModule, WKPath, MAX_PATH);
		::PathRemoveFileSpec(WKPath);
		int error_code=0;
		if(PathFileExists(WKPath))
		{ // ThriveEngines !
			bool browser_deferred_creating=false;
			if(browser_deferred_create_time)
			{
				clock_t time = clock();
				if(time-browser_deferred_create_time<550)
					browser_deferred_creating=1;
				else
					browser_deferred_create_time=0;
			}
			// Webview2
			if(!browser_deferred_creating && !currentkernel && kernelType==3)
			{
				currentkernelType=3;
				regWndClassWin(L"WINWND", CS_HREDRAW | CS_VREDRAW);
				HWND hWnd = ::CreateWindowEx(0 , L"WINWND" , NULL
					, WS_CHILD , 0 , 0 , 840 , 680 , _hSelf , NULL , ::GetModuleHandle(NULL), NULL);
				hBrowser=hWnd;
				ShowWindow(hWnd, 1);
				//UpdateWindow(hWnd);
				auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
				HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(0,0,0,
					Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>([this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
					m_webViewEnvironment=env;
					env->CreateCoreWebView2Controller(hBrowser, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>([this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
						if (controller) {
							webviewController = controller;
							webviewController->get_CoreWebView2(&mWebView_3);
						}
						ICoreWebView2Settings* Settings;
						mWebView_3->get_Settings(&Settings);
						Settings->put_IsScriptEnabled(TRUE);
						Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
						Settings->put_IsWebMessageEnabled(TRUE);

						currentkernel = (LONG_PTR)mWebView_3.get();

						SendMessage(_hSelf, WM_SIZE, 0, 0);

						static EventRegistrationToken pageEventRegistrationToken = {};
						mWebView_3->AddWebResourceRequestedFilter(L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
						CHECK_FAILURE(mWebView_3->add_WebResourceRequested(
							Callback<ICoreWebView2WebResourceRequestedEventHandler>([this](
									ICoreWebView2* sender,
									ICoreWebView2WebResourceRequestedEventArgs* args) {
							COREWEBVIEW2_WEB_RESOURCE_CONTEXT resourceContext;
							CHECK_FAILURE(args->get_ResourceContext(&resourceContext));
							// Ensure that the type is image
							wil::com_ptr<ICoreWebView2WebResourceRequest> req;
							args->get_Request(&req);
							wil::unique_cotaskmem_string navigationTargetUri;
							req->get_Uri(&navigationTargetUri);
							TCHAR* uriTarget=(TCHAR*)navigationTargetUri.get();

							if(wcsncmp(uriTarget, TEXT("http://tests/MDT/"), 17)==0)
							{ // constuct page here
							  //::MessageBox(NULL, TEXT("来了"), TEXT(""), MB_OK);
								auto end=uriTarget+wcslen(uriTarget);
								uriTarget+=17;
								auto PageId=uriTarget;
								for(;uriTarget<end;uriTarget++)
								{
									if(*uriTarget=='/') 
									{
										*uriTarget++='\0';
										break;
									}
								}
								LONG_PTR bid=0;
								STR2LONGPTR(PageId, bid);
								if(bid)
								{
									if (resourceContext == COREWEBVIEW2_WEB_RESOURCE_CONTEXT_DOCUMENT)
									{
										CHAR* page_content = new CHAR[1024];
										//post message
										//strcpy(page_content, "<!doctype html><meta charset=\"utf-8\"><script>var w=window,wv=w.chrome.webview;wv.addEventListener(\'message\', event => APMD(event.data));w.update=function(){wv.postMessage('");
										//auto nxt_st=page_content+strlen(page_content);
										//nxt_st+=LONGPTR2STR(nxt_st, bid);
										//strcpy(nxt_st, "');}</script><body><script src=\"http://mdbr/");

										//xhr http
										strcpy(page_content, "<!doctype html><meta charset=\"utf-8\"><script src=\"http://mdbr/ui.js\"></script><script>var w=window;w.update=function(){var R=new XMLHttpRequest();R.open('POST','");
										auto nxt_st=page_content+strlen(page_content);
										nxt_st+=LONGPTR2STR(nxt_st, bid);
										strcpy(nxt_st, ".text',true);R.onreadystatechange=function(){APMD(R.responseText)};R.send()}</script><body><script src=\"http://mdbr/");
										
										nxt_st+=strlen(nxt_st);
										AppendPageResidue(nxt_st); // 加载WV2

										wil::com_ptr<ICoreWebView2WebResourceResponse> response;
										wil::com_ptr<IStream> stream = SHCreateMemStream((const BYTE*)page_content, strlen(page_content));

										CHECK_FAILURE(m_webViewEnvironment->CreateWebResourceResponse(stream.get(), 200, L"OK", L"charset=utf-8", &response));

										wil::com_ptr<ICoreWebView2HttpResponseHeaders> headers;
										CHECK_FAILURE(response->get_Headers(&headers));
										headers->AppendHeader(L"Content-Type", L"text/html; charset=utf-8");

										CHECK_FAILURE(args->put_Response(response.get()));
									}
									else
									{
										size_t docLength=0;
										auto str = GetDocTex(docLength, bid);
										if(docLength)
										{
											wil::com_ptr<ICoreWebView2WebResourceResponse> response;
											wil::com_ptr<IStream> stream = SHCreateMemStream((const BYTE*)str, docLength);
											CHECK_FAILURE(m_webViewEnvironment->CreateWebResourceResponse(stream.get(), 200, L"OK", L"charset=utf-8", &response));
											wil::com_ptr<ICoreWebView2HttpResponseHeaders> headers;
											CHECK_FAILURE(response->get_Headers(&headers));
											headers->AppendHeader(L"Content-Type", L"text/*; charset=utf-8");
											CHECK_FAILURE(args->put_Response(response.get()));
										}
									}
								}
								return S_OK;
							}
							else if(wcsncmp(uriTarget, TEXT("http://mdbr/"), 12)==0)
							{
								auto path = uriTarget+12;
								if(path)
								{
									if(wcsstr(path, L"..")) // security check
									{
										return E_INVALIDARG;
									}
									else
									{
										char decodedUrl[MAX_PATH_HALF];
										UrlDecode(decodedUrl, path);

										DWORD dataLen;
										auto buffer = loadPluginAsset(decodedUrl, dataLen);

										if(buffer)
										{
											wil::com_ptr<ICoreWebView2WebResourceResponse> response;
											wil::com_ptr<IStream> stream = SHCreateMemStream((const BYTE*)buffer, dataLen);
											CHECK_FAILURE(m_webViewEnvironment->CreateWebResourceResponse(stream.get(), 200, L"OK", L"charset=utf-8", &response));
											wil::com_ptr<ICoreWebView2HttpResponseHeaders> headers;
											CHECK_FAILURE(response->get_Headers(&headers));
											headers->AppendHeader(L"Content-Type", L"*/*; charset=utf-8");
											CHECK_FAILURE(args->put_Response(response.get()));
										}
									}
								}
							}
							return E_INVALIDARG;
						}).Get(), &pageEventRegistrationToken));

						EventRegistrationToken token;
						if(0)
						mWebView_3->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
							[](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs * args) -> HRESULT {
							PWSTR message;
							args->TryGetWebMessageAsString(&message);
							LONG_PTR bid=0;
							STR2LONGPTR(message, bid);
							if(bid)
							{
								// very slow !
								//::MessageBox(NULL, TEXT("收到！"), TEXT("收到！"), MB_OK);
								size_t docLength=0;
								auto str = GetDocTex(docLength, bid);
								if(docLength)
								{
									TCHAR* buffer = new TCHAR[docLength];
									MultiByteToWideChar(CP_ACP, 0, str, -1, buffer, docLength);
									webview->PostWebMessageAsString(buffer);
									delete[] buffer;
									//webview->PostWebMessageAsString(L"# hello");
								}
							}
							CoTaskMemFree(message);
							return S_OK;
						}).Get(), &token);

						mWebView_3->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
							[](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs * args) -> HRESULT {
							PWSTR message;
							args->TryGetWebMessageAsString(&message);
							if(wcsncmp(message, L"scinllo", 7)==0)
							{
								auto number = message+7;
								bool force;
								if(force=number[0]==L'_')
									number++;
								if(GetUIBoolReverse(0) && GetUIBoolReverse(2) || force)
								{
									doScintillaScroll(_wtoi(number));
								}
							}
							CoTaskMemFree(message);
							return S_OK;
						}).Get(), &token);

						if(0)
						mWebView_3->AddScriptToExecuteOnDocumentCreated(
							L"window.chrome.webview.addEventListener(\'message\', event => alert(event.data));" \
							L"window.chrome.webview.postMessage(window.document.URL);",
							nullptr);
						//mWebView_3->Navigate(L"https://www.bing.com/");
						//mWebView_3->Navigate(L"http://tests/MDT/1.html");

						RefreshWebview();
						return S_OK;
					}).Get());
					return S_OK;
				}).Get());
				if (SUCCEEDED(hr))
				{
					browser_deferred_creating=1;
					browser_deferred_create_time = clock();
				}
			}
			// BrowserWidget
			bool prefer_bw = kernelType==2||kernelType==-1;
			if(!browser_deferred_creating && !currentkernel && prefer_bw)
			{
				currentkernelType=2;
				auto libPath = LibPath;
				if(!libPath)
				{
					lstrcpy(WKPath1, WKPath);
					::PathAppend(WKPath1, L"..\\BrowserWidget\\cefclient.dll");
					libPath = WKPath1;
				}
				if(PathFileExists(libPath))
				{
					if(bwInit(libPath) && bwCreateBrowser({_hSelf, "https://tests/home", onBrowserPrepared, InterceptBrowserWidget, onBrowserFocused, onBrowserClose}))
					{
						browser_deferred_create_time = clock();
						browser_deferred_creating=1;
						//::MessageBox(NULL, TEXT("111"), TEXT(""), MB_OK);
					}
				}
			}
			// mb and wke
			if(!browser_deferred_creating)
			{
				::PathAppend(WKPath, L"miniblink_x64.dll");
				if(PathFileExists(WKPath))
				{
					// mb
					if(!currentkernel && kernelType!=0)
					{
						currentkernelType=1;
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
								wke_mb|=0x2;
								mWebView_1 = mbCreateWebView();
								if(currentkernel=mWebView_1)
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
									mbOnJsQuery(mWebView_1, onJsQuery, (void*)0);
								}
							}
						}
					}
					// wke
					if(!currentkernel && kernelType<=1)
					{
						currentkernelType=0;
						wkeSetWkeDllPath(WKPath);
						if(wkeInitialize()) 
						{
							wke_mb|=0x1;
							mWebView = wkeCreateWebWindow(WKE_WINDOW_TYPE_CONTROL, _hSelf , 0, 0, 640, 480); 
							if (currentkernel=(intptr_t)mWebView)
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
								wkeJsBindFunction("Scinllo", &ScintillaScroll1, nullptr, 1);
							} else {
								error_code = 101;
							}
						} else {
							error_code = 100;
						}
					}
				}
			}
			if(currentkernel)
			{
				if(mWebView) {
					RefreshWebview();
					wkeShowWindow(mWebView, TRUE);
				} else if(mWebView_1){
					RefreshWebview();
					mbShowWindow(mWebView_1, TRUE);
				}
			}
		}
	}

	//::SendMessage( _hSelf, SELF_REFRESH, 0, 1);
};

void MarkDownTextDlg::setClosed(bool toClose) {
	_isClosed = toClose;
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcMenu->_cmdID, !toClose);
}

std::string page_data="<!doctype html><meta charset=\"utf-8\"><script src=\"http://mdbr/main.js\"></script><body><script>window.APMD(GetDocText1(''));</script></body>";
//std::string page_data="";

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

bool STRENDWITH(TCHAR* strA,TCHAR* strB)
{
	int valen = lstrlen(strA);
	int pc = lstrlen(strB);
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

bool checkRenderMarkdown() { 
	if(bForcePreview)
		return 1;
	if(markdown_ext.size()==0) {
		markdown_ext.push_back(".md");
		markdown_ext.push_back(".md.html");
		markdown_ext.push_back(".svg");
	}
	return checkFileExt(markdown_ext);
}

bool checkRenderHtml() { 
	if(html_ext.size()==0) {
		html_ext.push_back(".html");
	}
	return checkFileExt(html_ext);
}

void MarkDownTextDlg::RefreshWebview(bool fromEditor) {
	if(currentkernel&&NPPRunning)
	{
		LONG_PTR bid = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
		//CustomRoutine = "MDViewer";
		char b1=fromEditor&&lastBid==bid, b2=lastBid!=bid;
		//CustomRoutine = 0;
		if(mWebView) {
			if(b1)
			{
				wkeRunJS(mWebView, "update()"); // wke soft update
			}
			else if(b2)
			{
				if(checkRenderMarkdown()) {
					CHAR* page_content = new CHAR[512];
					strcpy(page_content, "<!doctype html><meta charset=\"utf-8\"><script src=\"http://mdbr/ui.js\"></script><script>window.update=function(){APMD(GetDocText(''))}</script><body><script src=\"http://mdbr/");	
					AppendPageResidue(page_content+172); // 加载wke
					wkeLoadHTML(mWebView, page_content);
					lastBid=bid;
					lstrcpy(last_updated, last_actived);
					//wkeLoadHTML(mWebView, "<!doctype html><meta charset=\"utf-8\"> <script src=\"http://mdbr/main.js\"></script><body><script>window.update=function(){APMD(GetDocText(''))};update();</script></body>");
				}
			}
		} 
		else if(mWebView_1) {
			if(b1)
			{
				mbRunJs(mWebView_1, mbWebFrameGetMainFrame(mWebView_1), "update()", false, 0,0,0); // mb soft update
			}
			else if(b2)
			{
				if(checkRenderMarkdown()) {
					CHAR* page_content = new CHAR[512];
					strcpy(page_content, "<!doctype html><meta charset=\"utf-8\"><script src=\"http://mdbr/ui.js\"></script><script>function onNative(msg,rsp){if(msg==0x666)window.APMD(rsp)};window.update=function(){window.mbQuery(0x666,\"\",onNative)}</script><body><script src=\"http://mdbr/");
					AppendPageResidue(page_content+244); // 加载mb
					mbLoadHtmlWithBaseUrl(mWebView_1, page_content, "file://");
					lastBid=bid;
					lstrcpy(last_updated, last_actived);
					//mbLoadHtmlWithBaseUrl(mWebView_1, "<!doctype html><meta charset=\"utf-8\"><script src=\"http://mdbr/main.js\"></script><body><script>function onNative(msg,rsp){if(msg==0x666)window.APMD(rsp)}window.mbQuery(0x666,\"\",onNative);</script></body>", "file://");
				}
			}
		} 
		else if(mWebView_2) {
			CHAR* page_id = new CHAR[64];  // LIBCEF 需要拟构网址。 传文件名，只传ID吧。 http://tests/MDT/{bid}/index.html
			int st,ed;
			strcpy(page_id, "MDT/");
			LONGPTR2STR(page_id+(st=strlen(page_id)), bid);
			strcpy(page_id+(ed=strlen(page_id)), "/index.html");
			if(b1 && STRENDWITH(bwGetUrl(mWebView_2), page_id))
			{
				bwExecuteJavaScript(mWebView_2, "update()"); // bw soft update
			}
			else if(b2)
			{
				if(checkRenderMarkdown()) {
					CHAR* page_content = new CHAR[512];
					strcpy(page_content, "<!doctype html><meta charset=\"utf-8\"><script src=\"http://mdbr/ui.js\"></script><script>window.update=function(){window.APMD(GetDocText1('");

					auto nxt_st=page_content+136;
					strncpy(nxt_st, page_id+st, ed-st);
					nxt_st+=ed-st;

					strcpy(nxt_st, "'));}</script><body><script src=\"http://mdbr/");

					AppendPageResidue(nxt_st+45); // 加载bw
					bwLoadStrData(mWebView_2, page_id, page_content, 0);
					lastBid=bid;
					lstrcpy(last_updated, last_actived);
				}// else if(!legacy && checkRenderHtml()){

					//lastBid=0;
				//}
			}
			delete[] page_id;
			//bwLoadStrData(mWebView_2, url_, "<!doctype html><meta charset=\"utf-8\"><script src=\"http://mdbr/main.js\"></script><body><script>window.APMD(GetDocText1(0));</script></body>", 0);
		}
		else if(mWebView_3)
		{
			TCHAR* page_id = new TCHAR[64], * new_st;  // Webview2 需要虚拟网址。 传文件名，只传ID吧。 http://tests/MDT/{bid}/index.html
			lstrcpy(page_id, TEXT("http://tests/MDT/"));
			new_st=page_id+17;
			new_st+=LONGPTR2STR(new_st, bid);
			lstrcpy(new_st, TEXT("/index.html"));

			wil::unique_cotaskmem_string uri;
			mWebView_3->get_Source(&uri);
			if(b1 && STRENDWITH(uri.get(), page_id))
			{
				mWebView_3->ExecuteScript(L"update()", 0); // WV2 soft update
			}
			else if(b2)
			{
				if(checkRenderMarkdown()) {
					mWebView_3->Navigate(page_id);
					lastBid=bid;
					lstrcpy(last_updated, last_actived);
				}// else if(!legacy && checkRenderHtml()){

				 //lastBid=0;
				 //}
			}
		}
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
			//rc.top+=100;
			//rc.bottom-=100;
			if(currentkernel)
			{
				if(hBrowser)
				{
					if (webviewController) {
						webviewController->put_Bounds(rc);
					}
					::MoveWindow(hBrowser, rc.left, rc.top+toolbarHeight, rc.right, rc.bottom-toolbarHeight,1);
				}
				//if(mWebView) {
				//	wkeMoveWindow(mWebView,rc.left, rc.top+toolbarHeight, rc.right, rc.bottom-toolbarHeight);
				//	//::MoveWindow(wkeGetWindowHandle(mWebView), rc.left, rc.top, rc.right, rc.bottom,TRUE);
				//} else if(mWebView_1 && hBrowser) {
				//}
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

void SwitchEngineMarkdown(int idx);

void GlobalOnPvMnChecked(HMENU hMenu, int idx);

void simulToolbarMenu(HMENU pluginMenu, RECT *rc, HWND _hSelf, std::vector<FuncItem> & items){
	int cmd = TrackPopupMenu(pluginMenu, TPM_RETURNCMD, rc->left,  rc->top, 0, _hSelf, NULL);

	if(cmd) {
		for(int idx=0,len=items.size();idx<len;idx++) {
			if(items[idx]._cmdID==cmd) {
				if(items[idx]._pFunc==(PFUNCPLUGINCMD)SwitchEngineMarkdown) {
					SwitchEngineMarkdown(idx-MDCRST);
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

HMENU hMenuEngines=0;

HMENU hMenuZoom=0;

void happy(){}

int requestedInvalidSwitch=-1;

bool MDEngineScanned;

void MarkDownTextDlg::saveParameters()
{
	int core=requestedInvalidSwitch>0?requestedInvalidSwitch-1:kernelType;
	PutProfInt("BrowserKernel", core);
	PutProfString("MarkdownEngine", CustomRoutine);
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
	if(val=GetProfString("MarkdownEngine"))
	{
		strcpy(CustomRoutine, val->data());
	}
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

void MarkDownTextDlg::switchEngineByIndex(int id)
{
	if(currentkernelType!=id)
	{
		if(id<2&&wke_mb&&(id+1)^wke_mb)
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
		if(currentkernel)
		{
			if(mWebView)
			{
				wkeDestroyWebView(mWebView);
				mWebView=0;
			}
			if(mWebView_1)
			{
				mbDestroyWebView(mWebView_1);
				mWebView_1=0;
			}
			if(mWebView_2)
			{
				bwDestroyWebview(mWebView_2);
				mWebView_2=0;
			}
			if(mWebView_3)
			{
				webviewController->Close();
			}
			currentkernel=0;
		}
		if(IsWindow(hBrowser))
		{
			CloseWindow(hBrowser);
			DestroyWindow(hBrowser);
		}
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
			if(_MDText.mWebView) {
				wkeRunJS(_MDText.mWebView, "doScintillo(1)");
			} else if(_MDText.mWebView_1){
				mbRunJs(_MDText.mWebView_1, mbWebFrameGetMainFrame(_MDText.mWebView_1), "doScintillo(1)", TRUE, 0, 0, 0);
			} else if(_MDText.mWebView_2){
				bwExecuteJavaScript(_MDText.mWebView_2, "doScintillo(1)");
			} else if(_MDText.mWebView_3){
				//if(mWebView_3->CanGoBack())
				_MDText.mWebView_3->ExecuteScript(L"doScintillo(1)", 0);
			}
		break;
		case 265:
			_MDText.syncWebToline(true);
		break;
	}
}

void SwitchEngineMarkdown(int idx) {
	if(idx<-1)
	{
		MDEngineScanned=false;
		MDEngines.clear();
		return;
	}
	else if(idx==-1)
	{
		_MDText.CustomRoutine[0]='\0';
	}
	else if(idx>=0&&idx<MDEngines.size())
	{
		WideCharToMultiByte(CP_ACP, 0, MDEngines[idx].data(), -1, _MDText.CustomRoutine, MAX_PATH, NULL, NULL);
	}
	for(int i=-1,len=MDEngines.size();i<len;i++)
	{
		CheckMenuItem(hMenuEngines, i+MDCRST, MF_BYPOSITION|(i==idx?MF_CHECKED:MF_UNCHECKED));
	}
	_MDText.lastBid=0;
	_MDText.RefreshWebview();
}

void ResetZoom(){
	if(_MDText.mWebView) {
		wkeSetZoomFactor(_MDText.mWebView, 1);
	} else if(_MDText.mWebView_1){
		mbSetZoomFactor(_MDText.mWebView_1, 1);
	} else if(_MDText.mWebView_2){
		bwSetZoomLevel(_MDText.mWebView_2, 0);
	} else if(_MDText.mWebView_3){
		_MDText.webviewController->put_ZoomFactor(1);
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
				if(_MDText.mWebView) {
					wkeShowDevtools(_MDText.mWebView, L"C:\\tmp\\miniblink-20200824\\front_end\\inspector.html", 0, 0);
				} else if(_MDText.mWebView_1){
					mbSetDebugConfig(_MDText.mWebView_1, "showDevTools", "C:\\tmp\\miniblink-20200824\\front_end\\inspector.html");
				} else if(_MDText.mWebView_2){
					bwShowDevTools(_MDText.mWebView_2);
				} else if(_MDText.mWebView_3){
					_MDText.mWebView_3->OpenDevToolsWindow();
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
			if(currentkernel) {
				if(mWebView) {
					//if(wkeGoBack(mWebView))
					wkeGoBack(mWebView);
				} else if(mWebView_1){
					mbGoBack(mWebView_1);
				} else if(mWebView_2){
					//if(bwCanGoBack(mWebView_2))
					bwGoBack(mWebView_2);
				} else if(mWebView_3){
					//if(mWebView_3->CanGoBack())
					mWebView_3->GoBack();
				}
			}
		return;
		case IDM_EX_UP:
			if(currentkernel) {
				if(mWebView) {
					//if(wkeGoForward(mWebView))
					wkeGoForward(mWebView);
				} else if(mWebView_1){
					mbGoForward(mWebView_1);
				} else if(mWebView_2){
					//if(bwCanGoForward(mWebView_2))
					bwGoForward(mWebView_2);
				} else if(mWebView_2){
					//if(mWebView_3->CanGoForward())
					mWebView_3->GoForward();
				}
			}
		return;
		case IDM_EX_REFRESH:
			if(source==0)
			{
				if(currentkernel) {
					lastBid=0;
					RefreshWebview();
				}
			}
		return;
		case IDM_EX_ZOO:
			#define mbzd 0.25
			if(source==0)
			{
				if(mWebView) {
					//if(wkeGoForward(mWebView))
					float zoom=wkeZoomFactor(mWebView)-mbzd;
					if(zoom<0.25) zoom=0.25;
					wkeSetZoomFactor(mWebView, zoom);
				} else if(mWebView_1){
					float zoom=mbGetZoomFactor(mWebView_1)-mbzd;
					if(zoom<0.25) zoom=0.25;
					mbSetZoomFactor(mWebView_1, zoom);
				} else if(mWebView_2){
					bwZoomLevelDelta(mWebView_2, -1);
				} else if(mWebView_3){
					double zoom;
					webviewController->get_ZoomFactor(&zoom);
					webviewController->put_ZoomFactor(zoom-0.15);
				}
				return;
			}
		case IDM_EX_ZOI:
		{
			if(source==0)
			{
				if(mWebView) {
					//if(wkeGoForward(mWebView))
					float zoom=wkeZoomFactor(mWebView)+mbzd;
					if(zoom>5) zoom=5;
					wkeSetZoomFactor(mWebView, zoom);
				} else if(mWebView_1){
					float zoom=mbGetZoomFactor(mWebView_1)+mbzd;
					if(zoom>5) zoom=5;
					mbSetZoomFactor(mWebView_1, zoom);
				} else if(mWebView_2){
					bwZoomLevelDelta(mWebView_2, 1);
				} else if(mWebView_3){
					double zoom;
					webviewController->get_ZoomFactor(&zoom);
					webviewController->put_ZoomFactor(zoom+0.15);
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
				EngineSwicther.at(6)={TEXT(""), (PFUNCPLUGINCMD)SwitchEngineMarkdown, 66, false, 0};
				lstrcpy(EngineSwicther.at(6)._itemName, ZH_CN?TEXT("切换渲染引擎："):TEXT("Switch Markdown Engine :"));
				EngineSwicther.at(7)={TEXT("md.html"), (PFUNCPLUGINCMD)SwitchEngineMarkdown, 67, false, 0};
			}
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
				EngineSwicther.resize(MDCRST+MDEngines.size());
				while(GetMenuItemCount(hMenuEngines)>MDCRST)
					RemoveMenu(hMenuEngines, MDCRST, MF_BYPOSITION);
				int foundCheck=0;
				if(CustomRoutine[0])
				{
					MultiByteToWideChar(CP_ACP, 0, CustomRoutine, -1, path_buffer, MAX_PATH);
				}
				else // 默认引擎 md.html
				{
					CheckMenuItem(hMenuEngines, MDCRST-1, MF_BYPOSITION|MF_CHECKED);
				}
				for(int i=0,ii,len=MDEngines.size();i<len;i++)
				{
					ii=MDCRST+i;
					EngineSwicther.at(ii)={TEXT(""), (PFUNCPLUGINCMD)SwitchEngineMarkdown, 60+MDCRST+i, false, 0};
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
				MDEngineScanned=1;
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