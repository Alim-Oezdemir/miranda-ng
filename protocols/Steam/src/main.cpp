#include "stdafx.h"

int hLangpack;
HINSTANCE g_hInstance;
CMPlugin g_plugin;

HANDLE hExtraXStatus;

PLUGININFOEX pluginInfo =
{
	sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESCRIPTION,
	__AUTHOR,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	// {68F5A030-BA32-48EC-9507-5C2FBDEA5217}
	{ 0x68f5a030, 0xba32, 0x48ec, { 0x95, 0x7, 0x5c, 0x2f, 0xbd, 0xea, 0x52, 0x17 }}
};

DWORD WINAPI DllMain(HINSTANCE hInstance, DWORD, LPVOID)
{
	g_hInstance = hInstance;

	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD)
{
	return &pluginInfo;
}

extern "C" __declspec(dllexport) const MUUID MirandaInterfaces[] = { MIID_PROTOCOL, MIID_LAST };

extern "C" int __declspec(dllexport) Load(void)
{
	mir_getLP(&pluginInfo);

	char iconName[100];
	mir_snprintf(iconName, "%s_%s", MODULE, "gaming");

	// extra statuses
	HookEvent(ME_SKIN2_ICONSCHANGED, OnReloadIcons);
	hExtraXStatus = ExtraIcon_RegisterIcolib("steam_game", LPGEN("Steam game"), iconName);

	CSteamProto::InitMenus();
	return 0;
}

extern "C" int __declspec(dllexport) Unload(void)
{
	return 0;
}
