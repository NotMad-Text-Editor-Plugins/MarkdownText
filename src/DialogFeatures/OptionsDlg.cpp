#include "OptionsDlg.h"

#include "ControlEx/UIBeautifulSwitch.h"

#include "MDTextDlg.h"

using namespace DuiLib;

OptionsDlg::OptionsDlg()
{ 
    CControlFactory::GetInstance()->RegistControl(TEXT("CBSwitchUI"), CBSwitchUI::CreateControl);

};

UINT OptionsDlg::GetClassStyle() const 
{
    return UI_CLASSSTYLE_FRAME | CS_DBLCLKS; 
};

void OptionsDlg::OnFinalMessage(HWND /*hWnd*/) { 
    //delete this; 
};

bool OptionsDlg::OnValueChanged(void* param) {
    //TNotifyUI* pMsg = (TNotifyUI*)param;
    //if( pMsg->sType == _T("valuechanged") ) {
    //    m_pm.SetOpacity((static_cast<CSliderUI*>(pMsg->pSender))->GetValue());
    //}
    return true;
}

void OptionsDlg::OnPrepare() 
{
    //CCheckBoxUI *pCheck = static_cast<CCheckBoxUI*>(m_pm.FindControl(_T("chkOption")));
    //pCheck->Activate();
    //pCheck->SetMaxHeight(40);
    //pCheck->SetMaxWidth(60);


    // CBSwitchUI* pSwitch = static_cast<CBSwitchUI*>(m_pm.FindControl(_T("circle_progress")));
    //
    // if( pSwitch ) {
    //     pSwitch->OnNotify += MakeDelegate(this, &CFrameWindowWnd::OnValueChanged);
    // }

    bwpath = static_cast<CComboUI*>(m_pm.FindControl(_T("bwpath")));

    bwedit = static_cast<CControlUI*>(m_pm.FindControl(_T("bwedit")));


    if( bwpath ) {
        auto & lp = _MDText.LibPaths;
        TCHAR tmpPath[MAX_PATH]{0};
        bwpath->itemRightClickable = true;
        bwpath->SetItemTextStyle(DT_VCENTER|DT_SINGLELINE);
        bwpath->RemoveAll();
        for(int i=0,len=lp.size();i<len;i++) {
            auto listItem = new CListLabelElementUI();
            char* data = (char*)(lp[i]?lp[i]->data():NULL);
            if(!data) 
            {
                data = "";
            }
            MultiByteToWideChar(CP_ACP, 0, data, -1, tmpPath, MAX_PATH); // bytes conversion :: data from ini to display
            listItem->SetText(tmpPath);
            listItem->SetToolTip(TEXT("右击选择文件夹"));
            bwpath->Add(listItem);
            if(i==_MDText.LibCefSel) {
                bwedit->SetText(tmpPath);
            }
        }
    }

    //pCheck->SetAttribute(_T("normalimage"), _T("file='image\\switchbutton.png' source='0,0,143,91'"));
    //pCheck->SetAttribute(_T("selectedimage"), _T("file='image\\switchbutton.png' source='0,182,143,273'"));

}


void OptionsDlg::setTweakedPath(char* path)
{
    auto & lp = _MDText.LibPaths;
    auto sz = lp.size();
    bool tweaking = path;
    if(path) 
    {
        if(itemTweakingIndex>=sz) {
            itemTweakingIndex = sz;
            lp.push_back(NULL);
        }
        _MDText.setLibPathAt(itemTweakingIndex, path);
    } else if(itemTweakingIndex>=0&&itemTweakingIndex<sz){
        _MDText.LibCefSel = itemTweakingIndex;
        auto item = lp[itemTweakingIndex];
        path = (char*)(item?item->data():0);
    }
    // upate list ui
    TCHAR tmpPath[MAX_PATH]{0};
    if(path) 
    {
        MultiByteToWideChar(CP_ACP, 0, path, -1, tmpPath, MAX_PATH); // bytes conversion :: data from file picker or converted display to display
        if(bwedit&&itemTweakingIndex==_MDText.LibCefSel) {
            bwedit->SetText(tmpPath);
        }
    }
    if(tweaking) 
    {
        auto item = bwpath->GetItemAt(itemTweakingIndex);
        if(item) item->SetText(tmpPath);
    }
}

static int CALLBACK BrowseForFolderCallBack(HWND hwnd, UINT message, LPARAM lParam, LPARAM lpData) {
    if (message == BFFM_INITIALIZED) {
        OptionsDlg * tDlg = (OptionsDlg * )lpData;
        int sel = tDlg->itemTweakingIndex;
        auto & lp = _MDText.LibPaths;
        if(sel>=0&&sel<lp.size())
        {
            auto lpstr = lp[sel];
            if(lpstr) {
                char* path = (char*)lpstr->data();
                TCHAR tmppath[MAX_PATH];
                MultiByteToWideChar(CP_ACP, 0, path, -1, tmppath, MAX_PATH); // bytes conversion :: data from ini to file picker
                ::SendMessage(hwnd, BFFM_SETSELECTION, 1, (LPARAM)tmppath);
            }
        }
    }
    return 0;
}

string WChar2MByte(const wchar_t* wstr)
{
    char* str;
    int nLen;
    string strRet;

    nLen = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    str = new char[nLen];
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, nLen, NULL, NULL);
    strRet = str;
    delete []str;

    return strRet;
}

void OptionsDlg::Notify(TNotifyUI& msg)
{
    if( msg.sType == _T("windowinit") ) OnPrepare();
    else if( msg.sType == _T("click") ) 
    {
        if( msg.pSender->GetName() == _T("insertimagebtn") ) {
            CRichEditUI* pRich = static_cast<CRichEditUI*>(m_pm.FindControl(_T("testrichedit")));
            if( pRich ) {
                pRich->RemoveAll();
            }
        }
        else if( msg.pSender->GetName() == _T("changeskinbtn") ) {
            if( CPaintManagerUI::GetResourcePath() == CPaintManagerUI::GetInstancePath() )
                CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath() + _T("skin\\FlashRes"));
            else
                CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath());
            CPaintManagerUI::ReloadSkin();
        }
    }
    // notify :: right click on a list item of combobox
    else if( msg.pSender == bwpath ) 
    {
        if( msg.sType == DUI_MSGTYPE_ITEMSELECT ) 
        {
            int index = msg.wParam;
            itemTweakingIndex = index;
            setTweakedPath(NULL);
        }
        else if( msg.sType == DUI_MSGTYPE_ITEMMENU ) 
        {
            itemTweakingIndex = msg.wParam;
            CHAR path[MAX_PATH];
            BROWSEINFOA bi = { 0 };
            bi.ulFlags = BIF_USENEWUI;
            bi.lParam = (LPARAM)this;
            bi.lpfn = BrowseForFolderCallBack;
            bi.lpszTitle = "Pick LibCef folder: ( contains cefclient.dll )";
            bi.hwndOwner = nppData._nppHandle;  // fix window flicker :: GetHWND(); 
            LPITEMIDLIST pidl = SHBrowseForFolderA ( &bi );
            if (pidl != 0 && SHGetPathFromIDListA(pidl, path)) {
                setTweakedPath(path);
            }
        }
    }
    // notify :: return on the edit control
    else if( msg.pSender == bwedit ) {
        if( msg.sType == DUI_MSGTYPE_RETURN ) {
            itemTweakingIndex = _MDText.LibCefSel;
            CHAR path[MAX_PATH*2];
            auto tmpText = bwedit->GetText();
            int len = WideCharToMultiByte(CP_ACP, 0, tmpText
                , tmpText.GetLength(), path, MAX_PATH*2-1, 0, 0); // bytes conversion :: data from display to ini and display
            path[len] = '\0';
            ::MessageBoxA(NULL, path, (""), MB_OK);
            setTweakedPath(path);
        }
    }
}

LRESULT OptionsDlg::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if( uMsg == WM_CREATE ) {
        m_pm.Init(m_hWnd);
        CDialogBuilder builder;

        CControlUI* pRoot = builder.Create(TEXT("test1.xml"), (UINT)0, 0, &m_pm);

        ASSERT(pRoot && "Failed to parse XML");
        m_pm.AttachDialog(pRoot);
        m_pm.AddNotifier(this);

        //m_pm.SetDPI(100);  // Set the new DPI, retrieved from the wParam

        //m_pWndShadow = new CWndShadow;
        //m_pWndShadow->Create(m_hWnd);
        //RECT rcCorner = {3,3,4,4};
        //RECT rcHoleOffset = {0,0,0,0};
        //m_pWndShadow->SetImage(_T("LeftWithFill.png"), rcCorner, rcHoleOffset);

        //DWMNCRENDERINGPOLICY ncrp = DWMNCRP_ENABLED;
        //SetWindowAttribute(m_hWnd, DWMWA_TRANSITIONS_FORCEDISABLED, &ncrp, sizeof(ncrp));

        //DWM_BLURBEHIND bb = {0};
        //bb.dwFlags = DWM_BB_ENABLE;
        //bb.fEnable = true;
        //bb.hRgnBlur = NULL;
        //EnableBlurBehindWindow(m_hWnd, bb);

        //DWM_MARGINS margins = {-1}/*{0,0,0,25}*/;
        //ExtendFrameIntoClientArea(m_hWnd, margins);

        Init();
        return 0;
    }
    else if( uMsg == WM_DESTROY ) {
        //::PostQuitMessage(0L);
    }
    else if( uMsg == WM_NCACTIVATE ) {
        if( !::IsIconic(*this) ) return (wParam == 0) ? TRUE : FALSE;
    }
    LRESULT lRes = 0;
    if( m_pm.MessageHandler(uMsg, wParam, lParam, lRes) ) return lRes;
    return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
}