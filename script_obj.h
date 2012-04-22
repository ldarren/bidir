#pragma once
#ifndef _SCRIPT_OBJECT_H_
#define _SCRIPT_OBJECT_H_

#include "./script.h"
#include "./script_mgr.h"

#include <stddef.h>

template <typename T>
struct ScriptObj : public Script
{
	ScriptObj()
	{
		ScriptMgr::addScript(this);
	}
	virtual ~ScriptObj()
	{
		ScriptMgr::removeScript(this);
		pVM_ = 0;
	}

	virtual void setup(lua_State *L)
	{
		Script::setup(L);
		T::installScript(this);
	}

	void uninstallScript()
	{
		T::uninstallScript(this);
	}

	virtual void reset()
	{
		Script::reset();
	}

	// helper functions
	int popInt(int idx)
	{
		return (int)lua_tointeger(pVM_, idx);
	}

	void pushInt(int val)
	{
		lua_pushinteger(pVM_, val);
	}

	double popNum(int idx)
	{
		return lua_tonumber(pVM_, idx);
	}

	void pushNum(double val)
	{
		lua_pushnumber(pVM_, val);
	}

	const char* popStr(int idx)
	{
		return lua_tostring(pVM_, idx);
	}

	void pushStr(const char *val)
	{
		lua_pushstring(pVM_, val);
	}

	bool popBool(int idx)
	{
		return lua_toboolean(pVM_, idx)==1;
	}

	void pushBool(bool val)
	{
		lua_pushboolean(pVM_, val);
	}
};

#endif // _SCRIPT_OBJECT_H_