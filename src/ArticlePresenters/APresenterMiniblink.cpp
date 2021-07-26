#include "APresenterMiniblink.h"
#include "PluginDefinition.h"
#include "APresentee.h"
#include "shlwapi.h"

#include "SU.h"

#define kClassWindow L"MiniblinkWebs"

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
	if(strncmp(url, InternalTESTSResHead1, 17)==0)
	{
		LONG_PTR bid=0;
		int from = STR2LONGPTRA((CHAR*)url+17, bid);
		if(from&&bid)
		{
			char decodedUrl[MAX_PATH];
			auto path = url+18+from;

			if(!strcmp(path, "text")||!strcmp(path, "doc.html"))
			{
				size_t len;
				auto res=presentee->GetDocTex(len, bid, 0);
				mbNetSetData(job, res, (int)len);
				return true;
			}
			else if(!strcmp(path, "text.html"))
			{
				LONGPTR2STR(decodedUrl, bid);
				CHAR* page_content = new CHAR[512];
				int len = sprintf(page_content, "<!doctype html><meta charset=\"utf-8\"><script src=\"http://mdbr/ui.js\"></script><script>function onNative(msg,rsp){if(msg==0x666)window.APMD(rsp)};window.update=function(){window.mbQuery(0x666,\"%s\",onNative)}</script><body><script src=\"http://mdbr/", decodedUrl);
				presentee->AppendPageResidue(page_content+len); // 加载mb
				mbNetSetData(job, page_content, (int)strlen(page_content));
				return true;
			}
			UrlDecode(decodedUrl, path);
			DWORD dataLen;
			auto buffer = presentee->loadSourceAsset(0, decodedUrl, dataLen);
			if(buffer)
			{
				mbNetSetData(job, buffer, dataLen);
				//delete[] buffer;
				return true;
			}
		}
	}
	if(strncmp(url, InternalResHead1, 12)==0)
	{
		auto path = url+12;
		const utf8* decodeURL = mbUtilDecodeURLEscape(path);
		DWORD dataLen;
		auto buffer = presentee->loadPluginAsset(decodeURL, dataLen);
		if(buffer)
		{
			mbNetSetData(job, buffer, dataLen);
			return true;
		}
	}
	return false;
}

void MB_CALL_TYPE handleDocumentReady(mbWebView webView, void* param, mbWebFrameHandle frameId)
{
	OutputDebugStringA("HandleDocumentReady\n");
	mbRunJs(webView, mbWebFrameGetMainFrame(webView), "return window.onNativeRunjs('I am runjs');", TRUE, onRunJs, nullptr, nullptr);
}

void MB_CALL_TYPE handleLoadingFinish(mbWebView webView, void* param, mbWebFrameHandle frameId, const utf8* url, mbLoadingResult result, const utf8* failedReason)
{
	//if(result == MB_LOADING_SUCCEEDED)
	//::mbNetGetFavicon(webView, HandleFaviconReceived, param);
	//OutputDebugStringA("handleLoadingFinish \n");
}

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
		auto res=presentee->GetDocTex(len, 0, 0);
		mbResponseQuery(webView, queryId, customMsg, res);
		delete[] res;
	}
	else if(customMsg==0x999)
	{
		presentee->NewDoc((char*)request);
	}
	else if(customMsg==0x996)
	{
		//::SendMessage(nppData._nppHandle, NPPM_SETSTATUSBAR, STATUSBAR_DOC_TYPE, (LPARAM)L"0x996");
		if(strncmp(request, "scinllo", 7)==0)
		{
			auto number = request+7;
			bool force=number[0]==L'_';
			if(force)
				number++;
			if(GetUIBoolReverse(0) && GetUIBoolReverse(2) || force)
			{
				presentee->doScintillaScroll(atoi(number));
			}
		}
	}
	else if(customMsg==0x997)
	{
		if(strncmp(request, "getDarkBG", 9)==0)
		{
			mbResponseQuery(webView, queryId, customMsg, presentee->getDarkBG()?"123":"");
		}
	}
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
		unsigned int virtualKeyCode = (int)wParam;
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
		unsigned int virtualKeyCode = (int)wParam;
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
		unsigned int charCode = (int)wParam;
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



////////////////////////////////////////////////
////// class definition starts here       //////
////////////////////////////////////////////////

#ifdef _WIN64
#define MiniBlinkMBLibName TEXT("mb_x64.dll")
#else
#define MiniBlinkMBLibName TEXT("mb.dll")
#endif

TCHAR MBPath[MAX_PATH]{0};

APresenterMiniblink::APresenterMiniblink(TCHAR* WKPath, const TCHAR* modulePath, int & error_code, HWND & hBrowser, HWND hwnd) 
{
	mbSetMbMainDllPath(WKPath);
	TCHAR WKPath1[MAX_PATH];
	lstrcpy(WKPath1, WKPath);
	::PathRemoveFileSpec(WKPath1);
	//if (MBPath[0]&&lstrcmp(MBPath, WKPath1))
	{ // if already loaded other libs
		//mbUninit();
	}
	::PathAppend(WKPath1, MiniBlinkMBLibName);
	if(!PathFileExists(WKPath1))
	{
		lstrcpy(WKPath1, modulePath);
		::PathRemoveFileSpec(WKPath1);
		::PathAppend(WKPath1, MiniBlinkMBLibName);
	}
	error_code = -1;
	if(PathFileExists(WKPath1))
	{
		mbSetMbDllPath(WKPath1);
		mbSettings settings;
		memset(&settings, 0, sizeof(settings));
		//settings.mask = MB_ENABLE_NODEJS;
		mbInit(&settings);
		{
			mWebView = mbCreateWebView();
			if(mWebView)
			{
				lstrcpy(MBPath, WKPath1);
				::PathRemoveFileSpec(MBPath);
				error_code = 0;
				regWndClass(kClassWindow, CS_HREDRAW | CS_VREDRAW);
				hBrowser = ::CreateWindowEx(0 , kClassWindow , NULL
					, WS_CHILD , 0 , 0 , 840 , 680 , hwnd , NULL , ::GetModuleHandle(NULL), NULL);
				::SetProp(hBrowser, L"mb", (HANDLE)mWebView);
				mbSetHandle(mWebView, hBrowser);
				mbOnPaintUpdated(mWebView, handlePaintUpdatedCallback, hBrowser);
				mbOnLoadUrlBegin(mWebView, handleLoadUrlBegin, (void*)mWebView);
				mbOnDocumentReady(mWebView, handleDocumentReady, (void*)mWebView);
				mbOnLoadingFinish(mWebView, handleLoadingFinish, (void*)mWebView);
				mbOnCreateView(mWebView, handleCreateView, (void*)mWebView);
				mbSetNavigationToNewWindowEnable(mWebView, 1);
				mbSetCspCheckEnable(mWebView, false);
				mbMoveToCenter(mWebView);
				mbOnJsQuery(mWebView, onJsQuery, (void*)0);
			}
		}
	}
}

void APresenterMiniblink::Refresh() 
{
	//mbReload(mWebView);
	EvaluateJavascript("location.reload()");
}

void APresenterMiniblink::GoBack() 
{
	mbGoBack(mWebView);
}

void APresenterMiniblink::GoForward() 
{
	mbGoForward(mWebView);
}

void APresenterMiniblink::DestroyWebView(bool exit) 
{
	mbDestroyWebView(mWebView);
}

void APresenterMiniblink::EvaluateJavascript(char * JS) 
{
	mbRunJs(mWebView, mbWebFrameGetMainFrame(mWebView), JS, TRUE, 0, 0, 0);
}

void APresenterMiniblink::ResetZoom() 
{
	mbSetZoomFactor(mWebView, 1);
}

void APresenterMiniblink::ZoomOut() 
{
	float zoom=mbGetZoomFactor(mWebView)-mbzd;
	if(zoom<0.25) zoom=0.25f;
	mbSetZoomFactor(mWebView, zoom);
}

void APresenterMiniblink::ZoomIn() 
{
	float zoom=mbGetZoomFactor(mWebView)+mbzd;
	if(zoom>5) zoom=5.f;
	mbSetZoomFactor(mWebView, zoom);
}

void APresenterMiniblink::ShowDevTools(TCHAR *res_path) {
	CHAR tmp[MAX_PATH]{};
	WideCharToMultiByte(CP_ACP, 0, MBPath, -1, tmp, MAX_PATH-1, NULL, NULL);  
	::PathAppendA(tmp, ("\\front_end\\inspector.html"));
	mbSetDebugConfig(mWebView, "showDevTools", tmp);
}

void APresenterMiniblink::ShowWindow() 
{
	mbShowWindow(mWebView, TRUE);
}

void APresenterMiniblink::updateArticle(LONG_PTR bid, int articleType, bool softUpdate, bool update) 
{
	if (softUpdate&&articleType!=1&&presentee->lastBid==bid)
	{
		mbRunJs(mWebView, mbWebFrameGetMainFrame(mWebView), "update()", false, 0,0,0); // mb soft update
		return;
	}
	CHAR page_content[128]{"http://tests/MDT/"};
	if(presentee->RendererTypeIdx==1) {
		//mbLoadURL(mWebView, "http://mdbr/doc/text.html");
		if (softUpdate||update)
		{
			int to = LONGPTR2STR(page_content+17, bid);
			strcpy(page_content+17+to, "/doc.html");
			mbLoadURL(mWebView, page_content);
			if (update)
			{
				presentee->lastBid=bid;
				lstrcpy(last_updated, last_actived);
			}
		}
		return;
	}
	else if(update)
	{
		int to = LONGPTR2STR(page_content+17, bid);
		strcpy(page_content+17+to, "/text.html");
		mbLoadURL(mWebView, page_content);
		presentee->lastBid=bid;
		lstrcpy(last_updated, last_actived);
	}
}

