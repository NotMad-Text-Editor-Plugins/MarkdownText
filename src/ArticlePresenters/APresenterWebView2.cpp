#include "APresenterWebView2.h"
#include "PluginDefinition.h"
#include "APresentee.h"
#include "SU.h"
#include <shlwapi.h>

#include "WarningDlg.hpp"

#include "CheckFailure.h"

using namespace Microsoft::WRL;

wil::com_ptr<ICoreWebView2> mWebView;
wil::com_ptr<ICoreWebView2Environment> m_webViewEnvironment;
wil::com_ptr<ICoreWebView2Controller> webviewController;

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

extern CHAR* _dataPath;


LPWSTR GetLastErrorAsString()  
{  
	HLOCAL LocalAddress=NULL;  
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,GetLastError(),0,(PTSTR)&LocalAddress,0,NULL);  
	return (LPWSTR)LocalAddress;  
}

APresenterWebView2::APresenterWebView2(int & error_code, HWND & hBrowser, HWND hParent) 
{
	error_code=1;
	auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
	regWndClassWin(L"WINWND", CS_HREDRAW | CS_VREDRAW);
	HWND hWnd = ::CreateWindowEx(0 , L"WINWND" , NULL
		, WS_CHILD , 0 , 0 , 840 , 680 , hParent , NULL , ::GetModuleHandle(NULL), NULL);
	hBrowser=hWnd;
	::ShowWindow(hWnd, 1);
	//LogIs(2, ::GetCommandLine());
	// todo, cannot set path here.
	//TEXT("C:\\Program Files (x86)\\Microsoft\\Edge Beta\\Application")
	//	,TEXT("D:\\Code\\FigureOut\\Textrument\\PowerEditor\\bin64\\Textrument.exe.WebView2")
	//
	//options->put_AdditionalBrowserArguments(L"--mute-audio"); 
	//options->put_AdditionalBrowserArguments(L"--disk-cache-size=0"); 

	//options = new CoreWebView2EnvironmentOptions("–incognito ");

	TCHAR* dataPath = NULL; // L"C:\\temp\\"
	if (_dataPath && ::PathIsDirectoryA(_dataPath))
	{
		dataPath = new TCHAR[MAX_PATH];
		MultiByteToWideChar(CP_ACP, 0, _dataPath, -1, dataPath, MAX_PATH-1);
	}

	//options->put_AdditionalBrowserArguments();

	HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
		NULL , dataPath, options.Get(),
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>
		([this, hBrowser, hParent](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
		
		//LPWSTR versionInfo; env->get_BrowserVersionString(&versionInfo);
		//LogIs(2, L"Some BUG happened, ???. %ld  %d %s", result, (int)env, versionInfo);
		
		m_webViewEnvironment=env;
		env->CreateCoreWebView2Controller(hBrowser, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>
			([this, hParent](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
			if (controller != nullptr) 
			{
				webviewController = controller;
				webviewController->get_CoreWebView2(&mWebView);
			}

			if (!mWebView)
			{
				LogIs(2, L"\"%s\"\n\nSome BUG happened, webview2 failed to initialzie. ret=0x%lx controller=%d"
					, GetLastError()?GetLastErrorAsString():L""
					, result, (int)controller
				);
				return -1;
			}
			
			ICoreWebView2Settings* Settings;
			mWebView->get_Settings(&Settings);
			Settings->put_IsScriptEnabled(TRUE);
			Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
			Settings->put_IsWebMessageEnabled(TRUE);

			SendMessage(hParent, WM_SIZE, 0, 0);

			static EventRegistrationToken pageEventRegistrationToken = {};
			mWebView->AddWebResourceRequestedFilter(L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
			CHECK_FAILURE(mWebView->add_WebResourceRequested(
				Callback<ICoreWebView2WebResourceRequestedEventHandler>([this](
					ICoreWebView2* sender,
					ICoreWebView2WebResourceRequestedEventArgs* args) {
				COREWEBVIEW2_WEB_RESOURCE_CONTEXT resourceContext;
				CHECK_FAILURE(args->get_ResourceContext(&resourceContext));
				wil::com_ptr<ICoreWebView2WebResourceRequest> req;
				args->get_Request(&req);
				wil::unique_cotaskmem_string navigationTargetUri;
				req->get_Uri(&navigationTargetUri);
				TCHAR* uriTarget=(TCHAR*)navigationTargetUri.get();

				if(wcsncmp(uriTarget, TEXT("http://tests/MDT/"), 17)==0)
				{ // constuct page here
					TCHAR* end=uriTarget+wcslen(uriTarget);
					uriTarget+=17;
					TCHAR* PageId=uriTarget;
					TCHAR* page_type=NULL;
					for(;uriTarget<end;uriTarget++)
					{
						if(*uriTarget=='/') 
						{
							*uriTarget++='\0';
							page_type=uriTarget;
							break;
						}
					}
					LONG_PTR bid=0;
					STR2LONGPTR(PageId, bid);
					if(page_type && bid)
					{
						if (!lstrcmp(page_type, TEXT("index.html")))
						{
							// text2html index page.
							CHAR* page_content = new CHAR[1024];
							//post message
							//strcpy(page_content, "<!doctype html><meta charset=\"utf-8\"><script>var w=window,wv=w.chrome.webview;wv.addEventListener(\'message\', event => APMD(event.data));w.update=function(){wv.postMessage('");
							//auto nxt_st=page_content+strlen(page_content);
							//nxt_st+=LONGPTR2STR(nxt_st, bid);
							//strcpy(nxt_st, "');}</script><body><script src=\"http://mdbr/");

							//xhr http
							strcpy(page_content, "<!doctype html><meta charset=\"utf-8\"><script src=\"http://mdbr/ui.js\"></script><script>var w=window;w.update=function(){var R=new XMLHttpRequest();R.open('POST','");
							auto nxt_st=page_content+strlen(page_content);
							//nxt_st+=LONGPTR2STR(nxt_st, bid);
							strcpy(nxt_st, "text',true);R.onreadystatechange=function(){APMD(R.responseText)};R.send()}</script><body><script src=\"http://mdbr/");

							nxt_st+=strlen(nxt_st);
							presentee->AppendPageResidue(nxt_st); // 加载WV2

							wil::com_ptr<ICoreWebView2WebResourceResponse> response;
							wil::com_ptr<IStream> stream = SHCreateMemStream((const BYTE*)page_content, strlen(page_content));

							CHECK_FAILURE(m_webViewEnvironment->CreateWebResourceResponse(stream.get(), 200, L"OK", L"charset=utf-8", &response));

							wil::com_ptr<ICoreWebView2HttpResponseHeaders> headers;
							CHECK_FAILURE(response->get_Headers(&headers));
							headers->AppendHeader(L"Content-Type", L"text/html; charset=utf-8");

							CHECK_FAILURE(args->put_Response(response.get()));
						}
						else if (!lstrcmp(page_type, TEXT("text"))||!lstrcmp(page_type, TEXT("doc.html")))
						{
							size_t docLength=0;
							auto str = presentee->GetDocTex(docLength, bid, 0);
							if(docLength)
							{
								wil::com_ptr<ICoreWebView2WebResourceResponse> response;
								wil::com_ptr<IStream> stream = SHCreateMemStream((const BYTE*)str, docLength);
								CHECK_FAILURE(m_webViewEnvironment->CreateWebResourceResponse(stream.get(), 200, L"OK", L"charset=utf-8", &response));
								wil::com_ptr<ICoreWebView2HttpResponseHeaders> headers;
								CHECK_FAILURE(response->get_Headers(&headers));
								//headers->AppendHeader(L"Content-Type", L"text/*; charset=utf-8");
								CHECK_FAILURE(args->put_Response(response.get()));
							}
						}
						else
						{
							// load assets in the document folder.
							char decodedUrl[MAX_PATH];
							UrlDecode(decodedUrl, page_type);
							DWORD dataLen;
							auto data = presentee->loadSourceAsset(bid, decodedUrl, dataLen);
							if(data) {
								wil::com_ptr<ICoreWebView2WebResourceResponse> response;
								wil::com_ptr<IStream> stream = SHCreateMemStream((const BYTE*)data, dataLen);
								CHECK_FAILURE(m_webViewEnvironment->CreateWebResourceResponse(stream.get(), 200, L"OK", L"charset=utf-8", &response));
								wil::com_ptr<ICoreWebView2HttpResponseHeaders> headers;
								CHECK_FAILURE(response->get_Headers(&headers));
								//MIME=text/css workaround
								headers->AppendHeader(L"Content-Type", L"text/css; charset=utf-8");
								CHECK_FAILURE(args->put_Response(response.get()));
							}
						}
					}
					return S_OK;
				}
				else if(wcsncmp(uriTarget, TEXT("http://mdbr/"), 12)==0)
				{ // build resource response
					auto path = uriTarget+12;
					if(path)
					{
						// COREWEBVIEW2_WEB_RESOURCE_CONTEXT
						if(wcsstr(path, L"..")) // security check
						{
							return E_INVALIDARG;
						}
						else
						{
							char decodedUrl[MAX_PATH];
							UrlDecode(decodedUrl, path);


							wil::com_ptr<ICoreWebView2HttpRequestHeaders> headers;
							req->get_Headers(&headers);

							//BOOL val=false;
							//headers->Contains(L"Content-Type", &val);
							//if (val)
							//{
							//	wil::unique_cotaskmem_string header;
							//	headers->GetHeader(L"Content-Type", &header);
							//	LogIs(1, header.get());
							//}

							DWORD dataLen;
							auto buffer = presentee->loadPluginAsset(decodedUrl, dataLen);

							if(buffer)
							{
								wil::com_ptr<ICoreWebView2WebResourceResponse> response;
								wil::com_ptr<IStream> stream = SHCreateMemStream((const BYTE*)buffer, dataLen);
								CHECK_FAILURE(m_webViewEnvironment->CreateWebResourceResponse(stream.get(), 200, L"OK", L"charset=utf-8", &response));
								wil::com_ptr<ICoreWebView2HttpResponseHeaders> headers;
								CHECK_FAILURE(response->get_Headers(&headers));
								// MIME=text/css workaround
								headers->AppendHeader(L"Content-Type", L"text/css; charset=utf-8");
								headers->AppendHeader(L"Access-Control-Allow-Origin", L"*");
								//headers->AppendHeader(L"Content-Charset", L"utf-8");
								//headers->AppendHeader(L"charset", L"utf-8");
								CHECK_FAILURE(args->put_Response(response.get()));
							}
						}
					}
				}
				return E_INVALIDARG;
			}).Get(), &pageEventRegistrationToken));

			EventRegistrationToken token;
			if(0)
				mWebView->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
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
					auto str = presentee->GetDocTex(docLength, bid, 0);
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

			mWebView->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
				[](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs * args) -> HRESULT {
				PWSTR message;
				args->TryGetWebMessageAsString(&message);
				if(wcsncmp(message, L"scinllo", 7)==0)
				{
					auto number = message+7;
					bool force=number[0]==L'_';
					if(force)
						number++;
					if(GetUIBoolReverse(0) && GetUIBoolReverse(2) || force)
					{
						presentee->doScintillaScroll(_wtoi(number));
					}
				}
				if(wcsncmp(message, L"getDarkBG", 9)==0)
				{
					//LogIs(2, "asd");
					CHECK_FAILURE(sender->PostWebMessageAsString(presentee->getDarkBG()?L"123":L""));
				}
				CoTaskMemFree(message);
				return S_OK;
			}).Get(), &token);

			if(0)
				mWebView->AddScriptToExecuteOnDocumentCreated(
					L"window.chrome.webview.addEventListener(\'message\', event => alert(event.data));" \
					L"window.chrome.webview.postMessage(window.document.URL);",
					nullptr);
			//mWebView->Navigate(L"https://www.bing.com/");
			//mWebView->Navigate(L"http://tests/MDT/1.html");

			presentee->RefreshWebview();
			return S_OK;
		}).Get());
		return S_OK;
	}).Get());
	//if(true) return;
	if (dataPath)
	{
		delete[] dataPath;
	}

	if (SUCCEEDED(hr))
	{
		error_code = 0;
	}
	else {
		DestroyWindow(hBrowser);
		if (presentee->RequestedSwitch)
		{
			//::MessageBox(hParent, TEXT("Runtime Not Found!"),, L"WebView2 not found. " MB_OK);
			presentee->RequestedSwitch = false;
			if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
			{
				WarnDlg* wdlg = new WarnDlg(TEXT("wv2.xml"));
				wdlg->Create(nppData._nppHandle
					, presentee->GetLocalWText("no_rt", TEXT("Runtime Not Found !")).c_str()
					, WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION , WS_EX_DLGMODALFRAME );
				wdlg->CenterWindow();
				wdlg->ShowModal(nppData._nppHandle);
			}
			else {
				LogIs(2, "Unkonw Error!!! %ld ", hr);
			}
		}
	}
}

void APresenterWebView2::Refresh() 
{
	if (mWebView)
	mWebView->Reload();
}

void APresenterWebView2::GoBack() 
{
	//if(mWebView->CanGoBack())
	if (mWebView)
	mWebView->GoBack();
}

void APresenterWebView2::GoForward() 
{
	//if(mWebView->CanGoForward())
	if (mWebView)
	mWebView->GoForward();
}

void APresenterWebView2::DestroyWebView(bool exit) 
{
	if(webviewController) {
		webviewController->Close();
		m_webViewEnvironment.reset();
		webviewController.reset();
		mWebView.reset();
	}
}

void APresenterWebView2::EvaluateJavascript(char * JS) 
{
	if (!mWebView) return;
	TCHAR * LJS = new TCHAR[strlen (JS) + 1];
	MultiByteToWideChar (CP_ACP, 0, JS, strlen (JS) + 1, LJS, 256);
	mWebView->ExecuteScript(LJS, 0);
	delete[] LJS;
}

void APresenterWebView2::ResetZoom() 
{
	if(webviewController)
	webviewController->put_ZoomFactor(1);
}

void APresenterWebView2::ZoomOut() 
{
	if(!webviewController) return;
	double zoom;
	webviewController->get_ZoomFactor(&zoom);
	webviewController->put_ZoomFactor(zoom-0.15);
}

void APresenterWebView2::ZoomIn() 
{
	if(!webviewController) return;
	double zoom;
	webviewController->get_ZoomFactor(&zoom);
	webviewController->put_ZoomFactor(zoom+0.15);
}

void APresenterWebView2::ShowDevTools(TCHAR *res_path) {
	if (mWebView)
	mWebView->OpenDevToolsWindow();
}

void APresenterWebView2::ShowWindow() 
{
}

void APresenterWebView2::updateArticle(LONG_PTR bid, int articleType, bool softUpdate, bool update) 
{
	if(!mWebView) return;
	TCHAR* page_id = new TCHAR[64], * new_st;  // Webview2 需要虚拟网址。 传文件名，只传ID吧。 http://tests/MDT/{bid}/index.html
	lstrcpy(page_id, TEXT("http://tests/MDT/"));
	new_st=page_id+17;
	new_st+=LONGPTR2STR(new_st, bid);
	lstrcpy(new_st, TEXT("/index.html"));

	if(articleType==1) {
		if(softUpdate||update) {
			lstrcpy(new_st, TEXT("/doc.html"));
			mWebView->Navigate(page_id);
			if(update) {
				presentee->lastBid=bid;
				lstrcpy(last_updated, last_actived);
			}
		}
		return;
	}

	wil::unique_cotaskmem_string uri;
	mWebView->get_Source(&uri);
	if(softUpdate && STRSTARTWITH(uri.get(), page_id))
	{
		mWebView->ExecuteScript(L"update()", 0); // WV2 soft update
	}
	else if(update)
	{
		mWebView->Navigate(page_id);
		presentee->lastBid=bid;
		lstrcpy(last_updated, last_actived);
	}
}

void APresenterWebView2::notifyWindowSizeChanged(RECT & rc) {
	if(webviewController) {
		webviewController->put_Bounds(rc);
	}
}
