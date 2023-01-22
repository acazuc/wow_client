#include "functions.h"

#include "itf/interface.h"

#include "net/world_socket.h"

#include "ui/model_ffx.h"

#include "net/network.h"
#include "net/packet.h"
#include "net/opcode.h"

#include "wow_lua.h"
#include "cache.h"
#include "log.h"
#include "wow.h"
#include "dbc.h"

#include <libwow/dbc.h>

#include <inttypes.h>

static uint32_t selected_sex = 1;
static uint32_t selected_race = 9;
static uint32_t selected_class = 0;
static uint32_t selected_character = 0;
static float facing = 0;
static float character_facing = 0;
static int selected_category = 1;
static uint8_t available_classes[6];
static uint8_t available_classes_nb;

static struct
{
	int id;
	const char *name;
	bool available;
} const g_races[] =
{
	{1 , "HUMAN"   , true},
	{3 , "DWARF"   , true},
	{4 , "NIGHTELF", true},
	{7 , "GNOME"   , true},
	{11, "DRAENEI" , true},
	{2 , "ORC"     , true},
	{5 , "SCOURGE" , true},
	{6 , "TAUREN"  , true},
	{8 , "TROLL"   , true},
	{10, "BLOODELF", true},
};

static struct
{
	int id;
	const char *name;
} const g_classes[] =
{
	{1 , "WARRIOR"}, //0
	{8 , "MAGE"   }, //1
	{4 , "ROGUE"  }, //2
	{11, "DRUID"  }, //3
	{3 , "HUNTER" }, //4
	{7 , "SHAMAN" }, //5
	{5 , "PRIEST" }, //6
	{9 , "WARLOCK"}, //7
	{2 , "PALADIN"}, //8
};

static int luaAPI_DefaultServerLogin(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: DefaultServerLogin(\"username\", \"password\")");
	if (!lua_isstring(L, 1))
		return luaL_argerror(L, 1, "string expected");
	if (!lua_isstring(L, 2))
		return luaL_argerror(L, 2, "string expected");
	const char *user = lua_tostring(L, 1);
	if (!user)
		return luaL_argerror(L, 1, "lua_tostring failed");
	const char *pass = lua_tostring(L, 2);
	if (!pass)
		return luaL_argerror(L, 2, "lua_tostring failed");
	LOG_ERROR("login \"%s\" \"%s\"", user, pass);
	net_auth_connect(g_wow->network, user, pass);
	return 0;
}

static int luaAPI_SetSavedAccountName(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetSavedAccountName(\"username\")");
	if (!lua_isstring(L, 1))
		return luaL_argerror(L, 1, "string expected");
	const char *user = lua_tostring(L, 1);
	if (!user)
		return luaL_argerror(L, 1, "lua_tostring failed");
	LOG_INFO("save account: \"%s\"", user);
	return 0;
}

static int luaAPI_GetSavedAccountName(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetSavedAccountName()");
	lua_pushstring(L, "ADMINISTRATOR");
	return 1;
}

static int luaAPI_GetClientExpansionLevel(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetClientExpansionLevel()");
	lua_pushinteger(L, 2);
	return 1;
}

static int luaAPI_GetAccountExpansionLevel(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetAccountExpansionLevel()");
	lua_pushinteger(L, 1);
	return 1;
}
static int luaAPI_ResetCharCustomize(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: ResetCharCustomize()");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetAvailableRaces(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetAvailableRaces()");
	for (size_t i = 0; i < 10; ++i)
	{
		struct wow_dbc_row row;
		if (!dbc_get_row_indexed(g_wow->dbc.chr_races, &row, g_races[i].id))
		{
			LOG_INFO("nope");
			lua_pushstring(L, "");
			lua_pushstring(L, "");
			lua_pushinteger(L, 0);
			continue;
		}
		const char *name = wow_dbc_get_str(&row, selected_sex == 2 ? 132 : 200);
		if (selected_sex == 2 && (name == NULL || name[0] == '\0')) /* paladin doesn't have female name in french */
			name = wow_dbc_get_str(&row, 200);
		lua_pushstring(L, name);
		lua_pushstring(L, g_races[i].name);
		lua_pushinteger(L, g_races[i].available ? 1 : 0);
	}
	return 30;
}

static int luaAPI_GetSelectedSex(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetSelectedSex()");
	lua_pushinteger(L, selected_sex);
	return 1;
}

static int luaAPI_SetSelectedSex(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetSelectedSex(sex)");
	if (!lua_isinteger(L, 1))
		return luaL_argerror(L, 1, "integer expected");
	selected_sex = lua_tointeger(L, 1);
	return 0;
}

static int luaAPI_GetSelectedRace(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetSelectedRace()");
	lua_pushnumber(L, selected_race);
	return 1;
}

static int luaAPI_SetSelectedRace(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SelectSelectedRace(race)");
	if (!lua_isinteger(L, 1))
		return luaL_argerror(L, 1, "integer expected");
	selected_race = lua_tointeger(L, 1);
	available_classes_nb = 0;
	for (size_t i = 0; i < g_wow->dbc.char_base_info->file->header.record_count; ++i)
	{
		struct wow_dbc_row row = dbc_get_row(g_wow->dbc.char_base_info, i);
		if (wow_dbc_get_u8(&row, 0) != g_races[selected_race - 1].id)
			continue;
		available_classes[available_classes_nb++] = wow_dbc_get_u8(&row, 1);
	}
	return 0;
}

static int luaAPI_GetFactionForRace(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetFactionForRace()");
	if (selected_race < 5)
	{
		lua_pushstring(L, "Alliance"); //localized name
		lua_pushstring(L, "Alliance"); //internale name
	}
	else
	{
		lua_pushstring(L, "Horde"); //localized name
		lua_pushstring(L, "Horde"); //internale name
	}
	return 2;
}

static int luaAPI_GetNameForRace(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetNameForRace()");
	if (selected_race < 1 || selected_race > 10)
		return 0;
	struct wow_dbc_row row;
	if (dbc_get_row_indexed(g_wow->dbc.chr_races, &row, g_races[selected_race - 1].id))
	{
		lua_pushstring(L, wow_dbc_get_str(&row, selected_sex == 2 ? 132 : 200));
		lua_pushstring(L, g_races[selected_race - 1].name);
	}
	else
	{
		lua_pushstring(L, "");
		lua_pushstring(L, "");
	}
	return 2;
}

static int luaAPI_GetClassesForRace(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetClassesForRace()");
	uint32_t classes_nb = 0;
	for (uint32_t i = 0; i < available_classes_nb; ++i)
	{
		uint8_t class = available_classes[i];
		uint8_t class_id = 0xff;
		for (uint32_t j = 0; j < sizeof(g_classes) / sizeof(*g_classes); ++j)
		{
			if (g_classes[j].id == class)
			{
				class_id = j;
				break;
			}
		}
		if (class_id == 0xff)
		{
			LOG_WARN("class %u not found", class_id);
			continue;
		}
		LOG_INFO("found class %u", class);
		struct wow_dbc_row class_row;
		if (!dbc_get_row_indexed(g_wow->dbc.chr_classes, &class_row, class))
		{
			LOG_WARN("class id %u not found", class_id);
			continue;
		}
		lua_pushstring(L, wow_dbc_get_str(&class_row, selected_sex == 2 ? 92 : 160));
		lua_pushstring(L, g_classes[class_id].name);
		classes_nb++;
	}
	return classes_nb * 2;
}

static int luaAPI_SetCharCustomizeBackground(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetCharCustomizeBackground()");
	if (!lua_isstring(L, 1))
		return luaL_argerror(L, 1, "string expected");
	const char *fn = lua_tostring(L, 1);
	struct ui_frame *frame = interface_get_frame(g_wow->interface, "CharacterCreate");
	if (!frame)
		return luaL_error(L, "CharacterCreate frame doesn't exists");
	struct ui_model_ffx *model = ui_object_as_model_ffx((struct ui_object*)frame);
	if (!model)
		return luaL_error(L, "CharacterCreate frame isn't ModelFFX");
	ui_model_set_model((struct ui_model*)model, fn);
	return 0;
}

static int luaAPI_CycleCharCustomization(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: CycleCharCustomization(id, direction)");
	if (!lua_isinteger(L, 1))
		return luaL_argerror(L, 1, "integer expected");
	int id = lua_tointeger(L, 1);
	if (!lua_isinteger(L, 2))
		return luaL_argerror(L, 2, "integer expected");
	int dir = lua_tointeger(L, 2);
	LOG_ERROR("unimplemented function (id: %d, direction: %d)", id, dir);
	return 0;
}

static int luaAPI_RandomizeCharCustomization(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: RandomizeCharCustomization()");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetSelectedClass(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetSelectedClass()");
	struct wow_dbc_row row;
	if (dbc_get_row_indexed(g_wow->dbc.chr_classes, &row, g_classes[available_classes[selected_class]].id))
	{
		LOG_INFO("class filename: %s", wow_dbc_get_str(&row, selected_sex == 2 ? 92 : 160));
		lua_pushstring(L, wow_dbc_get_str(&row, selected_sex == 2 ? 92 : 160));
	}
	else
	{
		lua_pushstring(L, "");
	}
	lua_pushstring(L, g_classes[available_classes[selected_class]].name);
	return 2;
}

static int luaAPI_SetSelectedClass(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetSelectedClass(class)");
	if (!lua_isinteger(L, 1))
		return luaL_argerror(L, 1, "integer expected");
	selected_class = lua_tointeger(L, 1);
	return 0;
}

static int luaAPI_GetFacialHairCustomization(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetFacialHairCustomization()");
	struct wow_dbc_row row;
	if (dbc_get_row_indexed(g_wow->dbc.chr_races, &row, selected_race))
	{
		lua_pushstring(L, wow_dbc_get_str(&row, 264)); //NONE, EARRINGS, FEATURES, HAIR, HORNS, MARKINGS, NORMAL, PIERCINGS, TUSKS
	}
	else
	{
		LOG_ERROR("selected race not found: %" PRIu32, selected_race);
		lua_pushstring(L, "NONE");
	}
	return 1;
}

static int luaAPI_GetHairCustomization(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetHairCustomization()");
	struct wow_dbc_row row;
	if (dbc_get_row_indexed(g_wow->dbc.chr_races, &row, selected_race))
	{
		lua_pushstring(L, wow_dbc_get_str(&row, 268)); //NORMAL, HORNS
	}
	else
	{
		LOG_ERROR("selected race not found: %" PRIu32, selected_race);
		lua_pushstring(L, "NORMAL");
	}
	return 1;
}

static int luaAPI_GetCharacterCreateFacing(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetCharacterCreateFacing()");
	lua_pushnumber(L, character_facing);
	return 1;
}

static int luaAPI_SetCharacterCreateFacing(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetCharacterCreateFacing(facing)");
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "number expected");
	character_facing = lua_tonumber(L, 1);
	return 0;
}

static int luaAPI_GetSelectedCategory(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetSelectedCategory()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, selected_category);
	return 1;
}

static int luaAPI_SetSelectedCategory(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetSelectedCategory(category)");
	LUA_UNIMPLEMENTED_FN();
	selected_category = lua_tointeger(L, 1);
	return 0;
}

static int luaAPI_GetRealmCategories(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetRealmCategories()");
	LUA_UNIMPLEMENTED_FN();
	uint8_t categories[256];
	size_t categories_nb = 0;
	for (size_t i = 0; i < g_wow->network->world_servers.size; ++i)
	{
		struct world_server *world_server = JKS_ARRAY_GET(&g_wow->network->world_servers, i, struct world_server);
		for (size_t j = 0; j < categories_nb; ++j)
		{
			if (world_server->timezone == categories[j])
				goto next_server;
		}
		categories[categories_nb++] = world_server->timezone;
next_server:
		continue;
	}
	for (size_t i = 0; i < categories_nb; ++i)
	{
		LOG_INFO("category: %d", categories[i]);
		char tmp[32];
		snprintf(tmp, sizeof(tmp), "%d", categories[i]);
		lua_pushstring(L, tmp);
	}
	return categories_nb;
}

static int luaAPI_GetNumRealms(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetNumRealms(category)");
	LUA_UNIMPLEMENTED_FN();
	int category = lua_tointeger(L, 1);
	if (category <= 0)
		return luaL_argerror(L, 1, "invalid category");
	uint8_t categories[256];
	size_t categories_nb = 0;
	uint8_t nb = 0;
	for (size_t i = 0; i < g_wow->network->world_servers.size; ++i)
	{
		struct world_server *world_server = JKS_ARRAY_GET(&g_wow->network->world_servers, i, struct world_server);
		for (size_t j = 0; j < categories_nb; ++j)
		{
			if (world_server->timezone == categories[j])
				goto already_set;
		}
		categories[categories_nb++] = world_server->timezone;
already_set:
		if (categories_nb == category)
			nb++;
	}
	LOG_INFO("realms for category %d : %" PRIu8, category, nb);
	lua_pushinteger(L, nb);
	return 1;
}

static int luaAPI_GetRealmInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: GetRealmInfo(category, index)");
	for (size_t i = 0; i < g_wow->network->world_servers.size; ++i)
	{
		struct world_server *world_server = JKS_ARRAY_GET(&g_wow->network->world_servers, i, struct world_server);
		lua_pushstring(L, world_server->name);
		lua_pushinteger(L, world_server->characters);
		lua_pushboolean(L, world_server->flags & WORLD_SERVER_INVALID);
		lua_pushboolean(L, world_server->flags & WORLD_SERVER_OFFLINE);
		lua_pushboolean(L, false); //currentRealm
		lua_pushboolean(L, true); //PVP
		lua_pushboolean(L, true); //RP
		float load;
		if (world_server->flags & WORLD_SERVER_FULL)
			load = 2;
		else if (world_server->flags & WORLD_SERVER_RECOMMENDED)
			load = -3;
		else if (world_server->flags & WORLD_SERVER_NEW_PLAYERS)
			load = -2;
		else
			load = world_server->load;
		lua_pushnumber(L, load);
		lua_pushboolean(L, world_server->locked);
		lua_pushinteger(L, 2); //major
		lua_pushinteger(L, 4); //minor
		lua_pushinteger(L, 3); //revision
		lua_pushinteger(L, 8606); //build
	}
	return 13;
}

static int luaAPI_CancelRealmListQuery(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: CancelRealmListQuery()");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_ChangeRealm(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: ChangeRealm(\"category\", index)");
	LUA_UNIMPLEMENTED_FN();
	const char *category = lua_tostring(L, 1);
	int idx = lua_tointeger(L, 2); //0 for suggested realm
	if (idx < 0 || (size_t)idx > g_wow->network->world_servers.size)
	{
		LOG_ERROR("ChangeRealm invalid index (%d / %u)", idx, (unsigned)g_wow->network->world_servers.size);
		return 0;
	}
	struct world_server *world_server = JKS_ARRAY_GET(&g_wow->network->world_servers, !idx ? 0 : idx - 1, struct world_server);
	net_world_connect(g_wow->network, world_server->host);
	return 0;
}

static int luaAPI_IsConnectedToServer(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: IsConnectedToServer()");
	lua_pushboolean(L, g_wow->network->world_socket != NULL);
	return 1;
}

static int luaAPI_GetCharacterListUpdate(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetCharacterListUpdate()");
	LUA_UNIMPLEMENTED_FN();
	//Load character, then run event CHARACTER_LIST_UPDATE
	return 0;
}

static int luaAPI_RequestRealmList(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc > 1)
		return luaL_error(L, "Usage: RequestRealmList([force])");
	LUA_UNIMPLEMENTED_FN();
	interface_execute_event(g_wow->interface, EVENT_OPEN_REALM_LIST, 0);
	return 0;
}

static int luaAPI_RequestRealmSplitInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: RequestRealmSplitInfo()");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_SortRealms(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SortRealms(\"category\")");
	LUA_UNIMPLEMENTED_FN();
	const char *cat = lua_tostring(L, 1);
	if (cat == NULL)
		return luaL_argerror(L, 1, "String expected");
	//XXX Do the sort
	interface_execute_event(g_wow->interface, EVENT_OPEN_REALM_LIST, 0); //update the view
	return 0;
}

static int luaAPI_RealmListDialogCancelled(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: RealmListDialogCancelled()");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetNumCharacters(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetNumCharacters()");
	lua_pushinteger(L, g_wow->network->world_socket ? g_wow->network->world_socket->characters.size : 0);
	return 1;
}

static int luaAPI_GetCharacterInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetCharacterInfo(index)");
	if (!lua_isinteger(L, 1))
	{
		char tmp[256];
		snprintf(tmp, sizeof(tmp), "integer expected, got %s", lua_typename(L, lua_type(L, 1)));
		return luaL_argerror(L, 1, tmp);
	}
	int idx = lua_tointeger(L, 1);
	if (!g_wow->network->world_socket)
		return luaL_error(L, "no world socket");
	if (idx < 1 || (size_t)idx > g_wow->network->world_socket->characters.size)
	{
		char tmp[256];
		snprintf(tmp, sizeof(tmp), "invalid character id (%u / %u)", (unsigned)idx, (unsigned)g_wow->network->world_socket->characters.size);
		return luaL_argerror(L, 1, tmp);
	}
	const struct login_character *character = JKS_ARRAY_GET(&g_wow->network->world_socket->characters, idx - 1, struct login_character);
	lua_pushstring(L, character->name);
	struct wow_dbc_row row;
	if (dbc_get_row_indexed(g_wow->dbc.chr_races, &row, character->race_type))
		lua_pushstring(L, wow_dbc_get_str(&row, character->gender ? 132 : 200));
	else
		lua_pushstring(L, "");
	if (dbc_get_row_indexed(g_wow->dbc.chr_classes, &row, character->class_type))
		lua_pushstring(L, wow_dbc_get_str(&row, character->gender ? 92 : 160));
	else
		lua_pushstring(L, "");
	lua_pushinteger(L, character->level);
	if (dbc_get_row_indexed(g_wow->dbc.area_table, &row, character->zone))
		lua_pushstring(L, wow_dbc_get_str(&row, 52));
	else
		lua_pushstring(L, "");
	for (size_t i = 0; i < sizeof(g_races) / sizeof(*g_races); ++i)
	{
		if (g_races[i].id == character->race_type)
		{
			lua_pushstring(L, g_races[i].name);
			break;
		}
	}
	lua_pushinteger(L, character->gender);
	lua_pushboolean(L, false); //ghost
	return 8;
}

static int luaAPI_SetCharSelectBackground(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetCharSelectBackground(\"model\")");
	if (!lua_isstring(L, 1))
		return luaL_argerror(L, 1, "string expected");
	const char *fn = lua_tostring(L, 1);
	struct ui_frame *frame = interface_get_frame(g_wow->interface, "CharacterSelect");
	if (!frame)
		return luaL_error(L, "CharacterSelect frame doesn't exists");
	struct ui_model_ffx *model = ui_object_as_model_ffx((struct ui_object*)frame);
	if (!model)
		return luaL_error(L, "CharacterSelect frame isn't ModelFFX");
	ui_model_set_model((struct ui_model*)model, fn);
	return 0;
}

static int luaAPI_SelectCharacter(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SelectCharacter(index)");
	if (!lua_isinteger(L, 1))
		return luaL_argerror(L, 1, "integer expected");
	selected_character = lua_tointeger(L, 1);
	lua_pushnil(L);
	lua_pushinteger(L, selected_character);
	interface_execute_event(g_wow->interface, EVENT_UPDATE_SELECTED_CHARACTER, 1);
	return 0;
}

static int luaAPI_GetCharacterSelectFacing(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetCharacterSelectFacing()");
	lua_pushnumber(L, facing);
	return 1;
}

static int luaAPI_SetCharacterSelectFacing(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetCharacterSelectFacing(facing)");
	LUA_UNIMPLEMENTED_FN();
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "number expected");
	facing = lua_tonumber(L, 1);
	//XXX set facing
	return 0;
}

static int luaAPI_CreateCharacter(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: CreateCharacter(\"name\")");
	LUA_UNIMPLEMENTED_FN();
	if (!lua_isstring(L, 1))
		return luaL_argerror(L, 1, "string expected");
	const char *name = lua_tostring(L, 1);
	if (!name)
		return luaL_argerror(L, 1, "tostring failed");
	//selected_sex
	//selected_race
	//selected_class
	return 0;
}

static int luaAPI_EnterWorld(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: EnterWorld()");
	if (!g_wow->network->world_socket)
		return luaL_error(L, "no world socket");
	if (selected_character < 1 || (size_t)selected_character > g_wow->network->world_socket->characters.size)
	{
		char tmp[256];
		snprintf(tmp, sizeof(tmp), "invalid character id (%u / %u)", (unsigned)selected_character, (unsigned)g_wow->network->world_socket->characters.size);
		return luaL_argerror(L, 1, tmp);
	}
	const struct login_character *character = JKS_ARRAY_GET(&g_wow->network->world_socket->characters, selected_character - 1, struct login_character);
	struct net_packet_writer packet;
	net_packet_writer_init(&packet, CMSG_PLAYER_LOGIN);
	net_write_u64(&packet, character->guid);
	if (!net_send_packet(g_wow->network, &packet))
		LOG_ERROR("failed to send packet");
	net_packet_writer_destroy(&packet);
	return 1;
}

static int luaAPI_GetRandomName(lua_State *L)
{
	struct dbc *dbc;
	if (!cache_ref_by_key_dbc(g_wow->cache, "DBFilesClient\\NameGen.dbc", &dbc))
	{
		LOG_ERROR("failed to get NameGen.dbc");
		lua_pushstring(L, "");
		return 1;
	}
	lua_pushstring(L, "random");
	return 1;
	for (uint32_t i = 0; i < dbc->file->header.record_count; ++i)
	{
		struct wow_dbc_row row = dbc_get_row(dbc, i);
		uint32_t race = wow_dbc_get_u32(&row, 32);
		if (race != selected_race) // shouldn't be checked against selected_race (see ChrRaces.dbc)
			continue;
		uint32_t sex = wow_dbc_get_u32(&row, 36);
		if (sex != selected_sex)
			continue;
	}
	return 0;
}

static int luaAPI_DisconnectFromServer(lua_State *L)
{
	LUA_UNIMPLEMENTED_FN();
	net_disconnect(g_wow->network);
	return 0;
}

static int luaAPI_GetServerName(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetServerName()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "a");
	lua_pushboolean(L, true); //isPVP
	lua_pushboolean(L, true); //isRP
	return 3;
}

static int luaAPI_GetRealmName(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetRealmName()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "cc");
	return 1;
}

static int luaAPI_ShowChangedOptionWarnings(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: ShowChangedOptionWarnings()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true);
	return 1;
}

static int luaAPI_GetChangedOptionWarnings(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetChangedOptionWarnings()");
	LUA_UNIMPLEMENTED_FN();
	//lua_pushstring(L, "test1");
	//lua_pushstring(L, "test2");
	return 0;
}

static int luaAPI_IsTournamentRealmCategory(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: IsTournamentRealmCategory(index)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, false);
	return 1;
}

static int luaAPI_IsInvalidLocale(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: IsInvalidLocale(category)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, false);
	return 1;
}

void register_glue_functions(lua_State *L)
{
	LUA_REGISTER_FN(DefaultServerLogin);
	LUA_REGISTER_FN(SetSavedAccountName);
	LUA_REGISTER_FN(GetSavedAccountName);
	LUA_REGISTER_FN(GetClientExpansionLevel);
	LUA_REGISTER_FN(GetAccountExpansionLevel);
	LUA_REGISTER_FN(ResetCharCustomize);
	LUA_REGISTER_FN(GetAvailableRaces);
	LUA_REGISTER_FN(GetSelectedSex);
	LUA_REGISTER_FN(SetSelectedSex);
	LUA_REGISTER_FN(GetSelectedRace);
	LUA_REGISTER_FN(SetSelectedRace);
	LUA_REGISTER_FN(GetFactionForRace);
	LUA_REGISTER_FN(GetNameForRace);
	LUA_REGISTER_FN(GetClassesForRace);
	LUA_REGISTER_FN(SetCharCustomizeBackground);
	LUA_REGISTER_FN(CycleCharCustomization);
	LUA_REGISTER_FN(RandomizeCharCustomization);
	LUA_REGISTER_FN(GetSelectedClass);
	LUA_REGISTER_FN(SetSelectedClass);
	LUA_REGISTER_FN(GetFacialHairCustomization);
	LUA_REGISTER_FN(GetHairCustomization);
	LUA_REGISTER_FN(GetCharacterCreateFacing);
	LUA_REGISTER_FN(SetCharacterCreateFacing);
	LUA_REGISTER_FN(GetSelectedCategory);
	LUA_REGISTER_FN(SetSelectedCategory);
	LUA_REGISTER_FN(GetRealmCategories);
	LUA_REGISTER_FN(GetNumRealms);
	LUA_REGISTER_FN(GetRealmInfo);
	LUA_REGISTER_FN(CancelRealmListQuery);
	LUA_REGISTER_FN(ChangeRealm);
	LUA_REGISTER_FN(IsConnectedToServer);
	LUA_REGISTER_FN(GetCharacterListUpdate);
	LUA_REGISTER_FN(RequestRealmList);
	LUA_REGISTER_FN(RequestRealmSplitInfo);
	LUA_REGISTER_FN(SortRealms);
	LUA_REGISTER_FN(RealmListDialogCancelled);
	LUA_REGISTER_FN(GetNumCharacters);
	LUA_REGISTER_FN(GetCharacterInfo);
	LUA_REGISTER_FN(SetCharSelectBackground);
	LUA_REGISTER_FN(SelectCharacter);
	LUA_REGISTER_FN(GetCharacterSelectFacing);
	LUA_REGISTER_FN(SetCharacterSelectFacing);
	LUA_REGISTER_FN(CreateCharacter);
	LUA_REGISTER_FN(EnterWorld);
	LUA_REGISTER_FN(GetRandomName);
	LUA_REGISTER_FN(DisconnectFromServer);
	LUA_REGISTER_FN(GetServerName);
	LUA_REGISTER_FN(GetRealmName);
	LUA_REGISTER_FN(ShowChangedOptionWarnings);
	LUA_REGISTER_FN(GetChangedOptionWarnings);
	LUA_REGISTER_FN(IsTournamentRealmCategory);
	LUA_REGISTER_FN(IsInvalidLocale);
}
