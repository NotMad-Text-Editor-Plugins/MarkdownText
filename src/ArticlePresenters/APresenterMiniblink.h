#pragma once
#ifndef APRESENTERMINIBLINK_H
#define APRESENTERMINIBLINK_H
#include "ArticlePresenter.h"
#include "Miniblink/wke.h"
#include "Miniblink/mb.h"


#define mbzd 0.25f

class APresenterMiniWke : public ArticlePresenter
{
public:
	APresenterMiniWke(TCHAR* WKPath, int & error_code, HWND & hBrowser, HWND hwnd);
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
private:
	// miniblink-wke
	wkeWebView mWebView=0;
};

class APresenterMiniblink : public ArticlePresenter
{
public:
	APresenterMiniblink(TCHAR* WKPath, const TCHAR* modulePath, int & error_code, HWND & hBrowser, HWND hwnd);
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
private:
	// miniblink-mb
	mbWebView mWebView=0;
};
#endif