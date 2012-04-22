#include "./script.h"
#include "./core.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

const char * const Script::KEY_VARIABLE_TABLE = "var_t_";
const char * const Script::KEY_BASE_CLASS = "base_t_";
const char * const Script::KEY_POINTER = "ptr_p_";
const char * const Script::KEY_USERDATA = "udata";
const char * const Script::KEY_LIB = "glo_t_";
const char * const Script::KEY_DESTRUCTOR = "__gc";
const char * const Script::KEY_INDEX = "__index";
const char * const Script::KEY_NEW_INDEX = "__newindex";

const char* Script::sClassName__ = 0;
const char* Script::sBaseName__ = 0;
Script::MethodList Script::mMethods__;
Script::PropertyList Script::mProperties__;

Script::Script()
: pVM_(0)
{
	mMethods__.reserve(10);
	mProperties__.reserve(20);
}

Script::~Script()
{
}

void Script::setup(lua_State *L)
{
	pVM_ = L;
}

void Script::reset()
{
	pVM_ = 0;
}

bool Script::defFunc(const char *name, lua_CFunction func)
{
	if (sClassName__)
	{// class methods
		luaL_Reg reg = {name, func};
		mMethods__.push_back(reg);
	}
	else
	{// global function
		lua_pushstring(pVM_, name);
		lua_pushcclosure(pVM_, func, 0);
		lua_rawset(pVM_, LUA_GLOBALSINDEX);
	}
	return true;
}

bool Script::undefFunc(const char *name)
{
	if (sClassName__)
	{// class methods
	}
	else
	{// global function
		lua_pushstring(pVM_, name);
		lua_pushnil(pVM_);
		lua_rawset(pVM_, LUA_GLOBALSINDEX);
	}
	return true;
}

bool Script::defVar(const char *name, int *var)
{
	if (!ScriptMgr::findTable(LUA_ENVIRONINDEX, Script::KEY_VARIABLE_TABLE))
		return false;
	ScriptMgr::pushVar(-1, name, var);
	ScriptMgr::pushType(-1, name, Script::ESDT_Int);
	lua_pop(pVM_, 1);

	return true;
}

bool Script::defVar(const char *name, double *var)
{
	if (!ScriptMgr::findTable(LUA_ENVIRONINDEX, Script::KEY_VARIABLE_TABLE))
		return false;
	ScriptMgr::pushVar(-1, name, var);
	ScriptMgr::pushType(-1, name, Script::ESDT_Num);
	lua_pop(pVM_, 1);

	return true;
}

bool Script::defVar(const char *name, char *var)
{
	if (!ScriptMgr::findTable(LUA_ENVIRONINDEX, Script::KEY_VARIABLE_TABLE))
		return false;
	ScriptMgr::pushVar(-1, name, var);
	ScriptMgr::pushType(-1, name, Script::ESDT_Str);
	lua_pop(pVM_, 1);

	return true;
}

bool Script::defVar(const char *name, ScriptDataType type, size_t offset)
{
	assert(	sClassName__ );
	
	ClassPropType prop = {name, type, offset};
	mProperties__.push_back(prop);

	return true;
}

bool Script::undefVar(const char *name)
{
	return true;
}

void Script::startClass(const char *type, lua_CFunction ctor, const char *btype)
{
	sClassName__ = type;
	sBaseName__ = btype;

	luaL_Reg reg = {"Create", ctor};
	mMethods__.push_back(reg);
}

void Script::endClass(lua_CFunction dtor)
{
	assert(sClassName__);

	luaL_Reg reg = {"Destroy", dtor};
	mMethods__.push_back(reg);

	// 2 purposes here,
	// 1) create or load a module in Globals[ClassName]
	// 2) set the module as loaded by letting Register[_LOADED][ClassName] point to it
	if (!ScriptMgr::registerClass(sClassName__, mMethods__, mProperties__))
	{
		goto ln_Script_endClass_clear;
	}
	// at this point, -1 = Globals[ClassName] and Registry[_LOADED][ClassName] == Globals[ClassName]

	// create a metatable template in Register[ClassName]
	lua_createtable(pVM_, 0, 4);  /* create metatable. __index, __newindex, dtor, base */
	lua_pushstring(pVM_, sClassName__);
	lua_pushvalue(pVM_, -2);
	lua_rawset(pVM_, LUA_REGISTRYINDEX);  /* registry[ClassName] = metatable */
	// at this point, -1 = Registry[ClassName] -2 = Globals[ClassName]

	lua_pushstring(pVM_, Script::KEY_LIB);
	lua_pushvalue(pVM_, -3);
	lua_rawset(pVM_, -3); // Registry[ClasName][lib_t_]=Globals[ClassName];
	
	// add more detail to the metatable template
	lua_pushstring(pVM_, Script::KEY_NEW_INDEX);
		lua_pushvalue(pVM_, -3); // duplicate Globals[ClassName]
		lua_pushcclosure(pVM_, &set, 1); // upvalue for get func is Globals[ClassName]
	lua_rawset(pVM_, -3); // Registry[ClassNAme][__newindex] = set

	lua_pushstring(pVM_, Script::KEY_INDEX);
		lua_pushvalue(pVM_, -3); // duplicate Globals[ClassName]
		lua_pushcclosure(pVM_, &get, 1); // upvalue for get func is Globals[ClassName]
	lua_rawset(pVM_, -3); // Registry[ClassNAme][__index] = get

	lua_pushstring(pVM_, Script::KEY_DESTRUCTOR);
	lua_pushcclosure(pVM_, dtor, 0);
	lua_rawset(pVM_, -3); // Registry[ClassNAme][dtor_f_] = dtor

	if (sBaseName__ !=0 && strcmp(sBaseName__, "")) // base class defined
	{
		// find base template metatable in registry
		lua_pushstring(pVM_, Script::KEY_BASE_CLASS);
			lua_pushstring(pVM_, sBaseName__);
			lua_rawget(pVM_, LUA_REGISTRYINDEX);
		lua_rawset(pVM_, -3); // Registry[ClassName][base_c_] = Registry[BaseName]

		lua_createtable(pVM_, 0, 1); // create a meta table for Globals[ClassName]
		lua_pushstring(pVM_, Script::KEY_INDEX);
			lua_pushstring(pVM_, sBaseName__); // get Globals[BaseNAme]
			lua_rawget(pVM_, LUA_GLOBALSINDEX);
		lua_rawset(pVM_, -3); // metatable[__index] = Globals[BaseName]
		lua_setmetatable(pVM_, -3);	// Globals[ClassName][__index] = metatable
	}

	lua_pop(pVM_, 2); // pop away Registry[ClassName] and Globals[ClassName]

ln_Script_endClass_clear:
	mMethods__.clear();
	mProperties__.clear();
	sClassName__ = 0;
	sBaseName__ = 0;
}

bool Script::removeClass(const char *type)
{
	return true;
}

int Script::strchrc(const char *str, const char c)
{
	size_t count = strlen(str);
	int len = 0;
	for(size_t i=0; i<count; ++i)
		if ('%'==str[i]) 
			++len;
	return len;
}

int Script::call(const char *func, const char *in_fmt, const char *out_fmt, ...)
{
	lua_getfield(pVM_, LUA_GLOBALSINDEX, func);
	if (lua_isnoneornil(pVM_, -1)) 
	{
		lua_pop(pVM_, 1);
		return 0;
	}

	int in_argc = strchrc(in_fmt, '%');
	int out_argc = strchrc(out_fmt, '%');

	va_list vl;
	va_start(vl, out_fmt);
	int ret = callArg(false, in_fmt, in_argc, out_fmt, out_argc, vl);
	va_end(vl);

	lua_pop(pVM_, 1);
	return ret;
}

int Script::invoke(void *obj, const char *func, const char *in_fmt, const char *out_fmt, ...)
{
	const char *type = objects__[obj];
	const char *type_t = getTypeT(type);

	if (!ScriptMgr::getMethod(type, func, LUA_TFUNCTION))
		return 0;

	lua_remove(pVM_, -2); // remove obj-globals table
	lua_getfield(pVM_, LUA_REGISTRYINDEX, type_t); // get obj-globals table
	lua_pushlightuserdata(pVM_, obj);
	lua_rawget(pVM_, -2);
	lua_remove(pVM_, -2); // remove obj-globals table

	int in_argc = strchrc(in_fmt, '%');
	int out_argc = strchrc(out_fmt, '%');

	va_list vl;
	va_start(vl, out_fmt);
	int ret = callArg(true, in_fmt, in_argc, out_fmt, out_argc, vl);
	va_end(vl);

	lua_pop(pVM_, 2);
	return ret;
}

// assume lua functon being pushed to local stack -1 pos b4 calling this function
int Script::callArg(bool isObj, const char *in_fmt, int in_argc, const char *out_fmt, int out_argc, va_list argv)
{
	char *remain=(char*)in_fmt;
	int i;
	for (i=0; i<in_argc; ++i)
	{
		remain = strchr(remain, '%');
		++remain;
		switch(remain[0])
		{
		case 'b':
			lua_pushinteger(pVM_, va_arg(argv, bool));
			break;
		case 'c':
			lua_pushinteger(pVM_, va_arg(argv, char));
			break;
		case 'w':
			lua_pushinteger(pVM_, va_arg(argv, short));
			break;
		case 'i':
			lua_pushinteger(pVM_, va_arg(argv, int));
			break;
		case 'l':
			lua_pushinteger(pVM_, va_arg(argv, long));
			break;
		case 'f':
			lua_pushnumber(pVM_, va_arg(argv, float));
			break;
		case 'd':
			lua_pushnumber(pVM_, va_arg(argv, double));
			break;
		case 's':
			lua_pushstring(pVM_, va_arg(argv, const char*));
			break;
		}
	}

	int ret = lua_pcall(pVM_, isObj?++in_argc:in_argc, out_argc, 0);
	if (ret > 1)
	{
		popError();
		return ret;
	}

	remain=(char*)out_fmt;
	int idx = -out_argc;
	for (i=0; i<out_argc; ++i, ++idx)
	{
		remain = strchr(remain, '%');
		++remain;
		switch(remain[0])
		{
		case 'b':
			{
				bool *v = va_arg(argv, bool*);
				*v = lua_toboolean(pVM_, idx)!=0;
			}
			break;
		case 'c':
			{
				char *v = va_arg(argv, char*);
				*v = (char)lua_tointeger(pVM_, idx);
			}
			break;
		case 'w':
			{
				short *v = va_arg(argv, short*);
				*v = (short)lua_tointeger(pVM_, idx);
			}
			break;
		case 'i':
			{
				int *v = va_arg(argv, int*);
				*v = (int)lua_tointeger(pVM_, idx);
			}
			break;
		case 'l':
			{
				long *v = va_arg(argv, long*);
				*v = (long)lua_tointeger(pVM_, idx);
			}
			break;
		case 'f':
			{
				float *v = va_arg(argv, float*);
				*v = (float)lua_tonumber(pVM_, idx);
			}
			break;
		case 'd':
			{
				double *v = va_arg(argv, double*);
				*v = (double)lua_tonumber(pVM_, idx);
			}
			break;
		case 's':
			{
				char *s = va_arg(argv, char*);
				strcpy(s, lua_tostring(pVM_, idx));
			}
			break;
		}
	}

	return out_argc;
}

void Script::getVar(const char *vname, int *out)
{
	lua_pushstring(pVM_, vname);
	lua_rawget(pVM_, LUA_GLOBALSINDEX);
	*out = (int)lua_tointeger(pVM_, -1);
}

void Script::getVar(const char *vname, double *out)
{
	lua_pushstring(pVM_, vname);
	lua_rawget(pVM_, LUA_GLOBALSINDEX);
	*out = lua_tonumber(pVM_, -1);
}

void Script::getVar(const char *vname, char *out)
{
	lua_pushstring(pVM_, vname);
	lua_rawget(pVM_, LUA_GLOBALSINDEX);
	strcpy(out, lua_isstring(pVM_, -1) ? lua_tostring(pVM_, -1) : "");
}

void Script::getVar(void *obj, const char *vname, int *out)
{
	const char *type = objects__[obj];
	if (1 == ScriptMgr::getMethod(type, vname, LUA_TNUMBER))
	{
		*out = (int)lua_tointeger(pVM_, -1);
		lua_pop(pVM_, 2);
		return;
	}

	int offset = ScriptMgr::getProperty(type, vname);
	if (offset > 0)
		*out = *(int*)((char*)obj + offset);
}

void Script::getVar(void *obj, const char *vname, double *out)
{
	const char *type = objects__[obj];
	if (1 == ScriptMgr::getMethod(type, vname, LUA_TNUMBER))
	{
		*out = lua_tonumber(pVM_, -1);
		lua_pop(pVM_, 2);
		return;
	}

	int offset = ScriptMgr::getProperty(type, vname);
	if (offset > 0)
		*out = *(lua_Number*)((char*)obj + offset);
}

void Script::getVar(void *obj, const char *vname, char *out)
{
	const char *type = objects__[obj];
	if (1 == ScriptMgr::getMethod(type, vname, LUA_TNUMBER))
	{
		strcpy(out, lua_tostring(pVM_, -1));
		lua_pop(pVM_, 2);
		return;
	}

	int offset = ScriptMgr::getProperty(type, vname);
	if (offset > 0)
		strcpy(out, (const char*)((char*)obj + offset));
}

void Script::setVar(const char *vname, int in)
{
	lua_pushstring(pVM_, vname);
	lua_pushinteger(pVM_, in);
	lua_rawset(pVM_, LUA_GLOBALSINDEX);
}

void Script::setVar(const char *vname, double in)
{
	lua_pushstring(pVM_, vname);
	lua_pushnumber(pVM_, in);
	lua_rawset(pVM_, LUA_GLOBALSINDEX);
}

void Script::setVar(const char *vname, const char *in)
{
	lua_pushstring(pVM_, vname);
	lua_pushstring(pVM_, in);
	lua_rawset(pVM_, LUA_GLOBALSINDEX);
}

void Script::setVar(void *obj, const char *vname, int in)
{
	const char *type = objects__[obj];
	if (1 == ScriptMgr::getMethod(type, vname, LUA_TNUMBER))
	{
		lua_pushinteger(pVM_, in);
		lua_setfield(pVM_, -3, vname);
		lua_pop(pVM_, 2);
		return;
	}

	int offset = ScriptMgr::getProperty(type, vname);
	if (offset > 0)
		*(lua_Integer*)((char*)obj + offset) = in;
}

void Script::setVar(void *obj, const char *vname, double in)
{
	const char *type = objects__[obj];
	if (1 == ScriptMgr::getMethod(type, vname, LUA_TNUMBER))
	{
		lua_pushnumber(pVM_, in);
		lua_setfield(pVM_, -3, vname);
		lua_pop(pVM_, 2);
		return;
	}

	int offset = ScriptMgr::getProperty(type, vname);
	if (offset > 0)
		*(lua_Number*)((char*)obj + offset) = in;
}

void Script::setVar(void *obj, const char *vname, const char *in)
{
	const char *type = objects__[obj];
	if (1 == ScriptMgr::getMethod(type, vname, LUA_TSTRING))
	{
		lua_pushstring(pVM_, in);
		lua_setfield(pVM_, -3, vname);
		lua_pop(pVM_, 2);
		return;
	}

	int offset = ScriptMgr::getProperty(type, vname);
	if (offset > 0)
		strcpy(((char*)obj + offset), in);
}

// leave a new table in stack
bool Script::addObject(void *val, const char *type) 
{
	const char *type_t = getTypeT(type);

	lua_getfield(pVM_, LUA_REGISTRYINDEX, type_t);
	if (lua_type(pVM_, -1) != LUA_TTABLE)
	{
		lua_pop(pVM_, 1); // remove the nil

		lua_createtable(pVM_, 0, 0);
		lua_pushstring(pVM_, "kv");
		lua_setfield(pVM_, -2, "__mode");	// weak table for gc, pop value
		lua_pushvalue(pVM_, -1);			// __mode only work in metatable
		lua_setmetatable(pVM_, -2);

		lua_pushvalue(pVM_, -1);
		lua_setfield(pVM_, LUA_REGISTRYINDEX, type_t);
	}

	lua_pushlightuserdata(pVM_, val);
	lua_rawget(pVM_, -2);
	if (lua_isnil(pVM_, -1))
	{
		lua_pop(pVM_, 1); // remove the nil

		lua_createtable(pVM_, 0, 0);

		lua_pushlightuserdata(pVM_, val);
		lua_setfield(pVM_, -2, Script::KEY_POINTER);

		// TODO: merge this with pointer?
		// right now userdata can't reference, it is nil value
		// the only purpose of KEY_USERDATA is for destructor
		void **ubox=(void**)lua_newuserdata(pVM_, sizeof(void*));
		*ubox = val;
		lua_getfield(pVM_, LUA_REGISTRYINDEX, type);
		lua_setmetatable(pVM_, -2);
		lua_setfield(pVM_, -2, Script::KEY_USERDATA);

		lua_getfield(pVM_, LUA_REGISTRYINDEX, type);
		lua_setmetatable(pVM_, -2);

		lua_pushlightuserdata(pVM_, val); // key, object address
		lua_pushvalue(pVM_, -2); // value, table object
		lua_rawset(pVM_, -4);

		objects__[val] = type;

		lua_remove(pVM_, -2);
		return true;
	}
	lua_remove(pVM_, -2);
	return false;
}

// retrive c++ object base on lua table
void* Script::getObject(int idx, const char *type)
{
	lua_getfield(pVM_, LUA_REGISTRYINDEX, type);
	lua_getmetatable(pVM_, idx); // get the metatable of userdata

	while (lua_istable(pVM_, -1))
	{
		if (lua_rawequal(pVM_, -1, -2))
		{
			lua_pop(pVM_, 2);
			lua_getfield(pVM_, idx, Script::KEY_POINTER);
			return lua_touserdata(pVM_, -1);
		}
		lua_getfield(pVM_, -1, Script::KEY_BASE_CLASS);
		lua_replace(pVM_, -2);
	}

	luaL_typerror(pVM_, idx, type);
	return 0;
}

void Script::removeObject(void *obj)
{
	const char *type = objects__[obj];
	if (!type) return;
	const char *type_t = getTypeT(type);

	lua_getfield(pVM_, LUA_REGISTRYINDEX, type_t); // get the obj-table table

	lua_pushlightuserdata(pVM_, obj);
	lua_pushnil(pVM_);
	lua_rawset(pVM_, -3);

	lua_pop(pVM_, 1); // pop the obj-table table

	objects__.erase(obj);
}

const char* Script::lastError() 
{
	static char error[MAX_ERROR_LEN];
	strcpy(error, sError__);
	strcpy(sError__, "No error"); // clear error msg
	return error; 
}

void Script::popError(const char *defmsg)
{
	const char *msg = lua_tostring(pVM_, 128);
	strcpy(sError__, msg==0?defmsg:msg);
}

// 1|-3:table, 2|-2: key, 3|-1: value
int Script::set(lua_State *L)
{
	// not suppose to overwrite anything, therefore dun need to search
	// for existing item
	//lua_pushvalue(L, 2);
	//lua_gettable(L, lua_upvalueindex(1));
	//if (!lua_isnoneornil(L, -1))
	//{
	//	// set value
	//	return 0;
	//}

	lua_pushstring(L, Script::KEY_VARIABLE_TABLE);
	lua_rawget(L, lua_upvalueindex(1)); // get var-table out of globals[class] (sure get)

	while (lua_istable(L, -1)) 
	{
		lua_pushvalue(L, 2);
		lua_rawget(L, -2); // get variable in var-table

		if (lua_isnumber(L, -1))
		{
			int offset = (int)lua_tointeger(L, -1);

			lua_pushfstring(L, "%s_t_", lua_tostring(L, 2));
			lua_rawget(L, -3);
			int type = (int)lua_tointeger(L, -1);

			lua_pushstring(L, Script::KEY_POINTER);
			lua_rawget(L, 1);
			void *obj = lua_touserdata(L, -1);

			switch(type)
			{
			case Script::ESDT_Int:
				*(lua_Integer*)((char*)obj + offset) = lua_tointeger(L, 3);
				break;
			case Script::ESDT_Num:
				*(lua_Number*)((char*)obj + offset) = lua_tonumber(L, 3);
				break;
			case Script::ESDT_Str:
				strcpy(((char*)obj + offset), lua_tostring(L, 3));
				break;
			case Script::ESDT_Ptr:
				*(int*)((char*)obj + offset) = *(int*)(lua_touserdata(L, 3));
				break;
			case Script::ESDT_PtrInt:
				**(lua_Integer**)((char*)obj + offset) = lua_tointeger(L, 3);
				break;
			case Script::ESDT_PtrNum:
				**(lua_Number**)((char*)obj + offset) = lua_tonumber(L, 3);
				break;
			case Script::ESDT_PtrStr:
				strcpy(*(char**)((char*)obj + offset), lua_tostring(L, 3));
				break;
			}

			return 0;
		}
		else
		{
			lua_pop(L, 2);

			if (!lua_getmetatable(L, -1)) break;

			lua_pushstring(L, Script::KEY_INDEX);
			lua_rawget(L, -2);

			if (!lua_istable(L, -1)) break;

			lua_pushstring(L, Script::KEY_VARIABLE_TABLE);
			lua_rawget(L, -2);
		}
	}

	lua_pop(L, 3);

	lua_rawset(L, 1);

	return 0;
}

// 1|-2: table, 2|-1: key, upvalue(1): Globals[class] table
int Script::get(lua_State *L)
{
	// use getfield to trigger Globals[Class] metatable
	lua_pushvalue(L, 2);
	lua_gettable(L, lua_upvalueindex(1));
	if (!lua_isnoneornil(L, -1))
		return 1;

	lua_pushvalue(L, lua_upvalueindex(1)); // Globals[class]

	while (lua_istable(L, -1)) 
	{
		lua_pushstring(L, Script::KEY_VARIABLE_TABLE);
		lua_rawget(L, -2); // get var-table out of globals[class] (sure get)

		if (!lua_istable(L, -1)) break;

		lua_pushvalue(L, 2);
		lua_rawget(L, -2); // get variable in var-table

		if (lua_isnumber(L, -1))
		{
			int offset = (int)lua_tointeger(L, -1);

			lua_pushfstring(L, "%s_t_", lua_tostring(L, 2));
			lua_rawget(L, -3);
			int type = (int)lua_tointeger(L, -1);

			lua_pushstring(L, Script::KEY_POINTER);
			lua_rawget(L, 1);
			void *obj = lua_touserdata(L, -1);

			switch(type)
			{
			case Script::ESDT_Int:
				lua_pushinteger(L, *(lua_Integer*)((char*)obj + offset));
				break;
			case Script::ESDT_Num:
				lua_pushnumber(L, *(lua_Number*)((char*)obj + offset));
				break;
			case Script::ESDT_Str:
				lua_pushstring(L, (const char*)((char*)obj + offset));
				break;
			case Script::ESDT_Ptr:
				lua_pushlightuserdata(L, (void*)((char*)obj + offset));
				break;
			case Script::ESDT_PtrInt:
				lua_pushinteger(L, **(lua_Integer**)((char*)obj + offset));
				break;
			case Script::ESDT_PtrNum:
				lua_pushnumber(L, **(lua_Number**)((char*)obj + offset));
				break;
			case Script::ESDT_PtrStr:
				lua_pushstring(L, *(const char**)((char*)obj + offset));
				break;
			}

			return 1;
		}
		else
		{
			// get next class. globals[class][meta][index][bassclass]
			lua_pop(L, 2);

			if (!lua_getmetatable(L, -1)) break;

			lua_pushstring(L, Script::KEY_INDEX);
			lua_rawget(L, -2);
		}
	}

	// last resort
	lua_pushnil(L);
	return 1;
}

const char* Script::getTypeT(const char *type)
{
	static char type_t[64];
	sprintf(type_t, "%s_t_", type);
	return type_t;
}
