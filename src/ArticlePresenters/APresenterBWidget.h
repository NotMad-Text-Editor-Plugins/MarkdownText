#pragma once
#ifndef APRESENTERBWIDGET_H
#define APRESENTERBWIDGET_H
#include "ArticlePresenter.h"
#include <BrowserUI.h>


class APresenterBWidget : public ArticlePresenter
{
public:
	APresenterBWidget(TCHAR* WKPath, int & error_code, HWND & hBrowser, HWND hwnd);
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
	void updateArticle(LONG_PTR bid, int articleType, bool softUpdate, bool update)  override;
//private:
	// Libcef
	bwWebView mWebView=0;
};

#endif