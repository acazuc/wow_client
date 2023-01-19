#include "functions.h"

#include "itf/interface.h"

#include "net/packets.h"
#include "net/network.h"
#include "net/packet.h"

#include "game/social.h"

#include "wow_lua.h"
#include "const.h"
#include "wow.h"
#include "log.h"
#include "dbc.h"
#include "db.h"

#include <string.h>

static int luaAPI_ShowFriends(lua_State *L)
{
	LUA_VERBOSE_FN();
	static bool g_showed = false;
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: ShowFriends()");
	if (g_showed)
		return 0;
	g_showed = true;
	if (!g_wow->social->loaded)
		social_net_load(g_wow->social);
	else
		interface_execute_event(g_wow->interface, EVENT_FRIENDLIST_SHOW, 0);
	g_showed = false;
	return 0;
}

static int luaAPI_SetSelectedFriend(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetSelectedFriend(friend)");
	if (!lua_isinteger(L, 1))
		return luaL_argerror(L, 1, "integer expected");
	g_wow->social->selected_friend = lua_tointeger(L, 1);
	return 0;
}

static int luaAPI_GetSelectedFriend(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetSelectedFriend()");
	lua_pushinteger(L, g_wow->social->selected_friend);
	return 1;
}

static int luaAPI_GetNumFriends(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetNumFriends()");
	lua_pushinteger(L, g_wow->social->friends.size);
	return 1;
}

static int luaAPI_GetFriendInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetFriendInfo(friend)");
	int idx = lua_tointeger(L, 1);
	if (idx < 0 || idx > (int32_t)g_wow->social->friends.size)
		return 0;
	struct social_friend *social_friend = JKS_ARRAY_GET(&g_wow->social->friends, idx - 1, struct social_friend);
	const struct db_name *name = db_get_name(g_wow->db, social_friend->guid);
	lua_pushstring(L, name->name);
	lua_pushinteger(L, social_friend->level);
	wow_dbc_row_t row;
	if (!social_friend->class_type)
	{
		lua_pushstring(L, "");
	}
	else if (dbc_get_row_indexed(g_wow->dbc.chr_classes, &row, social_friend->class_type))
	{
		lua_pushstring(L, wow_dbc_get_str(&row, 24));
	}
	else
	{
		lua_pushstring(L, "");
		LOG_ERROR("class not found: %d", (int)social_friend->class_type);
	}
	if (!social_friend->zone)
	{
		lua_pushstring(L, "");
	}
	else if (dbc_get_row_indexed(g_wow->dbc.area_table, &row, social_friend->zone))
	{
		lua_pushstring(L, wow_dbc_get_str(&row, 52));
	}
	else
	{
		lua_pushstring(L, "");
		LOG_ERROR("zone not found: %d", (int)social_friend->zone);
	}
	lua_pushboolean(L, social_friend->status & FRIEND_STATUS_ONLINE);
	const char *text;
	if (social_friend->status & FRIEND_STATUS_AFK)
		text = "AFK";
	else if (social_friend->status & FRIEND_STATUS_AFK2)
		text = "AFK";
	else if (social_friend->status & FRIEND_STATUS_DND)
		text = "DND";
	else
		text = "";
	lua_pushstring(L, text);
	lua_pushstring(L, social_friend->note);
	lua_pushboolean(L, false); //RAF
	return 8;
}

static int luaAPI_GetNumIgnores(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetNumIgnores()");
	lua_pushinteger(L, g_wow->social->ignores.size);
	return 1;
}

static int luaAPI_GetSelectedIgnore(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetSelectedIgnore()");
	lua_pushinteger(L, g_wow->social->selected_ignore);
	return 1;
}

static int luaAPI_SetSelectedIgnore(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetSelectedIgnore(ignore)");
	if (!lua_isinteger(L, 1))
		return luaL_argerror(L, 1, "integer expected");
	g_wow->social->selected_ignore = lua_tointeger(L, 1);
	return 0;
}

static int luaAPI_GetIgnoreName(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetIgnoreName(index)");
	if (g_wow->social->selected_ignore < 1 || g_wow->social->selected_ignore > g_wow->social->ignores.size)
	{
		LOG_ERROR("invalid selected ignore");
		lua_pushnil(L);
		return 1;
	}
	struct social_ignore *ignore = JKS_ARRAY_GET(&g_wow->social->ignores, g_wow->social->selected_ignore - 1, struct social_ignore);
	const struct db_name *name = db_get_name(g_wow->db, ignore->guid);
	lua_pushstring(L, name->name);
	return 1;
}

static int luaAPI_DelIgnore(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: DelIgnore(ignoreIndex)");
	int index = lua_tointeger(L, 1);
	if (index < 1 || index > (int32_t)g_wow->social->ignores.size)
	{
		LOG_ERROR("invalid ignore index");
		return 0;
	}
	struct social_ignore *ignore = JKS_ARRAY_GET(&g_wow->social->ignores, index - 1, struct social_ignore);
	struct net_packet_writer packet;
	if (net_cmsg_del_ignore(&packet, ignore->guid))
		net_send_packet(g_wow->network, &packet);
	net_packet_writer_destroy(&packet);
	return 0;
}

static int luaAPI_AddIgnore(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: AddIgnore(\"name\")");
	const char *name = lua_tostring(L, 1);
	if (!name)
		return luaL_argerror(L, 1, "failed to get friend string");
	struct net_packet_writer packet;
	if (net_cmsg_add_ignore(&packet, name))
		net_send_packet(g_wow->network, &packet);
	net_packet_writer_destroy(&packet);
	return 0;
}

static int luaAPI_SetWhoToUI(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetWhoToUI(set)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetNumWhoResults(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetNumWhoResults()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 2);
	return 1;
}

static int luaAPI_SendWho(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SendWho(\"who\")");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_SortWho(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SortWho(\"sort\")");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_AddFriend(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: AddFriend(\"friend\")");
	const char *name = lua_tostring(L, 1);
	if (!name)
		return luaL_argerror(L, 1, "failed to get friend string");
	struct net_packet_writer packet;
	if (net_cmsg_add_friend(&packet, name, ""))
		net_send_packet(g_wow->network, &packet);
	net_packet_writer_destroy(&packet);
	return 0;
}

static int luaAPI_RemoveFriend(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: RemoveFriend(friendIndex)");
	int index = lua_tointeger(L, 1);
	if (index < 1 || index > (int32_t)g_wow->social->friends.size)
	{
		LOG_ERROR("invalid friend index");
		return 0;
	}
	struct social_friend *social_friend = JKS_ARRAY_GET(&g_wow->social->friends, index - 1, struct social_friend);
	struct net_packet_writer packet;
	if (net_cmsg_del_friend(&packet, social_friend->guid))
		net_send_packet(g_wow->network, &packet);
	net_packet_writer_destroy(&packet);
	return 0;
}

static int luaAPI_SetFriendNotes(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: SetFriendNotes(friendIndex, \"note\")");
	int index = lua_tointeger(L, 1);
	if (index < 1 || index > (int32_t)g_wow->social->friends.size)
	{
		LOG_ERROR("invalid friend index");
		return 0;
	}
	const char *note = lua_tostring(L, 2);
	if (!note)
		return luaL_argerror(L, 2, "failed to get friend note");
	struct social_friend *social_friend = JKS_ARRAY_GET(&g_wow->social->friends, index - 1, struct social_friend);
	struct net_packet_writer packet;
	if (net_cmsg_set_contact_notes(&packet, social_friend->guid, note))
		net_send_packet(g_wow->network, &packet);
	net_packet_writer_destroy(&packet);
	return 0;
}

void register_social_functions(lua_State *L)
{
	LUA_REGISTER_FN(ShowFriends);
	LUA_REGISTER_FN(SetSelectedFriend);
	LUA_REGISTER_FN(GetSelectedFriend);
	LUA_REGISTER_FN(GetNumFriends);
	LUA_REGISTER_FN(GetFriendInfo);
	LUA_REGISTER_FN(GetNumIgnores);
	LUA_REGISTER_FN(GetSelectedIgnore);
	LUA_REGISTER_FN(SetSelectedIgnore);
	LUA_REGISTER_FN(GetIgnoreName);
	LUA_REGISTER_FN(DelIgnore);
	LUA_REGISTER_FN(AddIgnore);
	LUA_REGISTER_FN(SetWhoToUI);
	LUA_REGISTER_FN(GetNumWhoResults);
	LUA_REGISTER_FN(SendWho);
	LUA_REGISTER_FN(SortWho);
	LUA_REGISTER_FN(AddFriend);
	LUA_REGISTER_FN(RemoveFriend);
	LUA_REGISTER_FN(SetFriendNotes);
}
