#include "functions.h"

#include "wow_lua.h"
#include "log.h"

static int luaAPI_issecure(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: issecure()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true);
	return 1;
}

static int luaAPI_securecall(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc < 1)
		return luaL_error(L, "Usage: securecall(function, ...)");
	LUA_UNIMPLEMENTED_FN();
	switch (lua_type(L, 1))
	{
		case LUA_TSTRING:
		{
			const char *name = lua_tostring(L, 1);
			if (!name)
				return 0;
			lua_getglobal(L, name);
			for (int i = 2; i <= argc; ++i)
				lua_pushvalue(L, i);
			lua_pcall(L, argc - 1, LUA_MULTRET, 0);
			return lua_gettop(L) - argc;
		}
		case LUA_TFUNCTION:
		{
			lua_pcall(L, argc - 1, LUA_MULTRET, 0);
			return lua_gettop(L) - argc;
		}
		default:
			return luaL_argerror(L, 1, "string or function expected");
	}
	return 0;
}

void register_secure_functions(lua_State *L)
{
	LUA_REGISTER_FN(issecure);
	LUA_REGISTER_FN(securecall);
}
