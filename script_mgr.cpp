#include "./script_mgr.h"

lua_State	*ScriptMgr::pVM_ = lua_open();
ScriptNode	*ScriptMgr::pHead = 0;

int ScriptMgr::installScripts(lua_State *L)
{
	luaL_openlibs(L);

	// create an weak envinroment table for typed checking
	//lua_newtable(L);
	//lua_pushstring(L, "v");
	//lua_setfield(L, -2, "__mode");	// weak table for gc, pop value
	//lua_pushvalue(L, -1);			// __mode only work in metatable
	//lua_setmetatable(L, -2);
	//lua_replace(L, LUA_ENVIRONINDEX);
	// table is cleared at this point

	// setup all script object's pVM_ to proper lua_State,
	// and expose variables and functions
	ScriptNode *curr = pHead;
	while (curr)
	{
		curr->script->setup(L);
		curr = curr->pNext;
	}

	// modified original globals metatable, if any.
	// this is for global variable and function
	lua_getmetatable(L, LUA_GLOBALSINDEX);
	if (!lua_istable(L, -1))
		lua_newtable(L);
	// replace __newindex's table to cfunction
	lua_pushstring(L, Script::KEY_NEW_INDEX);
	lua_pushvalue(L, -1);
	lua_rawget(L, -3);
	lua_pushcclosure(L, globalNewIndex, 1);
	lua_rawset(L, -3);
	// replace __index's table to cfunction
	lua_pushstring(L, Script::KEY_INDEX);
	lua_pushvalue(L, -1);
	lua_rawget(L, -3);
	lua_pushcclosure(L, globalIndex, 1);
	lua_rawset(L, -3);
	lua_setmetatable(L, LUA_GLOBALSINDEX);

	// execute main script
	luaL_loadfile(L, "main.lua");
	if (lua_pcall(L, 0, LUA_MULTRET, 0))
		printf("Error executing main.lua: %s\n", lua_tostring(L, -1));

	return 0;
}

bool ScriptMgr::init()
{
	lua_pushcclosure(pVM_, installScripts, 0);
	if (lua_cpcall(pVM_, installScripts, 0))
	{
		printf("Install Scripts Error: %s\n", lua_tostring(pVM_, -1));
		return false;
	}
	return true;
}

bool ScriptMgr::deinit()
{
	ScriptNode *temp;
	ScriptNode *curr = pHead;
	while (curr)
	{
		temp = curr;
		curr = curr->pNext;
		temp->script->uninstallScript();
	}
	
	lua_close(pVM_);

	curr = pHead;
	while (curr)
	{
		temp = curr;
		curr = curr->pNext;
		temp->script->reset();
		delete temp;
	}
	pHead = 0;
	pVM_ = 0;

	return true;
}

void ScriptMgr::addScript(Script *script)
{
	ScriptNode *node = (ScriptNode*)malloc(sizeof(ScriptNode));
	memset(node, 0, sizeof(ScriptNode));
	node->script = script;

	if (!pHead) 
	{
		pHead = node;
		return;
	}

	ScriptNode *last = pHead;
	while (last && last->pNext)
		last = last->pNext;
	last->pNext = node;
	node->pPrev = last;
}

// do we need this?
void ScriptMgr::removeScript(Script *script)
{
	ScriptNode *curr = pHead;
	while (curr)
	{
		if (curr->script == script)
		{
			script->reset();
			if (pHead == curr) pHead = curr->pNext;
			if (curr->pPrev) curr->pPrev->pNext = curr->pNext;
			if (curr->pNext) curr->pNext->pPrev = curr->pPrev;
			delete curr;
			return;
		}
	}
}

// 1|-3:table, 2|-2: key, 3|-1: value
int ScriptMgr::globalNewIndex(lua_State *L)
{
//const char *key = lua_tostring(L, 2);
	if (findTable(LUA_ENVIRONINDEX, Script::KEY_VARIABLE_TABLE, false))
	{
		void *var = popVar(-1, 2);
		if (var)
		{
			switch(popType(-2, 2))
			{
			case Script::ESDT_Int: // integer
				*(lua_Integer*)var = lua_tointeger(L, 3); // get value
				return 0;
			case Script::ESDT_Num: // double
				*(lua_Number*)var = lua_tonumber(L, 3); // get value
				return 0;
			case Script::ESDT_Str: // string
				strcpy((char*)var, lua_tostring(L, 3)); // get value
				return 0;
			case Script::ESDT_Ptr:
				*(int*)var = *(int*)(lua_touserdata(L, 3));
				return 0;
			case Script::ESDT_PtrInt:
				**(lua_Integer**)var = lua_tointeger(L, 3);
				return 0;
			case Script::ESDT_PtrNum:
				**(lua_Number**)var = lua_tonumber(L, 3);
				return 0;
			case Script::ESDT_PtrStr:
				strcpy(*(char**)var, lua_tostring(L, 3));
				return 0;
			}
		}
		lua_pop(L, 1); // pop away the table
	}

	switch(lua_type(L,lua_upvalueindex(1)))
	{
	case LUA_TNONE:
	case LUA_TNIL:
	case LUA_TTHREAD:
	case LUA_TLIGHTUSERDATA:
	case LUA_TUSERDATA:
	case LUA_TBOOLEAN:
	case LUA_TNUMBER:
	case LUA_TSTRING:
	case LUA_TTABLE:
		// use standard tag method
		lua_rawset(L, 1);
		break;
	case LUA_TFUNCTION:
		{
			lua_CFunction func = lua_tocfunction(L, lua_upvalueindex(1));
			func(L);
		}
		break;
	}

	return 0;
}

// 1|-2: table, 2|-1: key
int ScriptMgr::globalIndex(lua_State *L)
{
//const char *key = lua_tostring(L, 2);
	if (findTable(LUA_ENVIRONINDEX, Script::KEY_VARIABLE_TABLE, false))
	{
		void *var = popVar(-1, 2);
		if (var)
		{
			switch(popType(-2, 2))
			{
			case Script::ESDT_Int: // integer
				lua_pushinteger(L, *(lua_Integer*)var);
				return 1;
			case Script::ESDT_Num: // double
				lua_pushnumber(L, *(lua_Number*)var);
				return 1;
			case Script::ESDT_Str: // string
				lua_pushstring(L, (char*)var);
				return 1;
			case Script::ESDT_Ptr:
				lua_pushlightuserdata(L, (void*)var);
				return 1;
			case Script::ESDT_PtrInt:
				lua_pushinteger(L, **(lua_Integer**)var);
				return 1;
			case Script::ESDT_PtrNum:
				lua_pushnumber(L, **(lua_Number**)var);
				return 1;
			case Script::ESDT_PtrStr:
				lua_pushstring(L, *(const char**)var);
				return 1;
			}
		}
		lua_pop(L, 1); // pop away the table
	}

	switch(lua_type(L,lua_upvalueindex(1)))
	{
	case LUA_TNONE:
	case LUA_TNIL:
	case LUA_TTHREAD:
		lua_pushinteger(pVM_, 0);
		break;
	case LUA_TBOOLEAN:
	case LUA_TLIGHTUSERDATA:
	case LUA_TNUMBER:
	case LUA_TSTRING:
	case LUA_TFUNCTION:
	case LUA_TUSERDATA:
		lua_pushvalue(L, lua_upvalueindex(1));
		break;
	case LUA_TTABLE:
		lua_pushvalue(L, 2);
		lua_rawget(L, lua_upvalueindex(1));
		break;
	}

	return 1;
}

void ScriptMgr::pushVar(int idx, const char *name, void *data)
{
	lua_pushstring(pVM_, name);
	lua_pushlightuserdata(pVM_, data);
	lua_rawset(pVM_, (idx<0 ? idx-2 : idx));
}

void* ScriptMgr::popVar(int idx, int nameidx)
{
	lua_pushvalue(pVM_, nameidx); // key name
	lua_rawget(pVM_, (idx<0 ? idx-1 : idx));
	if (!lua_isuserdata(pVM_, -1)) 
	{
		lua_pop(pVM_, 1);
		return 0;
	}
	return lua_touserdata(pVM_, -1);
}

void ScriptMgr::pushType(int idx, const char *name, int type)
{
	lua_pushfstring(pVM_, "%s_t_", name);
	lua_pushinteger(pVM_, type);
	lua_rawset(pVM_, (idx<0 ? idx-2 : idx));
}

int ScriptMgr::popType(int idx, int nameidx)
{
	const char *name = lua_tostring(pVM_, nameidx); // key name
	lua_pushfstring(pVM_, "%s_t_", name);
	lua_rawget(pVM_, (idx<0 ? idx-1 : idx));
	if (!lua_isnumber(pVM_, -1)) 
	{
		lua_pop(pVM_, 1);
		return 0;
	}
	return (int)lua_tointeger(pVM_, -1);
}

/// re-implement luaL_register or luaI_openlib
/// 1) this version accept std::vector instead of c array and also no checking
/// for upvalue
/// 2) remove all lua_xxxfield to lua_rawxxx
bool ScriptMgr::registerClass(const char *type, Script::MethodList &methods, Script::PropertyList &properties)
{
	int size = (int)methods.size();
	/* check whether lib already exists */
	findTable(LUA_REGISTRYINDEX, "_LOADED", true, 1);
	lua_pushstring(pVM_, type);
	lua_rawget(pVM_, -2);  /* get _LOADED[type] */
	if (!lua_istable(pVM_, -1)) 
	{  /* not found? */
		lua_pop(pVM_, 1);  /* remove previous result */
		/* try global variable (and create one if it does not exist) */
		if (!findTable(LUA_GLOBALSINDEX, type, true, size+1)) // +1 for variable list
		{// conflict with existing module
			return false;
		}
		lua_pushstring(pVM_, type);
		lua_pushvalue(pVM_, -2); // duplicate Globals[type]
		lua_rawset(pVM_, -4);  // _LOADED[type] = new table
	}
	lua_remove(pVM_, -2);  /* remove _LOADED table */

	int i;
	for (i=0; i<size; ++i)
	{
		lua_pushstring(pVM_, methods[i].name);
		lua_pushcclosure(pVM_, methods[i].func, 0);
		lua_rawset(pVM_, -3);
	}

	size = (int)properties.size();
	lua_pushstring(pVM_, Script::KEY_VARIABLE_TABLE);
	lua_createtable(pVM_, 0, size*2);
	for (i=0; i<size; ++i)
	{
		lua_pushstring(pVM_, properties[i].name);
		lua_pushinteger(pVM_, properties[i].offset);
		lua_rawset(pVM_, -3); // store offset
		lua_pushfstring(pVM_, "%s_t_", properties[i].name);
		lua_pushinteger(pVM_, properties[i].type);
		lua_rawset(pVM_, -3); // store variable type
	}
	lua_rawset(pVM_, -3); // write variable table to Globals[type]

	return true; // Globals[type] table still in main stack
}

// Similar to luaL_findtable, this version use lua_rawxxx instead of lua_xxxtable
// and also remove the while loop (what is that for actually?)
bool ScriptMgr::findTable(int idx, const char *name, bool create, int size)
{
	lua_pushstring(pVM_, name);
	lua_rawget(pVM_, idx);

	if (lua_istable(pVM_, -1))
	{
		return true;
	}
	else if (lua_isnil(pVM_, -1) && create)
	{ // create a new table with given name
		lua_pop(pVM_, 1); // pop result
		lua_createtable(pVM_, 0, size);
		lua_pushstring(pVM_, name);
		lua_pushvalue(pVM_, -2);
		lua_rawset(pVM_, idx);
		return true;
	}

	lua_pop(pVM_, 1); // pop result
	return false; // already used for other datatypes
}

int ScriptMgr::getMethod(const char *type, const char *funcName, int funcType)
{
	lua_getfield(pVM_, LUA_REGISTRYINDEX, type); // get metatable

	while(lua_istable(pVM_, -1))
	{
		lua_getfield(pVM_, -1, Script::KEY_LIB); // get Globals[Type]
		lua_remove(pVM_, -2);

		lua_getfield(pVM_, -1, funcName);

		if (lua_type(pVM_, -1)== funcType)
		{
			return 2;
		}

		lua_pop(pVM_, 1);
		lua_getfield(pVM_, -1, Script::KEY_BASE_CLASS); // get metatable of base
		lua_remove(pVM_, -2);
	}
	lua_pop(pVM_, 1);

	return 0;
}

int ScriptMgr::getProperty(const char *type, const char *propName)
{
	lua_getfield(pVM_, LUA_REGISTRYINDEX, type); // get metatable of Type

	while(lua_istable(pVM_, -1))
	{
		lua_getfield(pVM_, -1, Script::KEY_LIB); // get Globals[Type]
		lua_getfield(pVM_, -1, Script::KEY_VARIABLE_TABLE); // get Gloals[Type][var_t_];
		lua_remove(pVM_, -2);
		lua_remove(pVM_, -2);

		lua_pushvalue(pVM_, 3);
		lua_getfield(pVM_, -2, propName);

		if (lua_type(pVM_, -1) == LUA_TNUMBER)
		{
			int ret = (int)lua_tointeger(pVM_, -1);
			lua_pop(pVM_, 1);
			return ret;
		}

		lua_getfield(pVM_, -2, Script::KEY_BASE_CLASS); // get metatable of base
	}
	lua_pop(pVM_, 1);

	return -1;
}

void ScriptMgr::checkType(int idx)
{
	printf("Type Test Result[%d] = ", idx);
	switch(lua_type(pVM_, idx))
	{
	case LUA_TNONE:
	case LUA_TNIL:
		printf("Nil\n");
		break;
	case LUA_TLIGHTUSERDATA:
	case LUA_TUSERDATA:
		printf("pointer: 0x%p\n", lua_touserdata(pVM_, idx));
		break;
	case LUA_TNUMBER:
		printf("number: %f\n", lua_tonumber(pVM_, idx));
		break;
	case LUA_TSTRING:
		printf("string: %s\n", lua_tostring(pVM_, idx));
		break;
	case LUA_TTHREAD:
		printf("thread\n");
		break;
	case LUA_TBOOLEAN:
		printf("boolean: %d\n", lua_toboolean(pVM_, idx));
		break;
	case LUA_TFUNCTION:
		printf("function\n");
		break;
	case LUA_TTABLE:
		printf("table\n");
		break;
	default:
		printf("undefined type: %d\n", lua_type(pVM_, idx));
		break;
	}	
}
