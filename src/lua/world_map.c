#include "lua/functions.h"

#include "itf/interface.h"

#include "obj/worldobj.h"

#include "wow_lua.h"
#include "cache.h"
#include "log.h"
#include "wow.h"
#include "dbc.h"

#include <libwow/dbc.h>

#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <math.h>

static bool mapid_from_continent(uint32_t continent, uint32_t *mapid)
{
	wow_dbc_row_t mapid_row;
	if (!dbc_get_row_indexed(g_wow->dbc.world_map_continent, &mapid_row, continent))
		return false;
	*mapid = wow_dbc_get_u32(&mapid_row, 4);
	return true;
}

static int luaAPI_CreateWorldMapArrowFrame(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: CreateWorldMapArrowFrame(WorldFrame)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_UpdateWorldMapArrowFrames(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: UpdateWorldMapArrowFrames()");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_PositionWorldMapArrowFrame(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc < 2 || argc == 4 || argc > 5)
		return luaL_error(L, "Usage: PositionWorldMapArrowFrame(\"point\", \"frame\"[, \"relativePoint\"] [, offsetX, offsetY])");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_ShowWorldMapArrowFrame(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: ShowWorldMapArrowFrame()");
	LUA_UNIMPLEMENTED_FN();
	//bool
	return 0;
}

static int luaAPI_InitWorldMapPing(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: InitWorldMapPing(WorldMap)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

struct world_map_data_overlay
{
	char filename[256];
	uint32_t width;
	uint32_t height;
	uint32_t x;
	uint32_t y;
	uint32_t point_x;
	uint32_t point_y;
};

struct world_map_data_area
{
	char filename[256];
	char name[256];
	float tex_width;
	float tex_height;
	float x;
	float y;
	float width;
	float height;
	int32_t areaid;
	uint32_t id;
};

static struct
{
	struct world_map_data_overlay overlays[32];
	struct world_map_data_area areas[32];
	uint32_t overlays_nb;
	uint32_t areas_nb;
	int32_t zone_id;
	uint32_t zone_idx;
	const char *area_file;
	const char *area_name;
	int32_t area_id;
	int32_t continent_id;
	float continent_min_x;
	float continent_max_x;
	float continent_min_y;
	float continent_max_y;
	float continent_width;
	float continent_height;
	float zone_min_x;
	float zone_max_x;
	float zone_min_y;
	float zone_max_y;
	float zone_width;
	float zone_height;
} world_map_data;

static void update_continent(uint32_t id, struct world_map_data_area *area)
{
	wow_dbc_row_t row;
	bool found = false;
	for (size_t i = 0; i < g_wow->dbc.world_map_continent->file->header.record_count; ++i)
	{
		row = wow_dbc_get_row(g_wow->dbc.world_map_continent->file, i);
		if (wow_dbc_get_u32(&row, 4) != id)
			continue;
		found = true;
		break;
	}
	uint32_t left = wow_dbc_get_u32(&row, 8);
	uint32_t right = wow_dbc_get_u32(&row, 12);
	uint32_t top = wow_dbc_get_u32(&row, 16);
	uint32_t bottom = wow_dbc_get_u32(&row, 20);
	float offset_x = wow_dbc_get_flt(&row, 24);
	float offset_y = wow_dbc_get_flt(&row, 28);
	float scale = wow_dbc_get_flt(&row, 32);
	assert(found);
	area->tex_width = .1;
	area->tex_height = 1;
	area->x = - ((offset_x) / 32);
	LOG_INFO("x: %f", area->x);
	area->y = 0;
	area->width = (right - left) / 64.;
	area->height = (bottom - top) / 64.;
	LOG_INFO("width: %f; height: %f", area->width, area->height);
	area->areaid = 2;
	area->id = 2;
}

void update_world_map_data(int32_t continent_id, int32_t zone_id, uint32_t zone_idx)
{
#if 0
	LOG_INFO("setting continent %" PRId32 " zone %" PRIu32, continent_id, zone_id);
#endif
	world_map_data.continent_id = continent_id;
	world_map_data.zone_id = zone_id;
	world_map_data.zone_idx = zone_idx;
	switch (continent_id)
	{
		case 0: /* World */
		{
			world_map_data.area_file = 0;
			world_map_data.area_id = 0;
			world_map_data.overlays_nb = 0;
			world_map_data.areas_nb = 1;
			struct world_map_data_area *kalimdor = &world_map_data.areas[0];
			struct world_map_data_area *azeroth = &world_map_data.areas[1];
			snprintf(kalimdor->filename, sizeof(kalimdor->filename), "kalimdor");
			snprintf(kalimdor->name, sizeof(kalimdor->name), "kalimdor");
			update_continent(1, kalimdor);
			snprintf(azeroth->filename, sizeof(azeroth->filename), "azeroth");
			snprintf(azeroth->name, sizeof(azeroth->name), "azeroth");
			azeroth->tex_width = 1;
			azeroth->tex_height = 1;
			azeroth->x = 0;
			azeroth->y = .5;
			azeroth->width = .5;
			azeroth->height = .5;
			azeroth->areaid = 1;
			azeroth->id = 1;
			break;
		}
		case 1: /* Eastern kingdom */
		case 2: /* Kalimdor */
		case 3: /* Outlands */
			/* Global map */
			if (world_map_data.zone_id == 0)
			{
				const char *area_files[3] = {"azeroth", "kalimdor", "expansion01"};
				const int32_t continents[3] = {0, 1, 530};
				world_map_data.area_file = area_files[continent_id - 1];
				world_map_data.area_id = 0;
				world_map_data.overlays_nb = 0;
				world_map_data.areas_nb = 0;
				bool found = false;
				for (uint32_t i = 0; i < g_wow->dbc.world_map_area->file->header.record_count; ++i)
				{
					wow_dbc_row_t row = dbc_get_row(g_wow->dbc.world_map_area, i);
					if (wow_dbc_get_i32(&row, 8) != 0)
						continue;
					int32_t display_id = wow_dbc_get_i32(&row, 32);
					if (display_id != -1)
					{
						if (display_id != continents[continent_id - 1])
							continue;
					}
					else
					{
						if (wow_dbc_get_i32(&row, 4) != continents[continent_id - 1])
							continue;
					}
					world_map_data.continent_max_x = wow_dbc_get_flt(&row, 16);
					world_map_data.continent_min_x = wow_dbc_get_flt(&row, 20);
					world_map_data.continent_max_y = wow_dbc_get_flt(&row, 24);
					world_map_data.continent_min_y = wow_dbc_get_flt(&row, 28);
					found = true;
					break;
				}
				if (!found)
				{
					LOG_ERROR("continent with area 0 not found");
					return;
				}
				world_map_data.continent_width = world_map_data.continent_max_x - world_map_data.continent_min_x;
				world_map_data.continent_height = world_map_data.continent_max_y - world_map_data.continent_min_y;
				world_map_data.zone_min_x = world_map_data.continent_min_x;
				world_map_data.zone_max_x = world_map_data.continent_max_x;
				world_map_data.zone_min_y = world_map_data.continent_min_y;
				world_map_data.zone_max_y = world_map_data.continent_max_y;
				world_map_data.zone_width = world_map_data.continent_width;
				world_map_data.zone_height = world_map_data.continent_height;
				for (uint32_t i = 0; i < g_wow->dbc.world_map_area->file->header.record_count; ++i)
				{
					wow_dbc_row_t row = dbc_get_row(g_wow->dbc.world_map_area, i);
					if (!wow_dbc_get_i32(&row, 8)) //zoneid 0, for world map
						continue;
					int32_t display_id = wow_dbc_get_i32(&row, 32);
					if (display_id != -1)
					{
						if (display_id != continents[continent_id - 1])
							continue;
					}
					else
					{
						if (wow_dbc_get_i32(&row, 4) != continents[continent_id - 1])
							continue;
					}
					float loc_right = wow_dbc_get_flt(&row, 16);
					float loc_left = wow_dbc_get_flt(&row, 20);
					float loc_bottom = wow_dbc_get_flt(&row, 24);
					float loc_top = wow_dbc_get_flt(&row, 28);
					struct world_map_data_area *area = &world_map_data.areas[world_map_data.areas_nb];
					snprintf(area->filename, sizeof(area->filename), "%s", wow_dbc_get_str(&row, 12));
					area->tex_width = 1;
					area->tex_height = 1;
					float min_x = (loc_left - world_map_data.continent_min_x) / world_map_data.continent_width;
					float min_y = (loc_top - world_map_data.continent_min_y) / world_map_data.continent_height;
					float max_x = (loc_right - world_map_data.continent_min_x) / world_map_data.continent_width;
					float max_y = (loc_bottom - world_map_data.continent_min_y) / world_map_data.continent_height;
					area->x = min_x;
					area->y = min_y;
					area->width = max_x - min_x;
					area->height = max_y - min_y;
					area->areaid = wow_dbc_get_i32(&row, 8);
					area->id = wow_dbc_get_u32(&row, 0);
					if (dbc_get_row_indexed(g_wow->dbc.area_table, &row, area->areaid))
					{
						snprintf(area->name, sizeof(area->name), "%s", wow_dbc_get_str(&row, 52));
					}
					else
					{
						LOG_ERROR("area not found");
						area->name[0] = '\0';
					}
					world_map_data.areas_nb++;
				}
			}
			else
			{
				wow_dbc_row_t row;
				if (dbc_get_row_indexed(g_wow->dbc.world_map_area, &row, zone_id))
				{
					world_map_data.zone_min_x = wow_dbc_get_flt(&row, 20);
					world_map_data.zone_max_x = wow_dbc_get_flt(&row, 16);
					world_map_data.zone_min_y = wow_dbc_get_flt(&row, 28);
					world_map_data.zone_max_y = wow_dbc_get_flt(&row, 24);
					world_map_data.zone_width = world_map_data.zone_max_x - world_map_data.zone_min_x;
					world_map_data.zone_height = world_map_data.zone_max_y - world_map_data.zone_min_y;
					world_map_data.area_file = wow_dbc_get_str(&row, 12);
					world_map_data.area_id = wow_dbc_get_u32(&row, 8);
				}
				else
				{
					LOG_ERROR("failed to get zone %" PRId32, zone_id);
					world_map_data.zone_min_x = 0;
					world_map_data.zone_max_x = 0;
					world_map_data.zone_min_y = 0;
					world_map_data.zone_max_y = 0;
					world_map_data.zone_width = 0;
					world_map_data.zone_height = 0;
					world_map_data.area_file = NULL;
					world_map_data.area_id = 0;
				}
				if (dbc_get_row_indexed(g_wow->dbc.area_table, &row, world_map_data.area_id))
				{
					world_map_data.area_name = wow_dbc_get_str(&row, 52);
				}
				else
				{
					LOG_ERROR("failed to get area %" PRId32, world_map_data.area_id);
					world_map_data.area_name = NULL;
				}
				world_map_data.areas_nb = 0;
				world_map_data.overlays_nb = 0;
				for (uint32_t i = 0; i < g_wow->dbc.world_map_overlay->file->header.record_count; ++i)
				{
					row = dbc_get_row(g_wow->dbc.world_map_overlay, i);
					if (wow_dbc_get_i32(&row, 4) != zone_id)
						continue;
					struct world_map_data_overlay *overlay = &world_map_data.overlays[world_map_data.overlays_nb];
					const char *fn = wow_dbc_get_str(&row, 32);
					if (!fn || !fn[0]) /* dustwallow for exemple has 2 overlays with empty filename, with 1 having non-null size */
						continue;
					snprintf(overlay->filename, sizeof(overlay->filename), "Interface\\WorldMap\\%s\\%s", world_map_data.area_file, fn);
					overlay->width = wow_dbc_get_u32(&row, 36);
					overlay->height = wow_dbc_get_u32(&row, 40);
					overlay->x = wow_dbc_get_u32(&row, 44);
					overlay->y = wow_dbc_get_u32(&row, 48);
					overlay->point_x = wow_dbc_get_u32(&row, 24);
					overlay->point_y = wow_dbc_get_u32(&row, 28);
					world_map_data.overlays_nb++;
				}
			}
			break;
		default:
			world_map_data.area_file = 0;
			world_map_data.area_id = 0;
			world_map_data.overlays_nb = 0;
			world_map_data.areas_nb = 0;
			break;
	}
}

static int luaAPI_GetMapInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetMapInfo()");
	/* LOG_ERROR("area_file: %s", world_map_data.area_file); */
	lua_pushstring(L, world_map_data.area_file);
	lua_pushinteger(L, 0); //TextureHeight, unused
	return 2;
}

static int luaAPI_GetCurrentMapContinent(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetCurrentMapContinent()");
	/* LOG_INFO("current continent: %" PRId32, world_map_data.continent_id); */
	lua_pushnumber(L, world_map_data.continent_id); //0: world, 1: Azeroth, 2: Kalimdor, 3: outlands, -1: cosmic
	return 1;
}

static int luaAPI_GetCurrentMapZone(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetCurrentMapZone()");
	if (world_map_data.zone_idx == (uint32_t)-1)
		lua_pushnumber(L, 0);
	else
		lua_pushnumber(L, world_map_data.zone_idx);
	return 1;
}

static int luaAPI_GetNumMapLandmarks(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetNumMapLandmarks()");
	uint32_t mapid;
	if (!mapid_from_continent(world_map_data.continent_id, &mapid))
	{
		LOG_ERROR("failed to get mapid for continent %" PRIu32, world_map_data.continent_id);
		lua_pushnumber(L, 0);
		return 1;
	}
	size_t n = 0;
	for (size_t i = 0; i < g_wow->dbc.area_poi->file->header.record_count; ++i)
	{
		wow_dbc_row_t row = wow_dbc_get_row(g_wow->dbc.area_poi->file, i);
		uint32_t continent = wow_dbc_get_u32(&row, 28);
		if (continent != mapid)
			continue;
		uint32_t flags = wow_dbc_get_u32(&row, 32);
		if (world_map_data.zone_id == 0)
		{
			if (!(flags & 0x8))
				continue;
		}
		else if (!world_map_data.continent_id)
		{
			if (!(flags & 0x10))
				continue;
		}
		else
		{
			if (!(flags & 0x4))
				continue;
		}
		float x = wow_dbc_get_flt(&row, 20);
		float y = wow_dbc_get_flt(&row, 16);
		x = (x - world_map_data.zone_min_x) / world_map_data.zone_width;
		y = (y - world_map_data.zone_min_y) / world_map_data.zone_height;
		if (x < 0 || x > 1 || y < 0 || y > 1)
			continue;
		n++;
	}
	lua_pushinteger(L, n);
	return 1;
}

static int luaAPI_GetNumMapOverlays(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetNumMapOverlays()");
	lua_pushinteger(L, world_map_data.overlays_nb);
	return 1;
}

static int luaAPI_GetMapOverlayInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetMapOverlayInfo(overlay)");
	if (!lua_isinteger(L, 1))
		return luaL_argerror(L, 1, "integer expected");
	uint32_t index = lua_tointeger(L, 1);
	index--;
	if (index >= world_map_data.overlays_nb)
		return luaL_argerror(L, 1, "index too big");
	struct world_map_data_overlay *overlay = &world_map_data.overlays[index];
	lua_pushstring(L, overlay->filename);
	lua_pushnumber(L, overlay->width);
	lua_pushnumber(L, overlay->height);
	lua_pushnumber(L, overlay->x);
	lua_pushnumber(L, overlay->y);
	lua_pushnumber(L, overlay->point_x);
	lua_pushnumber(L, overlay->point_y);
	return 7;
}

static int luaAPI_SetMapToCurrentZone(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: SetMapToCurrentZone()");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static struct world_map_data_area *get_cursor_area(float cursor_x, float cursor_y)
{
	if (cursor_x < 0 || cursor_x > 1 || cursor_y < 0 || cursor_y > 1)
		return NULL;
	struct world_map_data_area *best = NULL;
	float best_delta = INFINITY;
	for (size_t i = 0; i < world_map_data.areas_nb; ++i)
	{
		struct world_map_data_area *area = &world_map_data.areas[i];
		if (cursor_x >= area->x && cursor_x <= area->x + area->width
		 && cursor_y >= area->y && cursor_y <= area->y + area->height)
		{
			float center_x = area->x + area->width / 2;
			float center_y = area->y + area->height / 2;
			float diff_x = center_x - cursor_x;
			float diff_y = center_y - cursor_y;
			float delta = sqrt(diff_x * diff_x + diff_y * diff_y);
			if (delta < best_delta)
			{
				best_delta = delta;
				best = area;
			}
		}
	}
	return best;
}

static int luaAPI_UpdateMapHighlight(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: UpdateMapHighlight(cursorX, cursorY)");
	float cursor_x = lua_tonumber(L, 1);
	float cursor_y = lua_tonumber(L, 2);
	cursor_y = 1 - cursor_y;
	struct world_map_data_area *best = get_cursor_area(cursor_x, cursor_y);
	if (!best)
		return 0;
	lua_pushstring(L, best->name);
	lua_pushstring(L, best->filename);
	lua_pushnumber(L, 1);
	lua_pushnumber(L, best->height > best->width * 1.5 ? best->height / (best->width * 1.5) / 2 : best->height / (best->width * 1.5));
	lua_pushnumber(L, best->width);
	lua_pushnumber(L, best->height);
	lua_pushnumber(L, 1 - best->x - best->width);
	lua_pushnumber(L, 1 - best->y - best->height);
	return 8;
}

static int luaAPI_GetPlayerMapPosition(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetPlayerMapPosition(\"unit\")");
	const char *s = lua_tostring(L, 1);
	struct unit *unit;
	if (s && !strcmp(s, "raid1")) /* XXX handle real player arrow */
		unit = (struct unit*)g_wow->player;
	else
		unit = lua_get_unit(L, 1);
	if (!unit)
	{
		lua_pushnumber(L, 0);
		lua_pushnumber(L, 0);
		return 2;
	}
	float x = -((struct worldobj*)unit)->position.z;
	float y = ((struct worldobj*)unit)->position.x;
	x = (x - world_map_data.zone_min_x) / world_map_data.zone_width;
	y = (y - world_map_data.zone_min_y) / world_map_data.zone_height;
	if (x < 0 || x > 1 || y < 0 || y > 1)
	{
		lua_pushnumber(L, 0);
		lua_pushnumber(L, 0);
		return 2;
	}
	lua_pushnumber(L, 1 - x);
	lua_pushnumber(L, 1 - y);
	return 2;
}

static int luaAPI_GetNumBattlefieldPositions(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetNumBattlefieldPositions()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 0);
	return 1;
}

static int luaAPI_GetBattlefieldPosition(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetBattlefieldPosition(index)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushnumber(L, 0);//rand() / (float)RAND_MAX); //percentX
	lua_pushnumber(L, 0);//rand() / (float)RAND_MAX); //percentY
	lua_pushstring(L, "coucou"); //name
	return 3;
}

static int luaAPI_GetNumBattlefieldFlagPositions(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetNumBattlefieldFlagPositions()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 0); //max 2
	return 1;
}

static int luaAPI_GetBattlefieldFlagPosition(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetBattlefieldFlagPosition(index)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushnumber(L, 0);//rand() / (float)RAND_MAX); //percentX
	lua_pushnumber(L, 0);//rand() / (float)RAND_MAX); //percentY
	lua_pushstring(L, "AllianceFlag"); //token: Interface/WorldStateFrame/token.blp
	return 3;
}

static int luaAPI_GetCorpseMapPosition(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetCorpseMapPosition()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushnumber(L, 0);//rand() / (float)RAND_MAX); //percentX
	lua_pushnumber(L, 0);//rand() / (float)RAND_MAX); //percentY
	return 2;
}

static int luaAPI_GetDeathReleasePosition(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetDeathReleasePosition()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushnumber(L, 0);//rand() / (float)RAND_MAX); //percentX
	lua_pushnumber(L, 0);//rand() / (float)RAND_MAX); //percentY
	return 2;
}

static int luaAPI_ProcessMapClick(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: ProcessMapClick(x, y)");
	float cursor_x = lua_tonumber(L, 1);
	float cursor_y = lua_tonumber(L, 2);
	cursor_y = 1 - cursor_y;
	struct world_map_data_area *best = get_cursor_area(cursor_x, cursor_y);
	if (!best)
		return 0;
	if (!world_map_data.continent_id)
		update_world_map_data(best->id, 0, -1);
	else
		update_world_map_data(world_map_data.continent_id, best->id, 1 + (best - &world_map_data.areas[0]));
	interface_execute_event(g_wow->interface, EVENT_WORLD_MAP_UPDATE, 0);
	return 0;
}

static int luaAPI_SetMapZoom(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc < 1 || argc > 2)
		return luaL_error(L, "Usage: SetMapZoom(continent [, area])");
	uint32_t continent = lua_tointeger(L, 1);
	uint32_t zone_id;
	LOG_INFO("continent: %d", continent);
	if (argc == 1)
	{
		update_world_map_data(continent, 0, -1);
		interface_execute_event(g_wow->interface, EVENT_WORLD_MAP_UPDATE, 0);
		return 0;
	}
	if (continent < 1)
		return 0;
	uint32_t mapid;
	if (!mapid_from_continent(continent, &mapid))
	{
		LOG_ERROR("failed to get mapid for continent %" PRIu32, continent);
		return 0;
	}
	zone_id = lua_tointeger(L, 2);
	if (zone_id < 1)
		return 0;
	uint32_t nb = 1;
	for (size_t i = 0; i < g_wow->dbc.world_map_area->file->header.record_count; ++i)
	{
		wow_dbc_row_t row = dbc_get_row(g_wow->dbc.world_map_area, i);
		uint32_t display_id = wow_dbc_get_i32(&row, 32);
		if (display_id != (uint32_t)-1)
		{
			if (display_id != mapid)
				continue;
		}
		else
		{
			if (wow_dbc_get_u32(&row, 4) != mapid)
				continue;
		}
		if (!wow_dbc_get_u32(&row, 8))
			continue;
		if (nb == zone_id)
		{
			update_world_map_data(continent, wow_dbc_get_u32(&row, 0), zone_id);
			interface_execute_event(g_wow->interface, EVENT_WORLD_MAP_UPDATE, 0);
			return 0;
		}
		nb++;
	}
	LOG_ERROR("invalid zone_id");
	return 0;
}

static int luaAPI_GetMapContinents(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetMapContinents()");
	uint32_t nb = 0;
	for (size_t i = 0; i < g_wow->dbc.world_map_continent->file->header.record_count; ++i)
	{
		wow_dbc_row_t continent_row = dbc_get_row(g_wow->dbc.world_map_continent, i);
		uint32_t mapid = wow_dbc_get_u32(&continent_row, 4);
		wow_dbc_row_t row;
		if (!dbc_get_row_indexed(g_wow->dbc.map, &row, mapid))
		{
			LOG_ERROR("failed to get map %" PRIu32, mapid);
			continue;
		}
		lua_pushstring(L, wow_dbc_get_str(&row, 24));
		nb++;
	}
	return nb;
}

static int luaAPI_GetMapZones(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetMapZones(continent)");
	int continent = lua_tointeger(L, 1);
	if (continent < 1)
		return 0;
	uint32_t mapid;
	if (!mapid_from_continent(continent, &mapid))
	{
		LOG_ERROR("failed to get mapid for continent %" PRIu32, continent);
		return 0;
	}
	uint32_t nb = 0;
	for (size_t i = 0; i < g_wow->dbc.world_map_area->file->header.record_count; ++i)
	{
		wow_dbc_row_t row = dbc_get_row(g_wow->dbc.world_map_area, i);
		uint32_t display_id = wow_dbc_get_i32(&row, 32);
		if (display_id != (uint32_t)-1)
		{
			if (display_id != mapid)
				continue;
		}
		else
		{
			if (wow_dbc_get_u32(&row, 4) != mapid)
				continue;
		}
		uint32_t area_id = wow_dbc_get_u32(&row, 8);
		if (!area_id)
			continue;
		wow_dbc_row_t area_row;
		if (!dbc_get_row_indexed(g_wow->dbc.area_table, &area_row, area_id))
		{
			LOG_ERROR("failed to get area %" PRIu32, area_id);
			continue;
		}
		lua_pushstring(L, wow_dbc_get_str(&area_row, 52));
		nb++;
	}
	return nb;
}

static int luaAPI_GetMapLandmarkInfo(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetMapLandmarkInfo(index)");
	int idx = lua_tointeger(L, 1);
	if (idx < 0)
		return 0;
	uint32_t mapid;
	if (!mapid_from_continent(world_map_data.continent_id, &mapid))
	{
		LOG_ERROR("failed to get mapid for continent %" PRIu32, world_map_data.continent_id);
		return 0;
	}
	int n = 0;
	for (size_t i = 0; i < g_wow->dbc.area_poi->file->header.record_count; ++i)
	{
		wow_dbc_row_t row = wow_dbc_get_row(g_wow->dbc.area_poi->file, i);
		uint32_t continent = wow_dbc_get_u32(&row, 28);
		if (continent != mapid)
			continue;
		uint32_t flags = wow_dbc_get_u32(&row, 32);
		if (world_map_data.zone_id == 0)
		{
			if (!(flags & 0x8))
				continue;
		}
		else if (!world_map_data.continent_id)
		{
			if (!(flags & 0x10))
				continue;
		}
		else
		{
			if (!(flags & 0x4))
				continue;
		}
		float x = wow_dbc_get_flt(&row, 20);
		float y = wow_dbc_get_flt(&row, 16);
		x = (x - world_map_data.zone_min_x) / world_map_data.zone_width;
		y = (y - world_map_data.zone_min_y) / world_map_data.zone_height;
		if (x < 0 || x > 1 || y < 0 || y > 1)
			continue;
		n++;
		if (n != idx)
			continue;
		lua_pushstring(L, wow_dbc_get_str(&row, 48));
		lua_pushstring(L, wow_dbc_get_str(&row, 116));
		lua_pushinteger(L, wow_dbc_get_u32(&row, 8));
		lua_pushnumber(L, 1 - x);
		lua_pushnumber(L, 1 - y);
		return 5;
	}
	return 0;
}

void register_world_map_functions(lua_State *L)
{
	LUA_REGISTER_FN(CreateWorldMapArrowFrame);
	LUA_REGISTER_FN(UpdateWorldMapArrowFrames);
	LUA_REGISTER_FN(PositionWorldMapArrowFrame);
	LUA_REGISTER_FN(ShowWorldMapArrowFrame);
	LUA_REGISTER_FN(InitWorldMapPing);
	LUA_REGISTER_FN(GetMapInfo);
	LUA_REGISTER_FN(GetCurrentMapContinent);
	LUA_REGISTER_FN(GetCurrentMapZone);
	LUA_REGISTER_FN(GetNumMapLandmarks);
	LUA_REGISTER_FN(GetNumMapOverlays);
	LUA_REGISTER_FN(GetMapOverlayInfo);
	LUA_REGISTER_FN(SetMapToCurrentZone);
	LUA_REGISTER_FN(UpdateMapHighlight);
	LUA_REGISTER_FN(GetPlayerMapPosition);
	LUA_REGISTER_FN(GetNumBattlefieldPositions);
	LUA_REGISTER_FN(GetBattlefieldPosition);
	LUA_REGISTER_FN(GetNumBattlefieldFlagPositions);
	LUA_REGISTER_FN(GetBattlefieldFlagPosition);
	LUA_REGISTER_FN(GetCorpseMapPosition);
	LUA_REGISTER_FN(GetDeathReleasePosition);
	LUA_REGISTER_FN(ProcessMapClick);
	LUA_REGISTER_FN(SetMapZoom);
	LUA_REGISTER_FN(GetMapContinents);
	LUA_REGISTER_FN(GetMapZones);
	LUA_REGISTER_FN(GetMapLandmarkInfo);
}
