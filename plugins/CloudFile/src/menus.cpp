#include "stdafx.h"

HGENMENU hContactMenu;

static INT_PTR UploadMenuCommand(void *obj, WPARAM hContact, LPARAM)
{
	CCloudService *service = (CCloudService*)obj;
	service->OpenUploadDialog(hContact);
	return 0;
}

void InitializeMenus()
{
	CMenuItem mi;
	SET_UID(mi, 0x93d4495b, 0x259b, 0x4fba, 0xbc, 0x14, 0xf9, 0x46, 0x2c, 0xda, 0xfc, 0x6d);
	mi.name.a = LPGEN("Upload to...");

	ptrA defaultService(db_get_sa(NULL, MODULE, "DefaultService"));
	if (defaultService) {
		CCloudService *service = FindService(defaultService);
		if (service) {
			mi.name.a = LPGEN("Upload");
			mi.pszService = MODULE "/Default/Upload";
			CreateServiceFunctionObj(mi.pszService, UploadMenuCommand, service);
		}
	}

	mi.position = -2000019999;
	mi.hIcon = LoadIconEx(IDI_UPLOAD);
	hContactMenu = Menu_AddContactMenuItem(&mi);
}

void AddServiceMenuItem(const CCloudService *service)
{
	CMenuItem mi;
	mi.root = hContactMenu;
	CMStringA serviceName(FORMAT, "/%s/Upload", service->GetAccountName());
	mi.pszService = serviceName.GetBuffer();
	mi.hLangpack = service->GetId();
	mi.flags = CMIF_SYSTEM | CMIF_UNICODE;
	mi.name.w = (wchar_t*)service->GetUserName();
	mi.position = Services.getCount();
	mi.hIcolibItem = GetIconHandle(service->GetIconId());
	Menu_AddContactMenuItem(&mi);
	CreateServiceFunctionObj(mi.pszService, UploadMenuCommand, (void*)service);
}

int OnPrebuildContactMenu(WPARAM hContact, LPARAM)
{
	Menu_ShowItem(hContactMenu, CanSendToContact(hContact));
	return 0;
}