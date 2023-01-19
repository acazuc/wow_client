#include "functions.h"

#include "wow_lua.h"

static int luaAPI_GetCraftSlots(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetCraftSlots()");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetCraftFilter(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetCraftFilter(index)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, 1); //checked
	return 1;
}

void register_craft_functions(lua_State *L)
{
	LUA_REGISTER_FN(GetCraftSlots);
	LUA_REGISTER_FN(GetCraftFilter);
}
