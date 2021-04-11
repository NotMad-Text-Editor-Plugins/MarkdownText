#include "OptionsDlg.h"

#include "ControlEx/UIBeautifulSwitch.h"

#include "ControlEx/DuiLibTranslator.h"

#include "MDTextDlg.h"

#include "ProfileStd.h"

using namespace DuiLib;

// Building the settings dialog using Duilib.
int remapRndType(int type)
{
    return type==1?2:type==2?1:type;
}

OptionsDlg::OptionsDlg()
{ 
    CControlFactory::GetInstance()->RegistControl(TEXT("CBSwitchUI"), CBSwitchUI::CreateControl);
};

UINT OptionsDlg::GetClassStyle() const 
{
    return UI_CLASSSTYLE_FRAME | CS_DBLCLKS; 
};

extern OptionsDlg* pFrame;

void OptionsDlg::OnFinalMessage(HWND hWnd) { 
    pFrame = NULL;
    delete this; 
};

// not used
bool OptionsDlg::OnValueChanged(void* param) {
    return true;
}

// LibPaths Combo Edit Storage
struct ComboeditContext
{
    CComboUI*   combo;
    CControlUI* edit;
    int * sel;
    std::vector<std::string*> * paths;
    CHAR* pick_tips;
    CHAR* key;
};

// Make a "editable" combobox. An edit control was placed on the top of it.
// Background images from "GameDemo"
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

// Initalize a simple material design checkbox.
void drawSwitcherUI(CPaintManagerUI & m_pm, TCHAR* name, bool select)
{
    if( select ) {
        COptionUI* sw = dynamic_cast<COptionUI*>(m_pm.FindControl(name));
        if(sw)
        {
            sw->Selected(select, false);
        }
    }
}

void drawExtEdit(CPaintManagerUI & m_pm, TCHAR* name, int channel)
{
    CControlUI* ext = dynamic_cast<CControlUI*>(m_pm.FindControl(name));
    if(ext)
    {
        string text;
       _MDText.readExtensions(channel, &text);
        if(text.length()) 
        {
            TCHAR tmpText[MAX_PATH]{0};
            MultiByteToWideChar(CP_ACP, 0, text.data(), -1, tmpText, MAX_PATH-1); 
            ext->SetText(tmpText);
        }
    }
}

//Initialize the UI
void OptionsDlg::OnPrepare() 
{
    bwpath = static_cast<CComboUI*>(  m_pm.FindControl(_T("bwpath")));
    bwedit = static_cast<CControlUI*>(m_pm.FindControl(_T("bwedit")));
    mbpath = static_cast<CComboUI*>(  m_pm.FindControl(_T("mbpath")));
    mbedit = static_cast<CControlUI*>(m_pm.FindControl(_T("mbedit")));
    wkpath = static_cast<CComboUI*>(  m_pm.FindControl(_T("wkpath")));
    wkedit = static_cast<CControlUI*>(m_pm.FindControl(_T("wkedit")));

    rndType = static_cast<CComboUI*>(m_pm.FindControl(_T("rndType")));  // default render types

    language = static_cast<CComboUI*>(  m_pm.FindControl(_T("language")));// languages

    drawComboEditUI(bwpath, bwedit, _MDText.LibCefSel, _MDText.LibPaths, "Pick LibCef folder: ( contains cefclient.dll )", "LibPath%d");
    drawComboEditUI(mbpath, mbedit, _MDText.LibMbSel, _MDText.MbPaths, "Pick Miniblink folder: ( contains miniblink_x64.dll )", "MbPath%d");
    //drawComboEditUI(wkpath, wkedit, _MDText.LibWkeSel, _MDText.WkePaths, "Pick WKE Path: ( wke.dll )", "WkePath%d");
    
    drawSwitcherUI(m_pm, TEXT("sw1"), GetUIBoolReverse(5)); // auto run
    drawSwitcherUI(m_pm, TEXT("sw2"), GetUIBool(6));        // auto close
    drawSwitcherUI(m_pm, TEXT("sw3"), GetUIBoolReverse(7)); // auto switch
    drawSwitcherUI(m_pm, TEXT("sw4"), GetUIBool(9));        // save update

    drawExtEdit(m_pm, TEXT("mddt"), 0);
    drawExtEdit(m_pm, TEXT("addt"), 1);
    drawExtEdit(m_pm, TEXT("htdt"), 2);

    rndType->SelectItem(remapRndType(_MDText.defaultRenderer));

    HandleMessage(2, NULL); // localize
}

// Set the active LibPaths
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

// Initialze the folder picker
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

// Find all files of localization/*.ini
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

    if (GetLastError() != ERROR_NO_MORE_FILES) 
    {
        FindClose(hFind);
        return false;
    }

    FindClose(hFind);
    hFind = INVALID_HANDLE_VALUE;

    return true;
}

// To use the same stack data.
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

void PutHotTextToIni(CControlUI* pSender)
{
    int id = _ttoi(pSender->GetUserData());
    if(id>=0&&id<=3)
    {
        CHAR path[MAX_PATH];
        auto tmpText = pSender->GetText();
        int len = WideCharToMultiByte(CP_ACP, 0, tmpText
            , tmpText.GetLength(), path, MAX_PATH-1, 0, 0); // bytes conversion :: data from display to ini
        path[len] = '\0';
        PutProfString(_MDText.extCtx[id].key, path);
        _MDText.readExtensions(id, NULL);
    }
}

// Duilib callback
void OptionsDlg::Notify(TNotifyUI& msg)
{
    if( msg.sType == _T("windowinit") ) OnPrepare();
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
    else if( msg.pSender == rndType ) 
    {
        _MDText.defaultRenderer = remapRndType(rndType->GetCurSel());
    }
    // notify :: return on the edit control
    else if( msg.pSender == bwedit || msg.pSender == wkedit || msg.pSender == mbedit ) {
        HandleMessage(1, &msg);
    }
    else 
    {
        CBSwitchUI* switcher = dynamic_cast<CBSwitchUI*>(msg.pSender);
        if(switcher)
        {
            if(msg.sType == DUI_MSGTYPE_SELECTCHANGED)
            {
                auto & ud = switcher->GetUserData();
                if(!ud.IsEmpty())
                {
                    int switcherid = _ttoi(ud.GetData());
                    bool reverse=false;
                    if(switcherid<0)
                    {
                        reverse = true;
                        switcherid = -switcherid;
                    }
                    if(GetUIBool(switcherid, reverse)!=switcher->IsSelected())
                    {
                        ToggleUIBool(switcherid, reverse);
                    }
                }
            }
            return;
        }

        CEditUI* edit = dynamic_cast<CEditUI*>(msg.pSender);
        if(edit)
        {
            // notify :: return on the edit control of extension fields
            // todo get rid of the beep sound

            auto & name = msg.pSender->GetName();
            if(name.GetLength()==4&&lstrcmp(name.GetData()+2, TEXT("ht")))
            {
                if(msg.sType == DUI_MSGTYPE_TEXTCHANGED)
                {
                    PutHotTextToIni(msg.pSender);
                }
                //else if(msg.sType == DUI_MSGTYPE_RETURN)
                //{
                //    PutHotTextToIni(msg.pSender);
                //}
            }
            return;
        }

        CButtonUI* btn = dynamic_cast<CButtonUI*>(msg.pSender);
        if (btn && msg.sType==_T("click"))
        {
            _MDText.displayInstallGuide();
            return;
        }
    }
}

// Window Messages callback
LRESULT OptionsDlg::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if( uMsg == WM_CREATE ) 
    {
        m_pm.Init(m_hWnd);
        CDialogBuilder builder;

        CControlUI* pRoot = builder.Create(TEXT("settings.xml"), (UINT)0, 0, &m_pm);

        ASSERT(pRoot && "Failed to parse XML");
        m_pm.AttachDialog(pRoot);
        m_pm.AddNotifier(this);

        TranslateUI(m_pm, _MDText.getLocaliseMap());

        //m_pm.SetDPI(100);  // Set the new DPI, retrieved from the wParam
        return 0;
    }
    if( uMsg == WM_GETICON ) {
        return NULL;
    }
    LRESULT lRes = 0;
    if( m_pm.MessageHandler(uMsg, wParam, lParam, lRes) ) return lRes;
    return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
}