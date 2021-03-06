#ifndef _LUA_SCRIPT_H_
#define _LUA_SCRIPT_H_

class CMLuaScript : public CMLuaEnvironment
{
public:
	enum Status
	{
		None,
		Loaded,
		Failed
	};

private:
	Status status;
	int unloadRef;

	char *moduleName;
	wchar_t *fileName;
	wchar_t filePath[MAX_PATH];

	void Unload();

public:
	CMLuaScript(lua_State *L, const wchar_t *path);
	CMLuaScript(const CMLuaScript &script);
	~CMLuaScript();

	const wchar_t* GetFilePath() const;
	const wchar_t* GetFileName() const;

	bool IsEnabled();
	void Enable();
	void Disable();

	Status GetStatus() const;

	bool Load();
	bool Reload();
};

#endif //_LUA_SCRIPT_H_
