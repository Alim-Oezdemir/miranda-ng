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

int LoadAwayMessageSending(void);

static HGENMENU hAwayMsgMenuItem;
static MWindowList hWindowList;

struct AwayMsgDlgData {
	MCONTACT hContact;
	HANDLE hSeq;
	HANDLE hAwayMsgEvent;
};

#define HM_AWAYMSG (WM_USER+10)

static INT_PTR CALLBACK ReadAwayMsgDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	AwayMsgDlgData *dat = (AwayMsgDlgData*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (message) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		dat = (AwayMsgDlgData*)mir_alloc(sizeof(AwayMsgDlgData));
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)dat);

		dat->hContact = db_mc_getMostOnline(lParam);
		if (dat->hContact == NULL)
			dat->hContact = lParam;
		dat->hAwayMsgEvent = HookEventMessage(ME_PROTO_ACK, hwndDlg, HM_AWAYMSG);
		dat->hSeq = (HANDLE)ProtoChainSend(dat->hContact, PSS_GETAWAYMSG, 0, 0);
		WindowList_Add(hWindowList, hwndDlg, dat->hContact);
		{
			wchar_t str[256], format[128];
			wchar_t *contactName = Clist_GetContactDisplayName(dat->hContact);
			char *szProto = GetContactProto(dat->hContact);
			WORD dwStatus = db_get_w(dat->hContact, szProto, "Status", ID_STATUS_OFFLINE);
			wchar_t *status = Clist_GetStatusModeDescription(dwStatus, 0);

			GetWindowText(hwndDlg, format, _countof(format));
			mir_snwprintf(str, format, status, contactName);
			SetWindowText(hwndDlg, str);

			GetDlgItemText(hwndDlg, IDC_RETRIEVING, format, _countof(format));
			mir_snwprintf(str, format, status);
			SetDlgItemText(hwndDlg, IDC_RETRIEVING, str);

			Window_SetProtoIcon_IcoLib(hwndDlg, szProto, dwStatus);
		}

		if (dat->hSeq == nullptr) {
			ACKDATA ack = { 0 };
			ack.hContact = dat->hContact;
			ack.type = ACKTYPE_AWAYMSG;
			ack.result = ACKRESULT_SUCCESS;
			SendMessage(hwndDlg, HM_AWAYMSG, 0, (LPARAM)&ack);
		}
		Utils_RestoreWindowPosition(hwndDlg, lParam, "SRAway", "AwayMsgDlg");
		return TRUE;

	case HM_AWAYMSG:
		{
			ACKDATA *ack = (ACKDATA*)lParam;
			if (ack->hContact != dat->hContact || ack->type != ACKTYPE_AWAYMSG) break;
			if (ack->result != ACKRESULT_SUCCESS) break;
			if (dat->hAwayMsgEvent && ack->hProcess == dat->hSeq) { UnhookEvent(dat->hAwayMsgEvent); dat->hAwayMsgEvent = nullptr; }

			SetDlgItemText(hwndDlg, IDC_MSG, (const wchar_t*)ack->lParam);

			ShowWindow(GetDlgItem(hwndDlg, IDC_RETRIEVING), SW_HIDE);
			ShowWindow(GetDlgItem(hwndDlg, IDC_MSG), SW_SHOW);
			SetDlgItemText(hwndDlg, IDOK, TranslateT("&Close"));
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
		case IDOK:
			DestroyWindow(hwndDlg);
			break;
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		break;

	case WM_DESTROY:
		if (dat->hAwayMsgEvent) UnhookEvent(dat->hAwayMsgEvent);
		Utils_SaveWindowPosition(hwndDlg, dat->hContact, "SRAway", "AwayMsgDlg");
		WindowList_Remove(hWindowList, hwndDlg);
		Window_FreeIcon_IcoLib(hwndDlg);
		mir_free(dat);
		break;
	}
	return FALSE;
}

static INT_PTR GetMessageCommand(WPARAM wParam, LPARAM)
{
	if (HWND hwnd = WindowList_Find(hWindowList, wParam)) {
		SetForegroundWindow(hwnd);
		SetFocus(hwnd);
	}
	else CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_READAWAYMSG), NULL, ReadAwayMsgDlgProc, wParam);
	return 0;
}

static int AwayMsgPreBuildMenu(WPARAM hContact, LPARAM)
{
	char *szProto = GetContactProto(hContact);
	if (szProto != nullptr) {
		int chatRoom = db_get_b(hContact, szProto, "ChatRoom", 0);
		if (!chatRoom) {
			int status = db_get_w(hContact, szProto, "Status", ID_STATUS_OFFLINE);
			if (CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_1, 0) & PF1_MODEMSGRECV) {
				if (CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_3, 0) & Proto_Status2Flag(status)) {
					wchar_t str[128];
					mir_snwprintf(str, TranslateT("Re&ad %s message"), Clist_GetStatusModeDescription(status, 0));
					Menu_ModifyItem(hAwayMsgMenuItem, str, Skin_LoadProtoIcon(szProto, status), CMIF_NOTOFFLINE);
					return 0;
				}
			}
		}
	}

	Menu_ShowItem(hAwayMsgMenuItem, false);
	return 0;
}

static int AwayMsgPreShutdown(WPARAM, LPARAM)
{
	if (hWindowList) {
		WindowList_BroadcastAsync(hWindowList, WM_CLOSE, 0, 0);
		WindowList_Destroy(hWindowList);
	}
	return 0;
}

int LoadAwayMsgModule(void)
{
	hWindowList = WindowList_Create();
	CreateServiceFunction(MS_AWAYMSG_SHOWAWAYMSG, GetMessageCommand);

	CMenuItem mi;
	SET_UID(mi, 0xd3282acc, 0x9ff1, 0x4ede, 0x8a, 0x1e, 0x36, 0x72, 0x3f, 0x44, 0x4f, 0x84);
	mi.position = -2000005000;
	mi.flags = CMIF_NOTOFFLINE;
	mi.name.a = LPGEN("Re&ad status message");
	mi.pszService = MS_AWAYMSG_SHOWAWAYMSG;
	hAwayMsgMenuItem = Menu_AddContactMenuItem(&mi);
	HookEvent(ME_CLIST_PREBUILDCONTACTMENU, AwayMsgPreBuildMenu);
	HookEvent(ME_SYSTEM_PRESHUTDOWN, AwayMsgPreShutdown);
	return LoadAwayMessageSending();
}
