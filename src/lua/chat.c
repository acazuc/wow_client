#include "functions.h"

#include "wow_lua.h"
#include "log.h"

static int luaAPI_GetChatTypeIndex(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetChatTypeIndex(index)");
	if (!lua_isstring(L, 1))
		return luaL_argerror(L, 1, "string expected");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 1);
	return 1;
}

static int luaAPI_GetChatWindowInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetChatWindowInfo(window)");
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "number expected");
	int idx = lua_tointeger(L, 1);
	/*if (idx != 1)
	{
		VERBOSE_INFO("invalid idx: %d", idx);
		return 0;
	}*/
	lua_pushstring(L, "Global"); //Name
	lua_pushnumber(L, 14); //FontSize
	lua_pushnumber(L, 0); //r
	lua_pushnumber(L, 0); //g
	lua_pushnumber(L, 0); //b
	lua_pushnumber(L, .2); //a
	lua_pushboolean(L, idx == 1); //Shown
	lua_pushboolean(L, false); //Locked
	lua_pushboolean(L, idx == 1); //Docked
	return 9;
}

static int luaAPI_SetChatWindowShown(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: SetChatWindowShow(window, \?\?\?)");
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "number expected");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetSelectedDisplayChannel(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetSelectedDisplayChannel()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 1);
	return 1;
}

static int luaAPI_GetNumDisplayChannels(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetNumDisplayChannels()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 1);
	return 1;
}

static int luaAPI_GetChannelDisplayInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetChannelDisplayInfo(index)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "name"); //name
	lua_pushstring(L, "test"); //header
	lua_pushboolean(L, false); //collapsed
	lua_pushinteger(L, 2); //channelNumber
	lua_pushinteger(L, 1); //count
	lua_pushboolean(L, true); //active
	lua_pushstring(L, "CHANNEL_CATEGORY_WORLD"); //category: CHANNEL_CATEGORY_WORLD, CHANNEL_CATEGORY_GROUP, CHANNEL_CATEGORY_CUSTOM
	lua_pushboolean(L, true); //voiceEnabled
	lua_pushboolean(L, true); //voiceActive
	return 9;
}

static int luaAPI_GetChannelRosterInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: GetChannelRosterInfo(channelID, rosterIndex)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "test"); //name
	lua_pushboolean(L, true); //owner
	lua_pushboolean(L, true); //moderator
	lua_pushboolean(L, true); //muted
	lua_pushboolean(L, true); //active
	lua_pushboolean(L, true); //enabled
	return 6;
}

static int luaAPI_SetChatWindowDocked(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: SetChatWindowDocked(id, count)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_SetChatWindowLocked(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: SetChatWindowLocked(id, locked)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_SetChatWindowName(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: SetChatWindowName(id, \"name\")");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetNumLanguages(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetNumLanguages()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 2);
	return 1;
}

static int luaAPI_GetLanguageByIndex(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetLanguageByIndex(index)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "langue");
	return 1;
}

static int luaAPI_GetChatWindowMessages(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetChatWindowMessages(index)");
	lua_pushstring(L, "SAY");
	return 1;
}

static int luaAPI_AddChatWindowMessages(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: AddChatWindowMessages(index, messageGroup)");
	/* see types in Interface/FrameXML/ChatFrame.lua */
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_RemoveChatWindowMessages(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: RemoveChatWindowMessages(index, messageGroup)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetChatWindowChannels(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetChatWindowChannels(index)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "bonjour");
	lua_pushnumber(L, 0);
	return 1;
}

static int luaAPI_SendChatMessage(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc < 1 || argc > 4)
		return luaL_error(L, "Usage: SendChatMessage(message[, chatType, language, target])");
	/* XXX chatType, language, target */
	const char *message = lua_tostring(L, 1);
	if (!message)
		return luaL_argerror(L, 1, "failed to get message string");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

void register_chat_functions(lua_State *L)
{
	LUA_REGISTER_FN(GetChatTypeIndex);
	LUA_REGISTER_FN(GetChatWindowInfo);
	LUA_REGISTER_FN(SetChatWindowShown);
	LUA_REGISTER_FN(GetSelectedDisplayChannel);
	LUA_REGISTER_FN(GetNumDisplayChannels);
	LUA_REGISTER_FN(GetChannelDisplayInfo);
	LUA_REGISTER_FN(GetChannelRosterInfo);
	LUA_REGISTER_FN(SetChatWindowDocked);
	LUA_REGISTER_FN(SetChatWindowLocked);
	LUA_REGISTER_FN(SetChatWindowName);
	LUA_REGISTER_FN(GetNumLanguages);
	LUA_REGISTER_FN(GetLanguageByIndex);
	LUA_REGISTER_FN(GetChatWindowMessages);
	LUA_REGISTER_FN(AddChatWindowMessages);
	LUA_REGISTER_FN(RemoveChatWindowMessages);
	LUA_REGISTER_FN(GetChatWindowChannels);
	LUA_REGISTER_FN(SendChatMessage);
}
