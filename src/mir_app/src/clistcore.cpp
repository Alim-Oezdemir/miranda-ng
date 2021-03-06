/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (c) 2012-18 Miranda NG team (https://miranda-ng.org),
Copyright (c) 2000-12 Miranda IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "stdafx.h"
#include "clc.h"
#include "genmenu.h"

int LoadContactListModule2(void);
int LoadCLCModule(void);

CLIST_INTERFACE cli;

static wchar_t szTip[MAX_TIP_SIZE+1];

static void fnPaintClc(HWND, ClcData*, HDC, RECT*)
{
}

static ClcContact* fnCreateClcContact(void)
{
	return (ClcContact*)mir_calloc(sizeof(ClcContact));
}

static BOOL fnInvalidateRect(HWND hwnd, CONST RECT* lpRect, BOOL bErase)
{
	return InvalidateRect(hwnd, lpRect, bErase);
}

static void fnOnCreateClc(void)
{
}

static int fnIsVisibleContact(ClcCacheEntry*, ClcGroup*)
{
	return false;
}

void InitClistCore()
{
	cli.menuProtos = &g_menuProtos;

	cli.pfnContactListControlWndProc = fnContactListControlWndProc;

	cli.pfnGetRowsPriorTo = fnGetRowsPriorTo;
	cli.pfnFindItem = fnFindItem;
	cli.pfnGetRowByIndex = fnGetRowByIndex;
	cli.pfnGetContactHiddenStatus = fnGetContactHiddenStatus;

	cli.pfnAddGroup = fnAddGroup;
	cli.pfnAddItemToGroup = fnAddItemToGroup;
	cli.pfnCreateClcContact = fnCreateClcContact;
	
	cli.pfnFreeContact = fnFreeContact;	
	cli.pfnAddInfoItemToGroup = fnAddInfoItemToGroup;	
	cli.pfnAddContactToGroup = fnAddContactToGroup;	
	cli.pfnAddContactToTree = fnAddContactToTree;	
	cli.pfnRebuildEntireList = fnRebuildEntireList;	
	cli.pfnGetGroupContentsCount = fnGetGroupContentsCount;
	cli.pfnSortCLC = fnSortCLC;

	cli.pfnProcessExternalMessages = fnProcessExternalMessages;

	cli.pfnPaintClc = fnPaintClc;

	cli.pfnHitTest = fnHitTest;	
	cli.pfnScrollTo = fnScrollTo;	
	cli.pfnRecalcScrollBar = fnRecalcScrollBar;	
	cli.pfnSetGroupExpand = fnSetGroupExpand;
	cli.pfnFindRowByText = fnFindRowByText;	
	cli.pfnBeginRenameSelection = fnBeginRenameSelection;
	cli.pfnIsVisibleContact = fnIsVisibleContact;
	cli.pfnGetDefaultFontSetting = fnGetDefaultFontSetting;
	cli.pfnLoadClcOptions = fnLoadClcOptions;
	cli.pfnGetRowBottomY = fnGetRowBottomY;
	cli.pfnGetRowHeight = fnGetRowHeight;
	cli.pfnGetRowTopY = fnGetRowTopY;
	cli.pfnGetRowTotalHeight = fnGetRowTotalHeight;
	cli.pfnRowHitTest = fnRowHitTest;

	cli.pfnAddEvent = fnAddEvent;
	cli.pfnGetEvent = fnGetEvent;
	cli.pfnGetImlIconIndex = fnGetImlIconIndex;
	cli.pfnRemoveEvent = fnRemoveEvent;

	cli.pfnInvalidateDisplayNameCacheEntry = fnInvalidateDisplayNameCacheEntry;
	cli.pfnCreateCacheItem = fnCreateCacheItem;
	cli.pfnCheckCacheItem = fnCheckCacheItem;
	cli.pfnFreeCacheItem = fnFreeCacheItem;

	cli.szTip = szTip;

	cli.pfnTrayIconInit = fnTrayIconInit;
	cli.pfnTrayIconPauseAutoHide = fnTrayIconPauseAutoHide;
	cli.pfnTrayIconProcessMessage = fnTrayIconProcessMessage;

	cli.pfnContactListWndProc = fnContactListWndProc;
	cli.pfnLoadCluiGlobalOpts = fnLoadCluiGlobalOpts;
	cli.pfnCluiProtocolStatusChanged = fnCluiProtocolStatusChanged;
	cli.pfnInvalidateRect = fnInvalidateRect;
	cli.pfnOnCreateClc = fnOnCreateClc;

	cli.pfnSetHideOffline = fnSetHideOffline;

	cli.pfnDocking_ProcessWindowMessage = fnDocking_ProcessWindowMessage;

	cli.pfnGetIconFromStatusMode = fnGetIconFromStatusMode;
	cli.pfnGetWindowVisibleState = fnGetWindowVisibleState;
	cli.pfnIconFromStatusMode = fnIconFromStatusMode;
	cli.pfnShowHide = fnShowHide;

	cli.pfnTrayCalcChanged = fnTrayCalcChanged;
	cli.pfnSetContactCheckboxes = fnSetContactCheckboxes;
}

MIR_APP_DLL(CLIST_INTERFACE*) Clist_GetInterface(void)
{
	if (g_bReadyToInitClist) {
		LoadContactListModule2();
		LoadCLCModule();
		g_bReadyToInitClist = false;
	}

	return &cli;
}
