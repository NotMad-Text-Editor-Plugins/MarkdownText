#include "APresenterMiniblink.h"
#include "PluginDefinition.h"
#include "APresentee.h"

#include "shlwapi.h"

#include "SU.h"

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

bool onLoadUrlBegin(wkeWebView webView, void* param, const char* url, void *job)
{
	if(strncmp(url, InternalTESTSResHead1, 17)==0)
	{
		LONG_PTR bid=0;
		int from = STR2LONGPTRA((CHAR*)url+17, bid);
		if(from&&bid) {
			char decodedUrl[MAX_PATH];
			auto path = url+18+from;
			if (strcmp(path, "text.html")==0)
			{
				LONGPTR2STR(decodedUrl, bid);
				CHAR* page_content = new CHAR[512];
				int len = sprintf(page_content, "<!doctype html><meta charset=\"utf-8\"><script src=\"http://mdbr/ui.js\"></script><script>window.update=function(){APMD(GetDocText('%s'))}</script><body><script src=\"http://mdbr/", decodedUrl);
				presentee->AppendPageResidue(page_content+len); // 加载wke
				wkeNetSetData(job, page_content, (int)strlen(page_content));
				return true;
			}
			else if (strcmp(path, "doc.html")==0)
			{
				size_t len;
				char* data = presentee->GetDocTex(len, bid, 0);
				wkeNetSetData(job, data, (int)len);
				return true;
			}
			UrlDecode(decodedUrl, path);
			DWORD dataLen;
			auto data = presentee->loadSourceAsset(bid, decodedUrl, dataLen);
			if(data) {
				wkeNetSetData(job, data, dataLen);
				return true;
			}
		}
	}
	if(strncmp(url, InternalResHead1, 12)==0)
	{
		auto path = url+12;
		const utf8* decodeURL = wkeUtilDecodeURLEscape(path);
		if(decodeURL)
		{
			DWORD dataLen;
			auto buffer = presentee->loadPluginAsset(path, dataLen);
			if (!buffer)
			{
				buffer = presentee->loadSourceAsset(presentee->lastBid, path, dataLen);
			}
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

//synchronous callback
jsValue WKE_CALL_TYPE GetDocText(jsExecState es, void* param)
{
	if (1 == jsArgCount(es))
	{
		jsValue arg0 = jsArg(es, 0);
		if (jsIsString(arg0))
		{
			std::string path = jsToTempString(es, arg0);
			LONG_PTR bid=0;
			STR2LONGPTRA((CHAR*)path.data(), bid);
			size_t len;
			auto ret=jsString(es, presentee->GetDocTex(len, bid, 0));
			return ret;
		}
	}
	return jsString(es, "# Hello World!");
}

jsValue WKE_CALL_TYPE NewDoc(jsExecState es, void* param)
{
	if (1 == jsArgCount(es))
	{
		jsValue arg0 = jsArg(es, 0);
		if (jsIsString(arg0))
		{
			std::string text = jsToTempString(es, arg0);
			presentee->NewDoc((char*)text.c_str());
			return 0;
		}
	}
	return jsString(es, "# Hello World!");
}

jsValue WKE_CALL_TYPE getDarkBG(jsExecState es, void* param)
{
	//LogIs(3, L"position=%s", 0);
	return jsInt(presentee->getDarkBG());
}

jsValue WKE_CALL_TYPE ScintillaScroll1(jsExecState es, void* param)
{
	//LogIs(3, L"position=%s", 0);

	if(GetUIBoolReverse(0) && GetUIBoolReverse(2) || jsArgCount(es)>1)
	{
		if(jsArgCount(es)<=2)
		{
			jsValue arg0 = jsArg(es, 0);
			if (!jsIsNumber(arg0))
				return jsUndefined();
			LONG_PTR bid = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
			if(presentee->lastBid==bid)
			{
				int val=jsToInt(es, arg0);
				presentee->doScintillaScroll(val);
			}
		}
	}
	return 0;
}



////////////////////////////////////////////////
////// class definition starts here       //////
////////////////////////////////////////////////


extern TCHAR MBPath[MAX_PATH];

APresenterMiniWke::APresenterMiniWke(TCHAR* WKPath, int & error_code, HWND & hBrowser, HWND hwnd) {
	wkeSetWkeDllPath(WKPath);
	error_code = -1;
	if(wkeInitialize()) 
	{
		mWebView = wkeCreateWebWindow(WKE_WINDOW_TYPE_CONTROL, hwnd , 0, 0, 640, 480); 
		if (mWebView)
		{
			lstrcpy(MBPath, WKPath);
			::PathRemoveFileSpec(MBPath);
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
			wkeJsBindFunction("NewDoc", &NewDoc, nullptr, 1);
			wkeJsBindFunction("Scinllo", &ScintillaScroll1, nullptr, 1);
			wkeJsBindFunction("getDarkBG", &getDarkBG, nullptr, 0);
			error_code = 0;
		} else {
			error_code = 101;
		}
	} else {
		error_code = 100;
	}
}

void APresenterMiniWke::Refresh() 
{
	//if (mWebView) wkeReload(mWebView);
	EvaluateJavascript("location.reload()");
}


void APresenterMiniWke::GoBack() {
	if (mWebView)
	wkeGoBack(mWebView);
}

void APresenterMiniWke::GoForward() {
	if (mWebView)
	wkeGoForward(mWebView);
}

void APresenterMiniWke::DestroyWebView(bool exit) {
	if (mWebView)
	wkeDestroyWebView(mWebView);
}

void APresenterMiniWke::EvaluateJavascript(char * JS) {
	if (mWebView)
	wkeRunJS(mWebView, JS);
}

void APresenterMiniWke::ResetZoom() {
	if (mWebView)
	wkeSetZoomFactor(mWebView, 1);
}

void APresenterMiniWke::ZoomOut() {
	float zoom=wkeZoomFactor(mWebView)-mbzd;
	if(zoom<0.25) zoom=0.25;
	if (mWebView)
	wkeSetZoomFactor(mWebView, zoom);
}

void APresenterMiniWke::ZoomIn() {
	float zoom=wkeZoomFactor(mWebView)+mbzd;
	if(zoom>5) zoom=5;
	if (mWebView)
	wkeSetZoomFactor(mWebView, zoom);
}

void APresenterMiniWke::ShowDevTools(TCHAR *res_path) {
	TCHAR tmp[MAX_PATH]{};
	lstrcpy(tmp, MBPath);
	::PathAppend(tmp, TEXT("front_end\\inspector.html"));
	if (mWebView)
	wkeShowDevtools(mWebView, tmp, 0, 0);
}

void APresenterMiniWke::ShowWindow() {
	if (mWebView)
	wkeShowWindow(mWebView, TRUE);
}

void APresenterMiniWke::updateArticle(LONG_PTR bid, int articleType, bool softUpdate, bool update) 
{
	if (softUpdate&&articleType!=1&&presentee->lastBid==bid)
	{
		wkeRunJS(mWebView, "update()"); // wke soft update
		return;
	}
	CHAR page_content[128]{"http://tests/MDT/"};
	if (articleType==1)
	{
		if (softUpdate||update)
		{
			int to = LONGPTR2STR(page_content+17, bid);
			strcpy(page_content+17+to, "/doc.html");
			wkeLoadURL(mWebView, page_content);
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
		wkeLoadURL(mWebView, page_content);
		presentee->lastBid=bid;
		lstrcpy(last_updated, last_actived);
	}
}
