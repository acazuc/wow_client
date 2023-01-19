#include "functions.h"

#include "wow_lua.h"
#include "log.h"

static int luaAPI_GetBattlefieldStatus(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetBattlefieldStatus(battlefield)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "none"); //queued, confirm, active, none
	lua_pushstring(L, "Warsong Gulch"); //Localized name
	lua_pushinteger(L, 0); //InstanceID
	lua_pushinteger(L, 1); //Level range min
	lua_pushinteger(L, 70); //Level range max
	lua_pushinteger(L, 0); //2, 3, 5 for PVP, 0 otherwise
	lua_pushinteger(L, 0); //1 if registered arena queue
	return 8;
}

static int luaAPI_CloseBattlefield(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: CloseBattlefield()");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

void register_battle_ground_functions(lua_State *L)
{
	LUA_REGISTER_FN(GetBattlefieldStatus);
	LUA_REGISTER_FN(CloseBattlefield);
}
