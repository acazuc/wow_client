#include "functions.h"

#include "wow_lua.h"
#include "log.h"

static int luaAPI_GetPVPYesterdayStats(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetPVPYesterdayStats()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 0); //HK
	lua_pushinteger(L, 0); //Contribution
	return 2;
}

static int luaAPI_GetPVPLifetimeStats(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetPVPLifetimeStats()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 0); //HK
	lua_pushinteger(L, 0); //Contribution
	return 2;
}

static int luaAPI_GetHonorCurrency(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetHonorCurrency()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 1337);
	return 1;
}

static int luaAPI_GetPVPSessionStats(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetPVPSessionStats()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 0); //HK
	lua_pushinteger(L, 0); //Contribution
	return 2;
}

static int luaAPI_GetPVPRankInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetPVPRankInfo(\"rank\")");
	lua_pushstring(L, "rank"); /* name */
	lua_pushinteger(L, 6); /* number */
	return 2;
}

void register_pvp_functions(lua_State *L)
{
	LUA_REGISTER_FN(GetPVPYesterdayStats);
	LUA_REGISTER_FN(GetPVPLifetimeStats);
	LUA_REGISTER_FN(GetHonorCurrency);
	LUA_REGISTER_FN(GetPVPSessionStats);
	LUA_REGISTER_FN(GetPVPRankInfo);
}
