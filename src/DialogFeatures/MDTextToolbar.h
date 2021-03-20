#pragma once
#ifndef _MDTEXT_TB_H_
#define _MDTEXT_TB_H_

#include "stdafx.h"
#include "resource.h"
#include "ToolbarPanel.h"

ToolBarButtonUnit PrivateToolBarIconList[] = {
	{IDM_EX_TOGGLE, ICO_EX_TOGGLE, ICO_EX_TOGGLE, ICO_EX_TOGGLE, 0 }, 
	{IDM_EX_DOWN, ICO_EX_DOWN, ICO_EX_DOWN, ICO_EX_DOWN, 0 }, 
	{IDM_EX_UP, ICO_EX_UP, ICO_EX_UP, ICO_EX_UP, 0 }, 
	{IDM_EX_REFRESH, ICO_EX_Refresh, ICO_EX_Refresh, ICO_EX_Refresh, 0 }, 
	{IDM_EX_DELTA, ICO_EX_DELTA, ICO_EX_DELTA, ICO_EX_DELTA, 0 }, 
	{IDM_EX_ZOI, ICO_EX_ZOI, ICO_EX_ZOI, ICO_EX_ZOI, 0 }, 
	{IDM_EX_ZOO, ICO_EX_ZOO, ICO_EX_ZOO, ICO_EX_ZOO, 0 }, 
	{IDM_EX_BOLDEN, ICO_EX_BOLDEN, ICO_EX_BOLDEN, ICO_EX_BOLDEN, 0 }, 
	{IDM_EX_ITALIC, ICO_EX_ITALIC, ICO_EX_ITALIC, ICO_EX_ITALIC, 0 }, 
	{IDM_EX_DEV, ICO_EX_DEV, ICO_EX_DEV, ICO_EX_DEV, 0 }, 
	{IDM_EX_LOCATE, ICO_EX_LOCATE, ICO_EX_LOCATE, ICO_EX_LOCATE, 0 }, 
};

#define ListBoxToolBarSize sizeof(PrivateToolBarIconList)/sizeof(ToolBarButtonUnit)

//	Note: On change, keep sure to change order of IDM_EX_... also in function GetNameStrFromCmd
LPTSTR ListBoxToolBarToolTip[] = {
	TEXT("Options"),
	TEXT("Go Back"),
	TEXT("Go Forward"),
	TEXT("Refresh"),
	TEXT("Alter Engine"),
	TEXT("Zoom In"),
	TEXT("Zoom Out"),
	TEXT("Bold"),
	TEXT("Italic"),
	TEXT("DevTools"),
	TEXT("Sync-Scroll"),
};

LPTSTR ListBoxToolBarToolTip_HAN[] = {
	TEXT("选项"),
	TEXT("后退"),
	TEXT("前进"),
	TEXT("刷新"),
	TEXT("切换引擎"),
	TEXT("放大"),
	TEXT("缩小"),
	TEXT("粗体"),
	TEXT("斜体"),
	TEXT("开发工具"),
	TEXT("同步滚动"),
};

#endif