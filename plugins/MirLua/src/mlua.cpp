#include "stdafx.h"

int hMLuaLangpack;

CMLua::CMLua()
	: PLUGIN(MODULE),
	L(nullptr),
	Scripts(1)
{
	MUUID muidLast = MIID_LAST;
	hMLuaLangpack = GetPluginLangId(muidLast, 0);

	CreatePluginService(MS_LUA_CALL, &CMLua::Call);
	CreatePluginService(MS_LUA_EXEC, &CMLua::Exec);
	CreatePluginService(MS_LUA_EVAL, &CMLua::Eval);
}

CMLua::~CMLua()
{
	Unload();
}

void CMLua::Load()
{
	Log("Loading lua engine");
	L = luaL_newstate();
	Log("Loading standard modules");
	luaL_openlibs(L);

	lua_atpanic(L, luaM_atpanic);

	CMLuaFunctionLoader::Load(L);
	CMLuaModuleLoader::Load(L);
	CMLuaScriptLoader::Load(L);
}

void CMLua::Unload()
{
	Log("Unloading lua engine");

	Scripts.destroy();

	KillModuleIcons(hMLuaLangpack);
	KillModuleSounds(hMLuaLangpack);
	KillModuleMenus(hMLuaLangpack);
	KillModuleHotkeys(hMLuaLangpack);

	KillObjectEventHooks(L);
	KillObjectServices(L);

	lua_close(L);
}

void CMLua::Reload()
{
	Unload();
	Load();
}

/***********************************************/

static int mlua_call(lua_State *L)
{
	const char *module = luaL_checkstring(L, -3);
	const char *function = luaL_checkstring(L, -2);

	if (module && module[0]) {
		lua_getglobal(L, "require");
		lua_pushstring(L, module);
		lua_pcall(L, 1, 1, 0);

		lua_getfield(L, -1, function);
		lua_replace(L, -2);
	}
	else
		lua_getglobal(L, function);

	lua_pcall(L, 0, 1, 0);

	return 1;
}

INT_PTR CMLua::Call(WPARAM wParam, LPARAM lParam)
{
	const wchar_t *module = (const wchar_t*)wParam;
	const wchar_t *function = (const wchar_t*)lParam;

	lua_pushstring(L, ptrA(mir_utf8encodeW(module)));
	lua_pushstring(L, ptrA(mir_utf8encodeW(function)));

	lua_newtable(L);
	lua_pushcclosure(L, mlua_call, 1);

	CMLuaEnvironment env(L);
	env.Load();

	wchar_t *result = mir_utf8decodeW(lua_tostring(L, -1));
	lua_pop(L, 1);

	return (INT_PTR)result;
}

INT_PTR CMLua::Eval(WPARAM, LPARAM lParam)
{
	const wchar_t *script = (const wchar_t*)lParam;

	if (luaL_loadstring(L, ptrA(mir_utf8encodeW(script)))) {
		ReportError(L);
		return NULL;
	}

	CMLuaEnvironment env(L);
	env.Load();

	wchar_t *result = mir_utf8decodeW(lua_tostring(L, -1));
	lua_pop(L, 1);

	return (INT_PTR)result;
}

INT_PTR CMLua::Exec(WPARAM, LPARAM lParam)
{
	const wchar_t *path = (const wchar_t*)lParam;

	if (luaL_loadfile(L, ptrA(mir_utf8encodeW(path)))) {
		ReportError(L);
		return NULL;
	}

	CMLuaEnvironment env(L);
	env.Load();

	wchar_t *result = mir_utf8decodeW(lua_tostring(L, -1));
	lua_pop(L, 1);

	return (INT_PTR)result;
}
