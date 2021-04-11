#pragma once
#ifndef ARTICLEPRESENTER_H
#define ARTICLEPRESENTER_H
#include <windows.h>

#pragma warning(disable:4100)

const char InternalResHead1[] = "http://mdbr/";
const char InternalTESTSResHead1[] = "http://tests/MDT/";

class APresentee;

extern APresentee* presentee;

class ArticlePresenter
{
public:
    virtual void Refresh() = 0;
    virtual void GoBack() = 0;
    virtual void GoForward() = 0;
    virtual void DestroyWebView(bool exit = false) = 0;
    virtual void EvaluateJavascript(char * JS) = 0;
    virtual void ResetZoom() = 0;
    virtual void ZoomOut() = 0;
    virtual void ZoomIn() = 0;
    virtual void ShowDevTools(TCHAR *res_path) = 0;
    virtual void ShowWindow() = 0;
    virtual void updateArticle(LONG_PTR bid, int articleType, bool softUpdate, bool update) = 0;
    virtual void notifyWindowSizeChanged(RECT & rc) { };
};

#endif