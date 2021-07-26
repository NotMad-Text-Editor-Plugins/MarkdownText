/* Copyright 2021 KnIfER
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
// Use miniblink, libcef and webview2 to preview HTML, Markdown and Asciidoc documents.
#pragma once
#ifndef _MDTEXT_DLG_H_
#define _MDTEXT_DLG_H_
#include "DockingDlgInterface.h"
#include "PluginDefinition.h"
#include "resource.h"
#include "menuCmdID.h"
#include "ToolbarPanel.h"
#include "APresenter.h"

#include <set>

class ArticlePresenter;

class WarnDlg;

#define SELF_REFRESH WM_USER+9

using namespace std;

const TCHAR sectionName[] = TEXT("Setting");
const TCHAR strMarkColor[] = TEXT("MarkColor");
const TCHAR strSaveColor[] = TEXT("SaveColor");

extern bool legacy ;

extern HWND curScintilla;

//#define TB_ENABLEBUTTON         (WM_USER + 1)

struct ReadExtContext{
	char* key;
	const char* defVal;
};

__declspec(selectany) int hToolsStartId = 0;
__declspec(selectany) int hToolsSz = 2;

class MarkDownTextDlg : public DockingDlgInterface, public APresentee
{
public :
	MarkDownTextDlg() : DockingDlgInterface(IDD_MARKDOWNTEXT) {};

    virtual void display(bool toShow = true); 

	HINSTANCE getModule(){ return (HINSTANCE)g_hModule; }

	virtual void setClosed(bool toClose);

	// Refresh the webview. |source| 0=switch docs;1=edit texts;2=switch engines;
	void RefreshWebview(int source=0);

	void refreshDarkMode();

	void refreshDlg(bool updateList, bool fromEditor);

	HWND getHWND() {
		return _hSelf;
	};

	void setParent(HWND parent2set){
		_hParent = parent2set;
	};
	void saveParameters();
	void readParameters();
	void  readExtensions(int channel, string * ret);
	void destroyWebViews(bool exit=false);
	void switchWebViewByIndex(int id);

	// empty=use internal md renderer;
	char MDRoutine[MAX_PATH_HALF]={0};
	//char HTMLRoutine[MAX_PATH_HALF]={0};
	char ADRoutine[MAX_PATH_HALF]={0};
	// 0=MD; 1=HTML; 2=ASCII
	char* RendererNames[3]={MDRoutine, "", ADRoutine};

	ToolBar toolBar;

	void doScintillaScroll(int ln);

	CHAR* loadSourceAsset(uptr_t bid, const char* pathA, DWORD & dataLen, bool * shouldDelete=NULL);

	CHAR* loadPluginAsset(const char* path, DWORD & dataLen);

	CHAR* GetDocTex(size_t & docLength, LONG_PTR bid, bool * shouldDelete);

	std::string* setLibPathAt(std::vector<std::string*> & paths, int idx, char* newpath, char * key);
protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	HMENU hMenuEngines=0;

	HMENU hMenuZoom=0;

	bool bAutoSwitchEngines=0;

	void releaseEnginesMenu();

	std::vector<FuncItem> ZOOMER;
	std::vector<FuncItem> EngineSwicther;
	std::vector<wstring> MDEngines;
	HMENU hMenuLocate=0;
	std::vector<FuncItem> LocateScroll;

	int requestedInvalidSwitch=-1;
	bool MDEngineScanned;

	bool FindMarkdownEngines(TCHAR* path);
	void readLibPaths(int & max, std::vector<std::string*> & LibPaths, char* key, int & sel, char* selkey);
	
	const char* rnd_res;

	WarnDlg* installGuide = NULL;
	bool isShowGuidePredateArticle = false;

	std::map<LONG_PTR, int> prefRndTyps;
public :
	APresenter presenter;
	int kernelType=-1; // -1_auto 0_wke 1_mb 2_bw 3_WV2
	int skFlags=0;
	bool hasChanged;
	ToolbarPanel ListBoxPanel;
	//Window ListBoxWrap;
	int getToolbarCommand(POINT &pointer);
	void OnToolBarRequestToolTip( LPNMHDR nmhdr );
	void OnToolBarCommand( UINT Cmd, char source=0, POINT* pt=nullptr);
	void AppendPageResidue(char* appendSt);
	void syncWebToline(bool force=false);
	int lastSyncLn=0;

	bool autoSwitchOnStart;
	bool alwaysOffOnStart;
	int maxPathHistory;
	int maxPathHistory1;
	int maxPathHistory2;

	void SwitchEngines(int idx);

	void GlobalOnPvMnChecked(HMENU hMenu, int idx);

	void setLanguageName(wstring & name, bool init=false);

	wstring getLanguageName() {
		return currentLanguageFile;
	};

	bool localeSet;

	wstring currentLanguageFile = L"zh_中文.ini";

	std::map<std::string, std::string> & getLocaliseMap();

	string * getLocalString(char* name);

	void localeTextToBuffer(TCHAR* buffer, int cchBuffer, char* name, TCHAR* defVal);

	void destroyDynamicMenus();

	std::vector<std::string> markdown_ext;

	std::vector<std::string> asciidoc_ext;

	std::vector<std::string> html_ext;

	std::vector<std::string>* all_exts[3]{&markdown_ext, &html_ext, &asciidoc_ext};

	void checkAutoRun();

	ReadExtContext* extCtx = NULL;

	bool bRunRequested = false;

	void displayInstallGuide();

	void CheckChaninedUpdate(LONG_PTR BID);

	std::wstring GetLocalWText(char* name, const TCHAR* defVal);

	std::set<LONG_PTR> buffersMap;

	std::set<LONG_PTR> chainedBuffersMap;
	std::set<std::wstring> chainedBuffersPath;

	int defaultRenderer;
	int lastPickedRenderer;
	HMENU hToolsMenu=0;

	void TurnHtmlToMarkdown(int from);
	void create();
	void LanguageToMarkdown();
	void RunToolsCommand(int id);	
	void NewDoc(TCHAR* name) override;
	void NewDoc(CHAR* name) override;
	int getDarkBG() override;

	bool checkFileExt(int type);
};

#endif //LNHISTORY_DLG_H
