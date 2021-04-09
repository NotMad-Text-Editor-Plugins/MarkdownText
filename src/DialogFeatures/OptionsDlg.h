#pragma once
#ifndef _OPTIONS_DLG_H_
#define _OPTIONS_DLG_H_

#include "UIlib.h"
using namespace DuiLib;

class  ComboeditContext;

class OptionsDlg : public CWindowWnd, public INotifyUI
{
public:
    OptionsDlg();
    LPCTSTR GetWindowClassName() const { return _T("UIMainFrame"); };
    UINT GetClassStyle() const;
    void OnFinalMessage(HWND /*hWnd*/);

    void Init() { }

    bool OnValueChanged(void* param);

    void OnPrepare();

    void Notify(TNotifyUI& msg);

    void setTweakedPath(ComboeditContext* ctx, char* path);

    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void HandleMessage(int type, TNotifyUI* msg);

    bool FindLanguages();
public:
    CPaintManagerUI m_pm;

    CComboUI*   bwpath;
    CControlUI* bwedit;
    CComboUI*   mbpath;
    CControlUI* mbedit;
    CComboUI*   wkpath;
    CControlUI* wkedit;

    CComboUI*   rndType;

    CComboUI*   rdnType;
    CComboUI*   language;

    int itemTweakingIndex;
    ComboeditContext* itemTweakingCtx;

    bool languages_scanned=false;

    std::vector<wstring> languages;
};




#endif