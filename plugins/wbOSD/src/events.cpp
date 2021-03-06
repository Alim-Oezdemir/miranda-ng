/*
Wannabe OSD
This plugin tries to become miranda's standard OSD ;-)

(C) 2005 Andrej Krutak

Distributed under GNU's GPL 2 or later
*/

#include "stdafx.h"

void logmsg2(char *str)
{
	FILE *f=fopen("c:\\logm.txt", "a");
	fprintf(f, "%s\n", str);
	fclose(f);
}

void showmsgwnd(unsigned int param)
{
	logmsg("showmsgwnd");
	if (db_get_b(NULL,THIS_MODULE, "showMessageWindow", DEFAULT_SHOWMSGWIN))
		CallService(MS_MSG_SENDMESSAGEW, (WPARAM)param, 0);
}

LRESULT ShowOSD(wchar_t *str, int timeout, COLORREF color, MCONTACT user)
{
	logmsg("ShowOSD");

	if (!g_hWnd)
		return 0;

	if (timeout==0)
		timeout=db_get_dw(NULL,THIS_MODULE, "timeout", DEFAULT_TIMEOUT);

	osdmsg om;
	om.text=str;
	om.timeout=timeout;
	om.color=color;
	om.param=(unsigned int)user;
	om.callback=showmsgwnd;
	
	return SendMessage(g_hWnd, WM_USER+4, (WPARAM)&om, 0);
}

int ProtoAck(WPARAM,LPARAM lparam)
{
	ACKDATA *ack=(ACKDATA *)lparam;
	
	logmsg("ProtoAck");

	if (!db_get_b(NULL,THIS_MODULE, "a_user", DEFAULT_ANNOUNCESTATUS))
		return 0;

	if (!(db_get_dw(NULL,THIS_MODULE,"showWhen", DEFAULT_SHOWWHEN)&(1<<(db_get_w(NULL, "CList", "Status", ID_STATUS_OFFLINE)-ID_STATUS_OFFLINE))))
		return 0;

	if ( ack->type == ACKTYPE_STATUS ) {
		if (!db_get_b(NULL,THIS_MODULE, "showMyStatus", DEFAULT_SHOWMYSTATUS))
			return 0;

		if ( ack->result == ACKRESULT_SUCCESS && (LPARAM)ack->hProcess != ack->lParam ) {
			DWORD ann = db_get_dw( NULL, THIS_MODULE, "announce", DEFAULT_ANNOUNCE );
			if ( ann & ( 1 << ( ack->lParam - ID_STATUS_OFFLINE ))) {
				wchar_t buffer[512];
				mir_snwprintf(buffer, TranslateT("%s is %s"), Clist_GetContactDisplayName(ack->hContact), Clist_GetStatusModeDescription(ack->lParam, 0));
				ShowOSD(buffer, 0, db_get_dw(NULL,THIS_MODULE, "clr_status", DEFAULT_CLRSTATUS), ack->hContact);
	}	}	}

	return 0;
}

int ContactSettingChanged(WPARAM wParam,LPARAM lParam)
{
	MCONTACT hContact = (MCONTACT) wParam;
	DBCONTACTWRITESETTING *cws=(DBCONTACTWRITESETTING*)lParam;

	if(hContact==NULL || strcmp(cws->szSetting,"Status")) return 0;

	logmsg("ContactSettingChanged1");

	WORD newStatus = cws->value.wVal;
	WORD oldStatus = DBGetContactSettingRangedWord(hContact,"UserOnline","OldStatus2",ID_STATUS_OFFLINE, ID_STATUS_MIN, ID_STATUS_MAX);
	
	if (oldStatus == newStatus) return 0;
	
	logmsg("ContactSettingChanged2");

	db_set_w(hContact,"UserOnline","OldStatus2", newStatus);

	if(CallService(MS_IGNORE_ISIGNORED,wParam,IGNOREEVENT_USERONLINE)) return 0;

	DWORD dwStatuses = MAKELPARAM(oldStatus, newStatus);
	NotifyEventHooks(hHookContactStatusChanged, wParam, (LPARAM)dwStatuses);

	return 0;
}

int ContactStatusChanged(WPARAM wParam, LPARAM lParam)
{
	MCONTACT hContact = (MCONTACT) wParam;
	WORD newStatus = HIWORD(lParam);
	DWORD ann=db_get_dw(NULL,THIS_MODULE,"announce", DEFAULT_ANNOUNCE);

	logmsg("ContactStatusChanged1");

	if (!db_get_b(NULL,THIS_MODULE, "a_user", DEFAULT_ANNOUNCESTATUS))
		return 0;

	if (!(db_get_dw(NULL,THIS_MODULE,"showWhen", DEFAULT_SHOWWHEN)&(1<<(db_get_w(NULL, "CList", "Status", ID_STATUS_OFFLINE)-ID_STATUS_OFFLINE))))
		return 0;

	if (!(ann&(1<<(newStatus-ID_STATUS_OFFLINE))) )
		return 0;
	
	logmsg("ContactStatusChanged2");

	if (db_get_b(hContact,"CList","NotOnList",0) || db_get_b(hContact,"CList","Hidden",0) || 
		(CallService(MS_IGNORE_ISIGNORED,wParam,IGNOREEVENT_USERONLINE) && newStatus==ID_STATUS_ONLINE)
	)
		return 0;

	wchar_t bufferW[512];
	mir_snwprintf(bufferW, TranslateT("%s is %s"), Clist_GetContactDisplayName(wParam), Clist_GetStatusModeDescription(newStatus, 0));
	ShowOSD(bufferW, 0, db_get_dw(NULL,THIS_MODULE, "clr_status", DEFAULT_CLRSTATUS), hContact);
	return 0;
}

int HookedNewEvent(WPARAM wParam, LPARAM hDBEvent)
{
	logmsg("HookedNewEvent1");
	DBEVENTINFO dbe;
	dbe.cbBlob = db_event_getBlobSize(hDBEvent);
	if (dbe.cbBlob == -1)
		return 0;

	dbe.pBlob = (PBYTE) malloc(dbe.cbBlob);
	if(db_event_get(hDBEvent,&dbe))
		return 0;

	if (dbe.flags & DBEF_SENT)
	    return 0;

	if (db_get_b(NULL,THIS_MODULE, "messages", DEFAULT_ANNOUNCEMESSAGES)==0)
		return 0;

	if (!(db_get_dw(NULL,THIS_MODULE,"showWhen", DEFAULT_SHOWWHEN)&(1<<(db_get_w(NULL, "CList", "Status", ID_STATUS_OFFLINE)-ID_STATUS_OFFLINE))))
		return 0;
	
	logmsg("HookedNewEvent2");

	wchar_t buf[512];
	wcsncpy(buf, DEFAULT_MESSAGEFORMAT,_countof(buf));

	DBVARIANT dbv;
	if(!db_get_ws(NULL,THIS_MODULE,"message_format",&dbv)) {
		mir_wstrcpy(buf, dbv.ptszVal);
		db_free(&dbv);
	}

	int i1=-1, i2=-1;
	wchar_t* pbuf = buf;
	while (*pbuf) {
		if (*pbuf=='%') {
			if (*(pbuf+1)=='n') {
				if (i1==-1)
					i1=1;
				else i2=1;
				*(pbuf+1)='s';
			} else if (*(pbuf+1)=='m') {
				if (i1==-1)
					i1=2;
				else i2=2;
				*(pbuf+1)='s';
			} else if (*(pbuf+1)=='l') {
				*pbuf=0x0d;
				*(pbuf+1)=0x0a;
			}
		}
		pbuf++;
	}

	wchar_t *c1 = nullptr, *c2 = nullptr;
	if ( i1 == 1 )
		c1 = mir_wstrdup(Clist_GetContactDisplayName(wParam));
	else if ( i1 == 2 )
		c1 = DbEvent_GetTextW( &dbe, 0 );

	if ( i2 == 1 )
		c2 = mir_wstrdup(Clist_GetContactDisplayName(wParam));
	else if ( i2 == 2 )
		c2 = DbEvent_GetTextW( &dbe, 0 );

	wchar_t buffer[512];
	mir_snwprintf(buffer, buf, c1, c2);
	ShowOSD(buffer, 0, db_get_dw(NULL,THIS_MODULE, "clr_msg", DEFAULT_CLRMSG), wParam);

	mir_free( c1 );
	mir_free( c2 );
	return 0;
}
