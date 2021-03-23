#include "OptionsDlg.h"

#include "ControlEx/UIBeautifulSwitch.h"

#include "ControlEx/DuiLibTranslator.h"

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


struct ComboeditContext
{
    CComboUI*   combo;
    CControlUI* edit;
    int * sel;
    std::vector<std::string*> * paths;
    CHAR* pick_tips;
    CHAR* key;
};

void drawComboEditUI(CComboUI* combo, CControlUI* edit
    , int & sel, std::vector<std::string*> & paths
    , char * tips, char * key
)
{
    if( combo ) {
        combo->SetTag((LONG_PTR)(new ComboeditContext{combo, edit, &sel, &paths, tips, key}));
        if(edit) edit->SetTag((LONG_PTR)combo);
        auto & lp = paths;
        TCHAR tmpPath[MAX_PATH]{0};
        combo->itemRightClickable = true;
        combo->SetItemTextStyle(DT_VCENTER|DT_SINGLELINE);
        combo->RemoveAll();
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
            combo->Add(listItem);
            if(i==sel && edit) {
                edit->SetText(tmpPath);
            }
        }
    }
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

    bwpath = static_cast<CComboUI*>(  m_pm.FindControl(_T("bwpath")));
    bwedit = static_cast<CControlUI*>(m_pm.FindControl(_T("bwedit")));
    mbpath = static_cast<CComboUI*>(  m_pm.FindControl(_T("mbpath")));
    mbedit = static_cast<CControlUI*>(m_pm.FindControl(_T("mbedit")));
    wkpath = static_cast<CComboUI*>(  m_pm.FindControl(_T("wkpath")));
    wkedit = static_cast<CControlUI*>(m_pm.FindControl(_T("wkedit")));

    rdnType = static_cast<CComboUI*>(  m_pm.FindControl(_T("rndType")));
    language = static_cast<CComboUI*>(  m_pm.FindControl(_T("language")));

    drawComboEditUI(bwpath, bwedit, _MDText.LibCefSel, _MDText.LibPaths, "Pick LibCef folder: ( contains cefclient.dll )", "LibPath%d");
    drawComboEditUI(mbpath, mbedit, _MDText.LibMbSel, _MDText.MbPaths, "Pick Miniblink folder: ( contains miniblink_x64.dll )", "MbPath%d");
    drawComboEditUI(wkpath, wkedit, _MDText.LibWkeSel, _MDText.WkePaths, "Pick WKE Path: ( wke.dll )", "WkePath%d");

    HandleMessage(2, NULL);
    //pCheck->SetAttribute(_T("normalimage"), _T("file='image\\switchbutton.png' source='0,0,143,91'"));
    //pCheck->SetAttribute(_T("selectedimage"), _T("file='image\\switchbutton.png' source='0,182,143,273'"));

}


void OptionsDlg::setTweakedPath(ComboeditContext* ctx, char* path)
{
    auto & lp = *ctx->paths;
    auto sz = lp.size();
    bool tweaking = path;
    if(path) 
    {
        if(itemTweakingIndex>=sz) {
            itemTweakingIndex = sz;
            lp.push_back(NULL);
        }
        _MDText.setLibPathAt(lp, itemTweakingIndex, path, ctx->key);
    } else if(itemTweakingIndex>=0&&itemTweakingIndex<sz){
        *ctx->sel = itemTweakingIndex;
        auto item = lp[itemTweakingIndex];
        path = (char*)(item?item->data():0);
    }
    // upate list ui
    TCHAR tmpPath[MAX_PATH]{0};
    if(path) 
    {
        MultiByteToWideChar(CP_ACP, 0, path, -1, tmpPath, MAX_PATH); // bytes conversion :: data from file picker or converted display to display
        if(ctx->edit&&itemTweakingIndex==*ctx->sel) {
            ctx->edit->SetText(tmpPath);
        }
    }
    if(tweaking) 
    {
        auto item = ctx->combo->GetItemAt(itemTweakingIndex);
        if(item) item->SetText(tmpPath);
    }
}

static int CALLBACK BrowseForFolderCallBack(HWND hwnd, UINT message, LPARAM lParam, LPARAM lpData) {
    if (message == BFFM_INITIALIZED) {
        // navigate to one of the paths stored in the ini file
        OptionsDlg * tDlg = (OptionsDlg * )lpData;
        if(tDlg->itemTweakingCtx)
        {
            int sel = tDlg->itemTweakingIndex;
            auto & lp = *tDlg->itemTweakingCtx->paths;
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
    }
    return 0;
}

bool OptionsDlg::FindLanguages()
{
    languages_scanned = true;
    TCHAR path[MAX_PATH];
    GetModuleFileName((HMODULE)g_hModule, path, MAX_PATH);
    PathRemoveFileSpec(path);

    PathAppend(path, TEXT("localization"));

    languages.clear();
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA ffd;
    wstring spec;
    auto len=lstrlen(path);

    PathAppend(path, TEXT("//*.ini"));

    hFind = FindFirstFile(path, &ffd);
    if (hFind == INVALID_HANDLE_VALUE)  {
        return false;
    }

    do {
        if (wcscmp(ffd.cFileName, L".") != 0 &&  wcscmp(ffd.cFileName, L"..") != 0) {
            if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0) {
                languages.push_back(ffd.cFileName);
            }
        }
    } while (FindNextFile(hFind, &ffd) != 0);

    if (GetLastError() != ERROR_NO_MORE_FILES) {
        FindClose(hFind);
        return false;
    }

    FindClose(hFind);
    hFind = INVALID_HANDLE_VALUE;

    return true;
}

void OptionsDlg::HandleMessage(int type, TNotifyUI* msg)
{
    CHAR path[MAX_PATH*2];
    if(type==0)
    {
        ComboeditContext* ctx = (ComboeditContext*)msg->pSender->GetTag();
        if( itemTweakingCtx = ctx )
        if( msg->sType == DUI_MSGTYPE_ITEMSELECT ) 
        {
            // notify :: select a list item of combobox
            itemTweakingIndex = msg->wParam;
            setTweakedPath(ctx, NULL);
        }
        else if( msg->sType == DUI_MSGTYPE_ITEMMENU ) 
        {
            // notify :: right click on a list item of combobox
            itemTweakingIndex = msg->wParam;
            BROWSEINFOA bi = { 0 };
            bi.ulFlags = BIF_USENEWUI;
            bi.lParam = (LPARAM)this;
            bi.lpfn = BrowseForFolderCallBack;
            bi.lpszTitle = ctx->pick_tips;
            bi.hwndOwner = nppData._nppHandle;  // fix window flicker :: GetHWND(); 
            LPITEMIDLIST pidl = SHBrowseForFolderA ( &bi );
            if (pidl != 0 && SHGetPathFromIDListA(pidl, path)) {
                setTweakedPath(ctx, path);
            }
        }
    } 
    else if(type==1)
    {
        CControlUI* tag = (CControlUI*)msg->pSender->GetTag();
        ComboeditContext* ctx = (ComboeditContext*)tag->GetTag();
        if( itemTweakingCtx = ctx )
        if( msg->sType == DUI_MSGTYPE_RETURN ) {
            itemTweakingIndex = *itemTweakingCtx->sel;
            auto tmpText = msg->pSender->GetText();
            int len = WideCharToMultiByte(CP_ACP, 0, tmpText
                , tmpText.GetLength(), path, MAX_PATH*2-1, 0, 0); // bytes conversion :: data from display to ini and display
            path[len] = '\0';
            //::MessageBoxA(NULL, path, (""), MB_OK);
            setTweakedPath(ctx, path);
        }
    }
    else if(type==2)
    {
        //::MessageBox(NULL, msg.sType, (L""), MB_OK);
        if(!languages_scanned
#ifdef _DEBUG
            ||(GetKeyState(VK_CONTROL) & 0x8000)
#endif
            ){
            auto & currentLanguageFile = _MDText.getLanguageName();
            if(msg) FindLanguages();
            else languages.push_back(currentLanguageFile);
            TCHAR* pathTmp = (TCHAR*)path;
            language->RemoveAll();
            TCHAR* start = lstrcpy(pathTmp, TEXT("  "))+lstrlen(TEXT("  "));
            int idx=0;
            for(int i=0,len=languages.size();i<len;i++)
            {
                auto listItem = new CListLabelElementUI();
                auto & string = languages[i];
                lstrcpy(start, string.c_str());
                int length = string.length()-4;
                start[length]='\0';
                //lstrcpy(start+length, TEXT("🇨🇳"));
                start[0] = _totupper(start[0]);
                listItem->SetText(pathTmp);
                //listItem->SetToolTip(TEXT("右击选择文件夹"));
                language->Add(listItem);
                if(string==currentLanguageFile)
                {
                    idx=i;
                }
            }
            language->SelectItem(idx);
        }
    }
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
    else if( msg.pSender==language ) {
        if( msg.sType == DUI_MSGTYPE_PREDROPDOWN ) 
        { 
            HandleMessage(2, &msg);
        }
        else if( msg.sType == DUI_MSGTYPE_ITEMSELECT ) 
        { 
            int itemIdx = msg.wParam;
            if(itemIdx>=0&&itemIdx<languages.size()){
                _MDText.setLanguageName(languages[itemIdx]);
                TranslateUI(m_pm, _MDText.getLocaliseMap());
            }
            //::MessageBox(NULL, TEXT("111"), TEXT(""), MB_OK);
        }
    }
    else if( msg.pSender == bwpath || msg.pSender == wkpath || msg.pSender == mbpath ) 
    {
        HandleMessage(0, &msg);
    }
    // notify :: return on the edit control
    else if( msg.pSender == bwedit || msg.pSender == wkedit || msg.pSender == mbedit ) {
        HandleMessage(1, &msg);
    }
}

LRESULT OptionsDlg::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if( uMsg == WM_CREATE ) {
        m_pm.Init(m_hWnd);
        CDialogBuilder builder;

        CControlUI* pRoot = builder.Create(TEXT("settings.xml"), (UINT)0, 0, &m_pm);

        ASSERT(pRoot && "Failed to parse XML");
        m_pm.AttachDialog(pRoot);
        m_pm.AddNotifier(this);


        TranslateUI(m_pm, _MDText.getLocaliseMap());

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
        //if( !::IsIconic(*this) ) return (wParam == 0) ? TRUE : FALSE;
    }
    LRESULT lRes = 0;
    if( m_pm.MessageHandler(uMsg, wParam, lParam, lRes) ) return lRes;
    return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
}