//this file is part of notepad++
//Copyright (C) 2011 AustinYoung<pattazl@gmail.com>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include "PluginDefinition.h"
#include <shlwapi.h>
#include <tchar.h>
#include "MDTextDlg.h"
#include <list>
#include <WinBase.h>
#include <set>  
#include <map>  
#include "Scintilla.h"

#define UseThread 0

////////////////SELF DATA BEGIN///////////
TCHAR currFile[MAX_PATH]={0};
static int currBufferID=-1;
extern FuncItem funcItem[nbFunc];
extern NppData nppData;

extern HANDLE				g_hModule;

//////////  SELF FUNCTION END ////////
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved )
{
    switch (reasonForCall)
    {
      case DLL_PROCESS_ATTACH:
	  {
		  pluginInit(hModule);
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


extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	nppData = notpadPlusData;

	_MDText.init((HINSTANCE)_MDText.getHinst(), nppData._nppHandle);

	commandMenuInit();
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
	return NPP_PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
	*nbF = nbFunc;
	return funcItem;
}

bool			legacy;
TCHAR*			buffer;
TCHAR*			last_actived = new TCHAR[MAX_PATH]{0};

typedef const TBBUTTON *LPCTBBUTTON;
static long preModifyPos = -1;//之前在的位置
static long preModifyLineAdd = -1;//之前添加的行数
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
				::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[menuOption]._cmdID, (LPARAM)&g_TBMarkdown);


			}
		break;
		case NPPN_SHUTDOWN:
		{
			commandMenuCleanUp();
			NPPRunning=0;
			//bIsPaused=1;
			//AllCloseFlag=1;
		}
		break;
		case NPPN_READY:
		{
			NeedUpdate=NPPRunning=true;
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
		case NPPN_FILECLOSED:
		{
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
		case SCN_UPDATEUI:
		{
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
				pszNewPath = buffer?buffer:(buffer=new TCHAR[MAX_PATH]);
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
		_MDText.refreshDlg(false, NeedUpdate==2);
	}
}


// Here you can process the Npp Messages 
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :
// http://sourceforge.net/forum/forum.php?forum_id=482781
//

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	// only support WM_SIZE and  WM_MOVE
	return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
#endif //UNICODE
