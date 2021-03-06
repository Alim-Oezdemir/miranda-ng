/*
IRC plugin for Miranda IM

Copyright (C) 2003-05 Jurgen Persson
Copyright (C) 2007-09 George Hazan

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
#include "version.h"

CMPlugin g_plugin;
CHAT_MANAGER *pci;
CLIST_INTERFACE *pcli;
HINSTANCE hInst = nullptr;

int hLangpack;

static int CompareServers( const SERVER_INFO* p1, const SERVER_INFO* p2 )
{
	return mir_strcmp( p1->m_name, p2->m_name );
}

OBJLIST<SERVER_INFO> g_servers( 20, CompareServers );

void UninitTimers( void );

// Information about the plugin
PLUGININFOEX pluginInfo =
{
	sizeof( PLUGININFOEX ),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESC,
	__AUTHOR,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
    {0x92382b4d, 0x5572, 0x48a0, {0xb0, 0xb9, 0x13, 0x36, 0xa6, 0x1, 0xd6, 0x89}} // {92382B4D-5572-48a0-B0B9-1336A601D689}
};

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD, LPVOID)
{
	hInst = hinstDLL;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD)
{
	return &pluginInfo;
}

extern "C" __declspec(dllexport) const MUUID MirandaInterfaces[] = { MIID_PROTOCOL, MIID_LAST };

/////////////////////////////////////////////////////////////////////////////////////////

extern "C" int __declspec(dllexport) Load()
{
	mir_getLP(&pluginInfo);
	pci = Chat_GetInterface();
	pcli = Clist_GetInterface();

	InitIcons();
	InitServers();
	InitContactMenus();
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

extern "C" int __declspec(dllexport) Unload(void)
{
	UninitContactMenus();
	UninitTimers();
	return 0;
}
