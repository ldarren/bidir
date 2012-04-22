#pragma once
#ifndef _SCRIPT_MANAGER_H_
#define _SCRIPT_MANAGER_H_

#include "./script.h"

#include <lua.hpp>

struct ScriptNode
{
	Script *script;
	ScriptNode *pPrev;
	ScriptNode *pNext;
};

class ScriptMgr
{
public:
	static bool init();
	static bool deinit();

	static void addScript(Script *script);
	static void removeScript(Script *script);

	static void checkType(int idx);

	static void pushVar(int idx, const char *name, void *data);
	static void* popVar(int idx, int nameidx);
	static void pushType(int idx, const char *name, int type);
	static int popType(int idx, int nameidx);

	// re-omplemented lua standard function
	static bool registerClass(const char *type, Script::MethodList &methods, Script::PropertyList &properties);
	static bool findTable(int idx, const char *name, bool create=true, int size=0);
	static int	getMethod(const char *type, const char *funcName, int funcType);
	static int	getProperty(const char *type, const char *propName);

private:
	static int installScripts(lua_State *L);
	static int globalNewIndex(lua_State *L);
	static int globalIndex(lua_State *L);

protected:
	static lua_State	*pVM_;
	static ScriptNode	*pHead;
};

#endif // _SCRIPT_MANAGER_H_