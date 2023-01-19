#include "functions.h"

#include "wow_lua.h"
#include "log.h"

static int luaAPI_SetTradeSkillItemNameFilter(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "SetTradeSkillItemNameFilter(\"filter\")");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

void register_trade_functions(lua_State *L)
{
	LUA_REGISTER_FN(SetTradeSkillItemNameFilter);
}
