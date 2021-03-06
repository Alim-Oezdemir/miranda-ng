#include "stdafx.h"

HINSTANCE hInst;
int hLangpack;
HANDLE hRestartMe;

PLUGININFOEX pluginInfo={
	sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESCRIPTION,
	__AUTHOR,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	// {61BEDF3A-0CC2-41A3-B980-BB9393368935}
	{0x61bedf3a, 0xcc2, 0x41a3, {0xb9, 0x80, 0xbb, 0x93, 0x93, 0x36, 0x89, 0x35}}
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD, LPVOID)
{
	hInst = hinstDLL;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD)
{
	return &pluginInfo;
}

static INT_PTR RestartMe(WPARAM, LPARAM)
{
	CallService(MS_SYSTEM_RESTART, 1, 0);
	return 0;
}

static IconItem icon = { LPGEN("Restart"), "rst_restart_icon", IDI_RESTARTICON };

extern "C" __declspec(dllexport) int Load(void)
{
	mir_getLP( &pluginInfo );

	// IcoLib support
	Icon_Register(hInst, LPGEN("Restart Plugin"), &icon, 1);

	hRestartMe = CreateServiceFunction("System/RestartMe", RestartMe);

	CMenuItem mi;
	SET_UID(mi, 0x9181059, 0x5316, 0x4be3, 0x96, 0xb7, 0x80, 0x51, 0xa9, 0x3a, 0xd8, 0x49);
	mi.position = -0x7FFFFFFF;
	mi.hIcolibItem = icon.hIcolib;
	mi.name.a = LPGEN("Restart");
	mi.pszService = "System/RestartMe";
	Menu_AddMainMenuItem(&mi);
	Menu_AddTrayMenuItem(&mi);
	return 0;
}

extern "C" __declspec(dllexport) int Unload(void)
{
	return 0;
}
