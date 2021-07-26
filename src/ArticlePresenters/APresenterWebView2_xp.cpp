#include "APresenterWebView2.h"
#include "PluginDefinition.h"
#include "APresentee.h"
#include "SU.h"

// XP support dropped!

APresenterWebView2::APresenterWebView2(int & error_code, HWND & hBrowser, HWND hParent) 
{
	error_code=1;
}

void APresenterWebView2::Refresh() 
{
}

void APresenterWebView2::GoBack() 
{
}

void APresenterWebView2::GoForward() 
{
}

void APresenterWebView2::DestroyWebView(bool exit) 
{
}

void APresenterWebView2::EvaluateJavascript(char * JS) 
{
}

void APresenterWebView2::ResetZoom() 
{
}

void APresenterWebView2::ZoomOut() 
{
}

void APresenterWebView2::ZoomIn() 
{
}

void APresenterWebView2::ShowDevTools(TCHAR *res_path) {
}

void APresenterWebView2::ShowWindow() 
{
}

void APresenterWebView2::updateArticle(LONG_PTR bid, int articleType, bool softUpdate, bool update) 
{
}

void APresenterWebView2::notifyWindowSizeChanged(RECT & rc) {
}
