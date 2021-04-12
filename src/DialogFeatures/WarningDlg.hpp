#pragma once
#ifndef _WARNING_DLG_H_
#define _WARNING_DLG_H_

#include "UIlib.h"

#include "ControlEx/DuiLibTranslator.h"

extern HWND mainAppWnd;

using namespace DuiLib;

class WarnDlg : public CWindowWnd, public INotifyUI
{
public:
	WarnDlg(TCHAR* _resourcePath) {
		resourcePath = _resourcePath;
	}
	LPCTSTR GetWindowClassName() const { 
		return _T("UIMainFrame"); 
	};
	UINT GetClassStyle() const {
		return UI_CLASSSTYLE_FRAME | CS_DBLCLKS; 
	};
	void OnFinalMessage(HWND /*hWnd*/){
		delete this;
	};

	void Init() { }

	void Notify(TNotifyUI& msg){
		if( msg.sType == _T("click") ) 
		{
			auto userdata = msg.pSender->GetUserData();
			if(!userdata.IsEmpty())
			{
				if(!_tcsnccmp(userdata.GetData(), TEXT("http"), 4))
				{
					// open it's url
					ShellExecute(NULL, TEXT("open"), userdata.GetData(), NULL, NULL, SW_SHOWNORMAL);
				}
			}
		}
	}

	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam){
		if( uMsg == WM_CREATE ) {
			m_pm.Init(m_hWnd);
			CDialogBuilder builder;
			CControlUI* pRoot = builder.Create(resourcePath, (UINT)0, 0, &m_pm);
			ASSERT(pRoot && "Failed to parse XML");
			m_pm.AttachDialog(pRoot);
			m_pm.AddNotifier(this);
			Init();

			if (localizationMap)
			{
				TranslateUI(m_pm, *localizationMap);
			}

			return 0;
		}
		else if( isModal && uMsg == WM_DESTROY ) 
		{
			EnableWindow(mainAppWnd, true);
			ShowWindow(false);
		}
		else if( uMsg == WM_CLOSE  ) {
			if (isModal)
			{
				EnableWindow(mainAppWnd, true);
			}
			if (m_hParent) 
			{
				NMHDR nmh;
				nmh.code = -1;
				nmh.idFrom = -1;
				nmh.hwndFrom = m_hWnd;
				::SendMessage(m_hParent, WM_NOTIFY, nmh.idFrom, (LPARAM)&nmh);
			}
		}
		else if( uMsg == WM_NCACTIVATE ) {
			//if( !::IsIconic(*this) ) return (wParam == 0) ? TRUE : FALSE;
		}
		else if( uMsg == WM_ACTIVATE ) {
			active = wParam;
			if(exit_requested)
			{
				exit_requested=false;
			}
		}
		else if( (wParam == VK_RETURN || wParam == VK_ESCAPE) && GetFocus()==m_hWnd ) 
		{
			if(uMsg==WM_KEYDOWN && active)
			{
				exit_requested=true;
				return true;
			}
			else if( uMsg==WM_KEYUP &&exit_requested ) 
			{
				Close();
				return true;
			}
		}
		LRESULT lRes = 0;
		if( m_pm.MessageHandler(uMsg, wParam, lParam, lRes) ) return lRes;
		return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
	}
public:
	TCHAR* resourcePath;
	bool active;
	bool exit_requested=false;
	CPaintManagerUI m_pm;

};

#endif