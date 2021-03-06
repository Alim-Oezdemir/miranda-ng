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

bool CheckProtocolOrder(void);
void BuildProtoMenus();

HICON Proto_GetIcon(PROTO_INTERFACE *ppro, int iconIndex);

static bool bModuleInitialized = false;
static HANDLE hHooks[4];

static int CompareAccounts(const PROTOACCOUNT* p1, const PROTOACCOUNT* p2)
{
	return mir_strcmp(p1->szModuleName, p2->szModuleName);
}

LIST<PROTOACCOUNT> accounts(10, CompareAccounts);

/////////////////////////////////////////////////////////////////////////////////////////

static int EnumDbModules(const char *szModuleName, void*)
{
	ptrA szProtoName(db_get_sa(0, szModuleName, "AM_BaseProto"));
	if (szProtoName) {
		if (!Proto_GetAccount(szModuleName)) {
			PROTOACCOUNT *pa = (PROTOACCOUNT*)mir_calloc(sizeof(PROTOACCOUNT));
			pa->szModuleName = mir_strdup(szModuleName);
			pa->szProtoName = szProtoName.detach();
			pa->tszAccountName = mir_a2u(szModuleName);
			pa->bIsVisible = true;
			pa->bIsEnabled = false;
			pa->iOrder = accounts.getCount();
			pa->iIconBase = -1;
			pa->iRealStatus = ID_STATUS_OFFLINE;
			accounts.insert(pa);
		}
	}
	return 0;
}

void LoadDbAccounts(void)
{
	int ver = db_get_dw(0, "Protocols", "PrVer", -1);
	int count = db_get_dw(0, "Protocols", "ProtoCount", 0);

	for (int i = 0; i < count; i++) {
		char buf[10];
		_itoa(i, buf, 10);
		char *szModuleName = db_get_sa(0, "Protocols", buf);
		if (szModuleName == nullptr)
			continue;

		PROTOACCOUNT *pa = Proto_GetAccount(szModuleName);
		if (pa == nullptr) {
			pa = (PROTOACCOUNT*)mir_calloc(sizeof(PROTOACCOUNT));
			pa->szModuleName = szModuleName;
			pa->iIconBase = -1;
			pa->iRealStatus = ID_STATUS_OFFLINE;
			accounts.insert(pa);
		}
		else {
			mir_free(szModuleName);
			szModuleName = pa->szModuleName;
		}

		_itoa(OFFSET_VISIBLE + i, buf, 10);
		pa->bIsVisible = db_get_dw(0, "Protocols", buf, 1) != 0;

		_itoa(OFFSET_PROTOPOS + i, buf, 10);
		pa->iOrder = db_get_dw(0, "Protocols", buf, 1);

		if (ver >= 4) {
			_itoa(OFFSET_NAME + i, buf, 10);
			pa->tszAccountName = db_get_wsa(0, "Protocols", buf);

			_itoa(OFFSET_ENABLED + i, buf, 10);
			pa->bIsEnabled = db_get_dw(0, "Protocols", buf, 1) != 0;
			if (!pa->bIsEnabled && !mir_strcmp(pa->szModuleName, META_PROTO)) {
				pa->bIsEnabled = true;
				db_set_dw(0, "Protocols", buf, 1);
			}
			pa->szProtoName = db_get_sa(0, szModuleName, "AM_BaseProto");
		}
		else pa->bIsEnabled = true;

		if (!pa->szProtoName) {
			pa->szProtoName = mir_strdup(szModuleName);
			db_set_s(0, szModuleName, "AM_BaseProto", pa->szProtoName);
		}

		if (!pa->tszAccountName)
			pa->tszAccountName = mir_a2u(szModuleName);
	}

	if (CheckProtocolOrder())
		WriteDbAccounts();

	int anum = accounts.getCount();
	db_enum_modules(EnumDbModules);
	if (anum != accounts.getCount())
		WriteDbAccounts();
}

/////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	int  arrlen;
	char **pszSettingName;
}
enumDB_ProtoProcParam;

static int enumDB_ProtoProc(const char* szSetting, void *lParam)
{
	if (szSetting) {
		enumDB_ProtoProcParam* p = (enumDB_ProtoProcParam*)lParam;

		p->arrlen++;
		p->pszSettingName = (char**)mir_realloc(p->pszSettingName, p->arrlen*sizeof(char*));
		p->pszSettingName[p->arrlen - 1] = mir_strdup(szSetting);
	}
	return 0;
}

void WriteDbAccounts()
{
	// enum all old settings to delete
	enumDB_ProtoProcParam param = { 0, nullptr };
	db_enum_settings(0, enumDB_ProtoProc, "Protocols", &param);

	// delete all settings
	if (param.arrlen) {
		for (int i = 0; i < param.arrlen; i++) {
			db_unset(0, "Protocols", param.pszSettingName[i]);
			mir_free(param.pszSettingName[i]);
		}
		mir_free(param.pszSettingName);
	}

	// write new data
	for (int i = 0; i < accounts.getCount(); i++) {
		PROTOACCOUNT *pa = accounts[i];

		char buf[20];
		_itoa(i, buf, 10);
		db_set_s(0, "Protocols", buf, pa->szModuleName);

		_itoa(OFFSET_PROTOPOS + i, buf, 10);
		db_set_dw(0, "Protocols", buf, pa->iOrder);

		_itoa(OFFSET_VISIBLE + i, buf, 10);
		db_set_dw(0, "Protocols", buf, pa->bIsVisible);

		_itoa(OFFSET_ENABLED + i, buf, 10);
		db_set_dw(0, "Protocols", buf, pa->bIsEnabled);

		_itoa(OFFSET_NAME + i, buf, 10);
		db_set_ws(0, "Protocols", buf, pa->tszAccountName);
	}

	db_unset(0, "Protocols", "ProtoCount");
	db_set_dw(0, "Protocols", "ProtoCount", accounts.getCount());
	db_set_dw(0, "Protocols", "PrVer", 4);
}

/////////////////////////////////////////////////////////////////////////////////////////

static int OnContactDeleted(WPARAM hContact, LPARAM lParam)
{
	if (hContact) {
		PROTOACCOUNT *pa = Proto_GetAccount(hContact);
		if (pa->IsEnabled() && pa->ppro)
			pa->ppro->OnEvent(EV_PROTO_ONCONTACTDELETED, hContact, lParam);
	}
	return 0;
}

static int OnDbSettingsChanged(WPARAM hContact, LPARAM lParam)
{
	if (hContact) {
		PROTOACCOUNT *pa = Proto_GetAccount(hContact);
		if (pa->IsEnabled() && pa->ppro)
			pa->ppro->OnEvent(EV_PROTO_DBSETTINGSCHANGED, hContact, lParam);
	}
	return 0;
}

static int InitializeStaticAccounts(WPARAM, LPARAM)
{
	int count = 0;

	for (auto &pa : accounts) {
		if (!pa->ppro || !pa->IsEnabled())
			continue;

		pa->ppro->OnEvent(EV_PROTO_ONLOAD, 0, 0);

		if (!pa->bOldProto)
			count++;
	}

	BuildProtoMenus();

	if (count == 0 && !db_get_b(0, "FirstRun", "AccManager", 0)) {
		db_set_b(0, "FirstRun", "AccManager", 1);
		CallService(MS_PROTO_SHOWACCMGR, 0, 0);
	}
	// This is for pack creators with a profile with predefined accounts
	else if (db_get_b(0, "FirstRun", "ForceShowAccManager", 0)) {
		CallService(MS_PROTO_SHOWACCMGR, 0, 0);
		db_unset(0, "FirstRun", "ForceShowAccManager");
	}
	return 0;
}

static int UninitializeStaticAccounts(WPARAM, LPARAM)
{
	// request permission to exit first
	for (auto &pa : accounts)
		if (pa->ppro && pa->IsEnabled())
			if (pa->ppro->OnEvent(EV_PROTO_ONREADYTOEXIT, 0, 0) != TRUE)
				return 1;

	// okay, all protocols are ready, exiting
	for (auto &pa : accounts)
		if (pa->ppro && pa->IsEnabled())
			pa->ppro->OnEvent(EV_PROTO_ONEXIT, 0, 0);

	return 0;
}

int LoadAccountsModule(void)
{
	bModuleInitialized = true;

	for (auto &pa : accounts) {
		pa->bDynDisabled = !Proto_IsProtocolLoaded(pa->szProtoName);
		if (pa->ppro)
			continue;

		if (!pa->IsEnabled())
			continue;

		if (!ActivateAccount(pa))
			pa->bDynDisabled = true;
	}

	hHooks[0] = HookEvent(ME_SYSTEM_MODULESLOADED, InitializeStaticAccounts);
	hHooks[1] = HookEvent(ME_SYSTEM_PRESHUTDOWN, UninitializeStaticAccounts);
	hHooks[2] = HookEvent(ME_DB_CONTACT_DELETED, OnContactDeleted);
	hHooks[3] = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, OnDbSettingsChanged);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

static HANDLE CreateProtoServiceEx(const char* szModule, const char* szService, MIRANDASERVICEOBJ pFunc, void* param)
{
	char tmp[100];
	mir_snprintf(tmp, "%s%s", szModule, szService);
	return CreateServiceFunctionObj(tmp, pFunc, param);
}

bool ActivateAccount(PROTOACCOUNT *pa)
{
	PROTOCOLDESCRIPTOR* ppd = Proto_IsProtocolLoaded(pa->szProtoName);
	if (ppd == nullptr)
		return false;

	if (ppd->fnInit == nullptr)
		return false;

	PROTO_INTERFACE *ppi = ppd->fnInit(pa->szModuleName, pa->tszAccountName);
	if (ppi == nullptr)
		return false;

	pa->ppro = ppi;
	if (ppi->m_hProtoIcon == nullptr)
		ppi->m_hProtoIcon = IcoLib_IsManaged(Skin_LoadProtoIcon(pa->szModuleName, ID_STATUS_ONLINE));
	ppi->m_iDesiredStatus = ppi->m_iStatus = ID_STATUS_OFFLINE;
	return true;
}

MIR_APP_DLL(int) Proto_GetAverageStatus(int *pAccountNumber)
{
	int netProtoCount = 0, averageMode = 0;

	for (auto &pa : accounts) {
		if (!pa->IsVisible() || pa->IsLocked())
			continue;

		netProtoCount++;
		if (averageMode == 0)
			averageMode = pa->iRealStatus;
		else if (averageMode > 0 && averageMode != pa->iRealStatus) {
			averageMode = -1;
			if (pAccountNumber == nullptr)
				break;
		}
	}

	if (pAccountNumber)
		*pAccountNumber = netProtoCount;
	return averageMode;
}

/////////////////////////////////////////////////////////////////////////////////////////

struct DeactivationThreadParam
{
	PROTO_INTERFACE *ppro;
	pfnUninitProto fnUninit;
	bool bIsDynamic, bErase;
};

pfnUninitProto GetProtocolDestructor(char *szProto);

static int DeactivationThread(DeactivationThreadParam* param)
{
	PROTO_INTERFACE *p = (PROTO_INTERFACE*)param->ppro;
	p->SetStatus(ID_STATUS_OFFLINE);

	char *szModuleName = NEWSTR_ALLOCA(p->m_szModuleName);

	if (param->bIsDynamic) {
		while (p->OnEvent(EV_PROTO_ONREADYTOEXIT, 0, 0) != TRUE)
			SleepEx(100, TRUE);

		p->OnEvent(EV_PROTO_ONEXIT, 0, 0);
	}

	KillObjectThreads(p); // waits for them before terminating
	KillObjectEventHooks(p); // untie an object from the outside world

	if (param->bErase)
		p->OnEvent(EV_PROTO_ONERASE, 0, 0);

	if (param->fnUninit)
		param->fnUninit(p);

	KillObjectServices(p);

	if (param->bErase)
		EraseAccount(szModuleName);

	delete param;
	return 0;
}

void DeactivateAccount(PROTOACCOUNT *pa, bool bIsDynamic, bool bErase)
{
	if (pa->ppro == nullptr) {
		if (bErase)
			EraseAccount(pa->szModuleName);
		return;
	}

	if (pa->hwndAccMgrUI) {
		DestroyWindow(pa->hwndAccMgrUI);
		pa->hwndAccMgrUI = nullptr;
		pa->bAccMgrUIChanged = FALSE;
	}

	DeactivationThreadParam *param = new DeactivationThreadParam;
	param->ppro = pa->ppro;
	param->fnUninit = GetProtocolDestructor(pa->szProtoName);
	param->bIsDynamic = bIsDynamic;
	param->bErase = bErase;
	pa->ppro = nullptr;
	if (bIsDynamic)
		mir_forkthread((pThreadFunc)DeactivationThread, param);
	else
		DeactivationThread(param);
}

/////////////////////////////////////////////////////////////////////////////////////////

void EraseAccount(const char *pszModuleName)
{
	// remove protocol contacts first
	for (MCONTACT hContact = db_find_first(pszModuleName); hContact != 0;) {
		MCONTACT hNext = db_find_next(hContact, pszModuleName);
		db_delete_contact(hContact);
		hContact = hNext;
	}

	// remove all protocol settings
	db_delete_module(0, pszModuleName);
}

/////////////////////////////////////////////////////////////////////////////////////////

void UnloadAccount(PROTOACCOUNT *pa, bool bIsDynamic, bool bErase)
{
	DeactivateAccount(pa, bIsDynamic, bErase);

	replaceStrW(pa->tszAccountName, 0);
	replaceStr(pa->szProtoName, 0);
	replaceStr(pa->szUniqueId, 0);

	// szModuleName should be freed only on a program's exit.
	// otherwise many plugins dependand on static protocol names will crash!
	// do NOT fix this 'leak', please
	if (!bIsDynamic) {
		mir_free(pa->szModuleName);
		mir_free(pa);
	}
}

void UnloadAccountsModule()
{
	if (!bModuleInitialized) return;

	auto T = accounts.rev_iter();
	for (auto &it : T) {
		UnloadAccount(it, false, false);
		accounts.remove(T.indexOf(&it));
	}
	accounts.destroy();

	for (auto &it : hHooks)
		UnhookEvent(it);
}
