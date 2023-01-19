#include "functions.h"

#include "wow_lua.h"

static int luaAPI_CombatLogResetFilter(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: CombatLogResetFilter()");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_CombatLogAddFilter(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 3)
		return luaL_error(L, "Usage: CombatLogAddFilter(list, srcFlags, dstFlags)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

void register_combat_log_functions(lua_State *L)
{
	LUA_REGISTER_FN(CombatLogResetFilter);
	LUA_REGISTER_FN(CombatLogAddFilter);
}
