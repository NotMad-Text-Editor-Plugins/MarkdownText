#pragma once
#ifndef APRESENTERWEBVIEW2_H
#define APRESENTERWEBVIEW2_H
#include "ArticlePresenter.h"

class APresenterWebView2 : public ArticlePresenter
{
public:
	APresenterWebView2(int & error_code, HWND & hBrowser, HWND hwnd);
	void Refresh() override;
	void GoBack() override;
	void GoForward() override;
	void DestroyWebView(bool exit = false) override;
	void EvaluateJavascript(char * JS) override;
	void ResetZoom() override;
	void ZoomOut() override;
	void ZoomIn() override;
	void ShowDevTools(TCHAR *res_path) override;
	void ShowWindow() override;
	void updateArticle(LONG_PTR bid, int articleType, bool softUpdate, bool update) override;
	void notifyWindowSizeChanged(RECT & rc) override;
private:
	// WebView2
};
#endif