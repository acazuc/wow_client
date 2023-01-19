#include "functions.h"

#include "wow_lua.h"
#include "log.h"

static int luaAPI_GetLootMethod(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetLootMethod()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "freeforall");
	lua_pushinteger(L, 0); //Party member loot master
	lua_pushnil(L); //Raid member index, nil if not in raid
	return 3;
}

static int luaAPI_GetLootThreshold(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetLootThreshold()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 0); //Quality, cf GetItemQualityColor
	return 1;
}

static int luaAPI_GetOptOutOfLoot(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetOptOutOfLoot()");
	LUA_UNIMPLEMENTED_FN();
	//lua_pushboolean(L, true); if passing on all loots
	return 0;
}

void register_loot_functions(lua_State *L)
{
	LUA_REGISTER_FN(GetLootMethod);
	LUA_REGISTER_FN(GetLootThreshold);
	LUA_REGISTER_FN(GetOptOutOfLoot);
}
