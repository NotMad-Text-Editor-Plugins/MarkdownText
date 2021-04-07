//this file defines the plugin interface of MDText
#include "PluginDefinition.h"
#include <shlwapi.h>
#include <tchar.h>
#include "MDTextDlg.h"
#include <list>
#include <WinBase.h>
#include <set>  
#include <map>  
#include "Scintilla.h"

TCHAR currFile[MAX_PATH]={0};

typedef const TBBUTTON *LPCTBBUTTON;

HWND mainAppWnd;

// export entry point
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved )
{
    switch (reasonForCall)
    {
      case DLL_PROCESS_ATTACH:
	  {
		  GetModuleFileName((HMODULE)hModule, g_ModulePath, MAX_PATH);
		  PathRemoveFileSpec(g_ModulePath);
		  _MDText.init((HINSTANCE)hModule, NULL);
		  g_hModule = hModule;
		  break;
	  }

      case DLL_PROCESS_DETACH:
		NPPRunning=0;
        pluginCleanUp();
        break;
      case DLL_THREAD_ATTACH:
        break;

      case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}

// receive the info struct from main application
extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	nppData = notpadPlusData;
	_MDText.init((HINSTANCE)_MDText.getHinst(), mainAppWnd=nppData._nppHandle);
	commandMenuInit();
}

// export plugin name
extern "C" __declspec(dllexport) const TCHAR * getName()
{
	return NPP_PLUGIN_NAME;
}

// export functions
extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
	*nbF = 10;
	return funcItems;
}

__declspec(selectany)  toolbarIcons		g_TBMarkdown{0,0,0x666,0,IDI_ICON_MD,0,0,IDB_BITMAP1};

bool autoRunChecking=false;

// export the listener
extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
	int ModifyType = notifyCode->modificationType;
	int code = notifyCode->nmhdr.code;
	int NeedUpdate=0;
	switch (code) 
	{
		case NPPN_TBMODIFICATION:
			if(notifyCode->nmhdr.hwndFrom == nppData._nppHandle) {
				/* add toolbar icon */
				auto HRO = (HINSTANCE)g_hModule;

				long filecount2 = ::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, (LPARAM)PRIMARY_VIEW);

				long version = ::SendMessage(nppData._nppHandle, NPPM_GETNOTMADVERSION, 0, 0);

				legacy = version<0x666;

				g_TBMarkdown.HRO = HRO;
				if(legacy)g_TBMarkdown.hToolbarBmp = (HBITMAP)::LoadImage(HRO, MAKEINTRESOURCE(IDB_BITMAP1), IMAGE_BITMAP, 0,0, (LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));
				::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItems[menuOption]._cmdID, (LPARAM)&g_TBMarkdown);

				if(!_MDText.localeSet) {
					TCHAR mStr[16]={0};
					GetMenuString(GetMenu(nppData._nppHandle), 0, mStr, 16, MF_BYPOSITION);
					ZH_CN=mStr[0]==L'文';
					if(!ZH_CN)
					{
						_MDText.currentLanguageFile = TEXT("");
					}
				}
				_MDText.setLanguageName(_MDText.currentLanguageFile, true);
			}
		break;
		case NPPN_SHUTDOWN:
		{
			commandMenuCleanUp();
			NPPRunning=0;
			_MDText.destroyWebViews(true);
			//bIsPaused=1;
			//AllCloseFlag=1;
		}
		break;
		case NPPN_READY:
		{
			NeedUpdate=NPPRunning=true;
			// auto run according to the command line arguments and current active file.
			if(!GetUIBool(6)&&(_MDText.bRunRequested || GetUIBoolReverse(5))) 
			{
				autoRunChecking = true;
			}
		}
		break;
		case NPPN_BEFORESHUTDOWN:
		{
			NPPRunning=false;
		}
		break;
		case NPPN_CANCELSHUTDOWN:
		{
			NPPRunning=true;
		}
		break;
		// mark setting remove to SCN_SAVEPOINTREACHED
		case NPPN_FILESAVED:
		{
		}
		break;
		case NPPN_FILEBEFORECLOSE:
		{
		}
		break;
		case NPPN_FILEBEFOREOPEN:
		case NPPN_FILEBEFORELOAD:
		{
		}
		break;
		case NPPN_FILEOPENED:
		{
			_MDText.buffersMap.insert(notifyCode->nmhdr.idFrom);
		}
		break;
		case NPPN_FILECLOSED:
		{
			if(NPPRunning)
			{
				_MDText.buffersMap.erase(notifyCode->nmhdr.idFrom);
			}
		}
		break;
		// 页面切换
		case NPPN_BUFFERACTIVATED:
		{
			if(NPPRunning)
			{
				NeedUpdate=1;
			}
		}
		break;
		case SCN_MODIFIED:
		{
			if(NPPRunning && notifyCode->length>0 
				&& notifyCode->modificationType & (SC_MOD_DELETETEXT | SC_MOD_INSERTTEXT))
			{
				NeedUpdate=2;
			}
		}
		break;
		case SCN_PAINTED:
		{
			if(NPPRunning)
			{
				//::MessageBox(NULL, TEXT("SCN_PAINTED"), TEXT(""), MB_OK);
				LONG_PTR bid = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
				if(_MDText.lastBid==bid)
				{
					_MDText.syncWebToline();
				}
			}
		}
		break;
		case SCN_UPDATEUI:
		{
			if (autoRunChecking)
			{
				_MDText.checkAutoRun();
				autoRunChecking = false;
			}
		}
		break;
		case SCN_SAVEPOINTREACHED:
		{
			//DoSavedColor();
		}
		break;
		default:
		break;
	}
	//processNavActions();
	if(NeedUpdate)
	{
		if(NeedUpdate==1)
		{
			TCHAR*  pszNewPath;
			if(legacy) {
				pszNewPath = path_buffer;
				::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH,0,(LPARAM)pszNewPath);
			} else {
				pszNewPath = (TCHAR*)::SendMessage(nppData._nppHandle,NPPM_GETRAWFULLCURRENTPATH, 0, 0);
			}

			if(lstrcmp(last_actived, pszNewPath)==0) { 
				return;
			} else {
				lstrcpy(last_actived, pszNewPath);
			}
		}
		else
		{
			_MDText.lastSyncLn=-1;
		}
		bool doUpdate=!GetUIBool(3);
		if(!doUpdate && NeedUpdate==2&&GetUIBoolReverse(4))
		{
			LONG_PTR bid = ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
			doUpdate = _MDText.lastBid==bid;
		}
		if(doUpdate)
			_MDText.refreshDlg(false, NeedUpdate==2);
	}
}

// 
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	// only support WM_SIZE and  WM_MOVE
	return TRUE;
}

// Unicode
#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
#endif
