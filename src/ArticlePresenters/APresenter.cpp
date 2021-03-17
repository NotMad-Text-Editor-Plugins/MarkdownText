#include "APresenter.h"
#include "APresenterMiniblink.h"
#include "APresenterBWidget.h"
#include "APresenterWebView2.h"
#include "MDTextDlg.h"

#include "ArticlePresenter.h"

#define SUCCEED error_code==0

// instantiate a new Webview widget according to the type. 
int APresenter::initWebViewImpl(int kernelType, MarkDownTextDlg * pt) 
{
	ArticlePresenter *newImpl=nullptr, *tmpImpl=nullptr;
	MarkDownTextDlg & presentee = *pt;

	int currentkernelType=-1;

	TCHAR WKPath[MAX_PATH]={0};
	TCHAR WKPath1[MAX_PATH]={0};
	::GetModuleFileName((HINSTANCE)presentee.getModule(), WKPath, MAX_PATH);
	::PathRemoveFileSpec(WKPath);
	int error_code=-1;
	if(PathFileExists(WKPath))
	{ // ThriveEngines !
		bool browser_deferred_creating=false;
		if(presentee.browser_deferred_create_time)
		{
			clock_t time = clock();
			if(time-presentee.browser_deferred_create_time<550)
				browser_deferred_creating=1;
			else
				presentee.browser_deferred_create_time=0;
		}

		/* Webview2 */ 
		if(!browser_deferred_creating && kernelType==WEBVIEW2_TYPE)
		{
			currentkernelType=WEBVIEW2_TYPE;
			tmpImpl = new APresenterWebView2(error_code, presentee.hBrowser, presentee.getHSelf()); 
			if ( SUCCEED )
			{
				newImpl = tmpImpl;
				browser_deferred_creating=1;
				presentee.browser_deferred_create_time = clock();
			}
		}

		/* BrowserWidget */
		bool prefer_bw = kernelType==BROWSERWIDGET_TYPE||kernelType==AUTO_TYPE;
		if(!browser_deferred_creating && prefer_bw)
		{
			currentkernelType=BROWSERWIDGET_TYPE;
			auto libPath = presentee.LibPath;
			if(!libPath)
			{
				lstrcpy(WKPath1, WKPath);
				::PathAppend(WKPath1, L"..\\BrowserWidget\\cefclient.dll");
				libPath = WKPath1;
			}
			tmpImpl = new APresenterBWidget(libPath, error_code, presentee.hBrowser, presentee.getHSelf()); 
			if( SUCCEED ) {
				newImpl = tmpImpl;
				browser_deferred_creating=1;
			}
		}

		/* mb and wke */
		if(!browser_deferred_creating)
		{
			::PathAppend(WKPath, L"miniblink_x64.dll");
			if(PathFileExists(WKPath))
			{
				if(error_code && kernelType!=MINILINK_WKE_TYPE)
				{
					wke_mb|=0x2;
					currentkernelType=MINILINK_TYPE; // mb
					tmpImpl = new APresenterMiniblink(WKPath, WKPath1, error_code, presentee.hBrowser, presentee.getHSelf()); 
				}
				if(error_code && kernelType<=MINILINK_TYPE)
				{
					wke_mb|=0x1;
					currentkernelType=MINILINK_WKE_TYPE; // wke
					tmpImpl = new APresenterMiniWke(WKPath, error_code, presentee.hBrowser, presentee.getHSelf());
				}
			}
			else if(presentee.RequestedSwitch)
			{
				::MessageBox(NULL, L"miniblink_x64.dll not found. ", TEXT(""), MB_OK);
			}
			if( SUCCEED ) {
				newImpl = tmpImpl;
				presentee.mWebView0 = tmpImpl;
				presentee.RefreshWebview();
				tmpImpl->ShowWindow();
			} 
		}
	}

	if( SUCCEED )
	{
		presentee.currentkernelType = currentkernelType;
		presentee.mWebView0 = newImpl;
	}

	return error_code;
}