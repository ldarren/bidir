#pragma once
#ifndef _SCRIPT_H_
#define _SCRIPT_H_

#include <lua.hpp>
#include <vector>
#include <map>

class ScriptMgr;

class Script
{
	friend class ScriptMgr;

public:
	static const size_t MAX_ERROR_LEN = 128;
	static const char * const KEY_VARIABLE_TABLE;
	static const char * const KEY_BASE_CLASS;
	static const char * const KEY_POINTER;
	static const char * const KEY_USERDATA;
	static const char * const KEY_LIB;
	static const char * const KEY_DESTRUCTOR;
	static const char * const KEY_INDEX;
	static const char * const KEY_NEW_INDEX;

	enum ScriptDataType
	{
		ESDT_Nil,
		ESDT_Int,
		ESDT_Num,
		ESDT_Str,
		ESDT_Ptr,
		ESDT_PtrInt,
		ESDT_PtrNum,
		ESDT_PtrStr,
	};

	struct ClassPropType
	{
		const char *name;
		ScriptDataType type;
		size_t offset;
	};

	typedef std::vector<luaL_Reg> MethodList;
	typedef std::vector<ClassPropType> PropertyList;

public:
	virtual void setup(lua_State *L);
	virtual void reset();
	virtual void uninstallScript() {}

	// administration
	void startClass(const char *type, lua_CFunction ctor, const char *btype=0);
	void endClass(lua_CFunction dtor=0);
	bool removeClass(const char *type);

	bool defFunc(const char *name, lua_CFunction func);
	bool undefFunc(const char *name);

	bool defVar(const char *name, int *var);
	bool defVar(const char *name, double *var);
	bool defVar(const char *name, char *var);
	bool defVar(const char *name, ScriptDataType type, size_t offset);
	bool undefVar(const char *name);

	// interfaces with lua
	int call(const char *func, const char *in_fmt, const char *out_fmt, ...);
	int invoke(void *obj, const char *func, const char *in_fmt, const char *out_fmt, ...);

	void getVar(const char *vname, int *out);
	void getVar(const char *vname, double *out);
	void getVar(const char *vname, char *out);
	void getVar(void *obj, const char *vname, int *out);
	void getVar(void *obj, const char *vname, double *out);
	void getVar(void *obj, const char *vname, char *out);

	void setVar(const char *vname, int in);
	void setVar(const char *vname, double in);
	void setVar(const char *vname, const char *in);
	void setVar(void *obj, const char *vname, int in);
	void setVar(void *obj, const char *vname, double in);
	void setVar(void *obj, const char *vname, const char *in);

	bool addObject(void *obj, const char *type);
	void* getObject(int idx, const char *type);
	void removeObject(void *obj);

	const char* lastError();

protected:
	Script();
	virtual ~Script();

	int callArg(bool isObj, const char *in_fmt, int in_argc, const char *out_fmt, int out_argc, va_list argv);

	lua_State *pVM_;

private:
	// mapping pointer to type (string), not using lua table to avoid 2048 limit
	typedef std::map<void*, const char*>	ObjectT;
	ObjectT		objects__;

	// count number of char in a string
	const char* getTypeT(const char *type);
	int strchrc(const char *str, const char c);
	void popError(const char *defmsg="Undefined error");

	static int set(lua_State *L);
	static int get(lua_State *L);

	char sError__[MAX_ERROR_LEN];
	static const char *sClassName__;
	static const char *sBaseName__;
	static MethodList mMethods__;
	static PropertyList mProperties__;
};

#endif // _SCRIPT_H_