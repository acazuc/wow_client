#include "functions.h"

#include "wow_lua.h"
#include "log.h"

static int luaAPI_GetInboxNumItems(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetInboxNumItems()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushnumber(L, 6);
	return 1;
}

static int luaAPI_GetInboxHeaderInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetInboxHeaderInfo(index)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "Interface\\Icons\\Spell_Shadow_SoulGem.blp"); //packageIcon
	lua_pushstring(L, "Interface\\Icons\\Spell_Shadow_SoulGem.blp"); //stationaryIcon
	lua_pushstring(L, "test"); //sender
	lua_pushstring(L, "coucou"); //subject
	lua_pushinteger(L, 13371337); //money
	lua_pushinteger(L, 13381338); //COD
	lua_pushinteger(L, 10); //daysLeft
	lua_pushinteger(L, 3); //itemCount
	lua_pushboolean(L, false); //wasRead
	lua_pushboolean(L, false); //wasReturned
	lua_pushboolean(L, false); //textCreated
	lua_pushboolean(L, true); //canReply
	lua_pushboolean(L, true); //isGM
	lua_pushinteger(L, 30); //firstItemQuantity
	return 14;
}

static int luaAPI_SetSendMailShowing(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetSendMailShowing(showing)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_InboxItemCanDelete(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: InboxItemCanDelete(index)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true);
	return 1;
}

void register_inbox_functions(lua_State *L)
{
	LUA_REGISTER_FN(GetInboxNumItems);
	LUA_REGISTER_FN(GetInboxHeaderInfo);
	LUA_REGISTER_FN(SetSendMailShowing);
	LUA_REGISTER_FN(InboxItemCanDelete);
}
