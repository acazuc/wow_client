#ifndef WOW_LUA_H
#define WOW_LUA_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdbool.h>

LUALIB_API int luaL_error(lua_State *L, const char *fmt, ...);

static inline bool lua_isinteger(lua_State *L, int index)
{
	return lua_type(L, index) == LUA_TNUMBER;
}

#endif
