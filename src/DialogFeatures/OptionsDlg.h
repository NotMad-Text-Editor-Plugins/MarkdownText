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
public:
    CPaintManagerUI m_pm;

    CComboUI*   bwpath;
    CControlUI* bwedit;
    CComboUI*   mbpath;
    CControlUI* mbedit;
    CComboUI*   wkpath;
    CControlUI* wkedit;

    int itemTweakingIndex;
    ComboeditContext* itemTweakingCtx;
};




#endif