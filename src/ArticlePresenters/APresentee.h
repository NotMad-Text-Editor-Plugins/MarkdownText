#pragma once
#ifndef APRESENTEE_H
#define APRESENTEE_H

#include <stdafx.h>
#include <scintilla.h>
#include <time.h>
#include <vector>

class ArticlePresenter;

struct DefferedLoadingData
{
	LONG_PTR bid;
	int articleType;
};

static DefferedLoadingData* defferedLoad;

class APresentee
{
public:
	// 0=MD; 1=HTML; 2=ASCII
	int RendererTypeIdx=0;

	int currentkernelType=0; // 0_wke 1_mb 2_bw 3_WV2

	ArticlePresenter* mWebView0;
	HWND hBrowser=nullptr;

	LONG_PTR lastBid=0;

	int LibCefSel;
	int LibWkeSel;
	int LibMbSel;
	std::vector<std::string*> LibPaths;
	//std::vector<std::string*> WkePaths;
	std::vector<std::string*> MbPaths;

	bool RequestedSwitch=false;
	clock_t browser_deferred_create_time=0;

	virtual HWND getHWND()=0;

	virtual HINSTANCE getModule()=0;

	virtual void doScintillaScroll(int ln)=0;

	virtual CHAR* loadSourceAsset(uptr_t bid, const char* pathA, DWORD & dataLen, bool * shouldDelete=NULL)=0;

	virtual CHAR* loadPluginAsset(const char* path, DWORD & dataLen)=0;

	virtual CHAR* GetDocTex(size_t & docLength, LONG_PTR bid, bool * shouldDelete)=0;

	//virtual bool checkRenderMarkdown()=0;

	//virtual bool checkRenderHtml()=0;

	virtual void AppendPageResidue(char* appendSt)=0;

	virtual void syncWebToline(bool force=false)=0;

	virtual void RefreshWebview(int source=0)=0;

	virtual std::wstring GetLocalWText(char* name, const TCHAR* defVal)=0;

	virtual void NewDoc(TCHAR* name)=0;

	virtual void NewDoc(CHAR* name)=0;

	virtual int getDarkBG()=0;
};

#endif