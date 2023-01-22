#include "functions.h"

#include "wow_lua.h"
#include "log.h"
#include "dbc.h"
#include "wow.h"

#include <libwow/dbc.h>

#include <inttypes.h>
#include <stdlib.h>

static int tabs[3] = {41, 61, 81};
static int tabs_count = 3;

static int luaAPI_GetNumTalentTabs(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetNumTalentTabs(\"inspect\")");
	lua_pushinteger(L, tabs_count);
	return 1;
}

static int luaAPI_GetTalentTabInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: GetTalentTabInfo(tab, \"inspect\")");
	int tab = lua_tointeger(L, 1) - 1;
	if (tab < 0 || (unsigned)tab >= sizeof(tabs) / sizeof(*tabs))
		return luaL_argerror(L, 1, "invalid tab");
	struct wow_dbc_row row;
	struct wow_dbc_row spell_icon_row;
	if (!dbc_get_row_indexed(g_wow->dbc.talent_tab, &row, tabs[tab]))
		return luaL_argerror(L, 1, "failed to get tab");
	if (!dbc_get_row_indexed(g_wow->dbc.spell_icon, &spell_icon_row, wow_dbc_get_u32(&row, 72)))
		return luaL_argerror(L, 1, "failed to get tab spell icon");
	lua_pushstring(L, wow_dbc_get_str(&row, 12)); //name
	lua_pushstring(L, wow_dbc_get_str(&spell_icon_row, 4)); //texture
	lua_pushinteger(L, 0); //points spent
	lua_pushstring(L, wow_dbc_get_str(&row, 88)); //file name
	return 4;
}

static int luaAPI_GetNumTalents(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: GetNumTalents(tab, \"inspect\")");
	LUA_UNIMPLEMENTED_FN();
	int tab = lua_tointeger(L, 1) - 1;
	if (tab < 0 || (unsigned)tab >= sizeof(tabs) / sizeof(*tabs))
		return luaL_argerror(L, 1, "invalid tab");
	size_t num = 0;
	for (size_t i = 0; i < g_wow->dbc.talent->file->header.record_count; ++i)
	{
		struct wow_dbc_row row = dbc_get_row(g_wow->dbc.talent, i);
		if (wow_dbc_get_i32(&row, 4) != tabs[tab])
			continue;
		++num;
	}
	lua_pushinteger(L, num);
	return 1;
}

static int luaAPI_GetTalentInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 3)
		return luaL_error(L, "Usage: GetTalentInfo(tab, index, \"inspect\")");
	LUA_UNIMPLEMENTED_FN();
	int tab = lua_tointeger(L, 1);
	if (tab < 1 || (unsigned)tab > sizeof(tabs) / sizeof(*tabs))
		return luaL_argerror(L, 1, "invalid tab");
	tab--;
	int index = lua_tointeger(L, 2);
	int num = 0;
	for (size_t i = 0; i < g_wow->dbc.talent->file->header.record_count; ++i)
	{
		struct wow_dbc_row row = dbc_get_row(g_wow->dbc.talent, i);
		if (wow_dbc_get_i32(&row, 4) != tabs[tab])
			continue;
		++num;
		if (num != index)
			continue;
		struct wow_dbc_row spell_row;
		struct wow_dbc_row spell_icon_row;
		if (!dbc_get_row_indexed(g_wow->dbc.spell, &spell_row, wow_dbc_get_u32(&row, 16)))
		{
			LOG_ERROR("failed to get spell row %" PRIu32, wow_dbc_get_u32(&row, 16));
			continue;
		}
		const char *icon;
		if (dbc_get_row_indexed(g_wow->dbc.spell_icon, &spell_icon_row, wow_dbc_get_u32(&spell_row, 496)))
		{
			icon = wow_dbc_get_str(&spell_icon_row, 4);
		}
		else
		{
			LOG_WARN("failed to get spell icon row %" PRIu32, wow_dbc_get_u32(&spell_row, 496));
			icon = "Interface/Icons/Temp";
		}
		size_t max = 0;
		for (; max < 9; ++max)
		{
			if (!wow_dbc_get_u32(&row, 12 + max * 4))
				break;
		}
		lua_pushstring(L, wow_dbc_get_str(&spell_row, 512));
		lua_pushstring(L, icon);
		lua_pushinteger(L, wow_dbc_get_i32(&row, 8) + 1);
		lua_pushinteger(L, wow_dbc_get_i32(&row, 12) + 1);
		lua_pushinteger(L, max == 0 ? 1 : max); //rank
		lua_pushinteger(L, max == 0 ? 1 : max); //max rank
		lua_pushboolean(L, 1); //exceptional
		lua_pushboolean(L, 1); //prereq
		return 8;
	}
	LOG_ERROR("can't find talent info: {tab: %d, index: %d}", tab, index);
	return 0;
}

static int luaAPI_LearnTalent(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: GetNumTalents(tab, index)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetTalentPrereqs(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 3)
		return luaL_error(L, "Usage: GetTalentPrereqs(tab, index, \"inspect\")");
	LUA_UNIMPLEMENTED_FN();
	//tier
	//column
	//learnable
	return 0;
}

void register_talent_functions(lua_State *L)
{
	LUA_REGISTER_FN(GetNumTalentTabs);
	LUA_REGISTER_FN(GetTalentTabInfo);
	LUA_REGISTER_FN(GetNumTalents);
	LUA_REGISTER_FN(GetTalentInfo);
	LUA_REGISTER_FN(LearnTalent);
	LUA_REGISTER_FN(GetTalentPrereqs);
}
