#include "lua/functions.h"

#include "ui/texture.h"

#include "map/map.h"

#include "wow_lua.h"
#include "memory.h"
#include "wow.h"
#include "dbc.h"
#include "log.h"

#include <assert.h>

static int g_current_node = 15;

static int luaAPI_SetTaxiMap(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetTaxiMap(frame)");
	struct ui_texture *texture = ui_get_lua_texture(L, 1);
	if (!texture)
		return luaL_argerror(L, 1, "invalid texture given");
	char file[256];
	snprintf(file, sizeof(file), "interface/taxiframe/taximap%d.blp", g_wow->map->id);
	ui_texture_set_file(texture, file);
	return 0;
}

static int luaAPI_NumTaxiNodes(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: NumTaxiNodes()");
	lua_pushnumber(L, g_wow->map->taxi.nodes_count);
	return 1;
}

static int luaAPI_TaxiNodeGetType(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: TaxiNodeGetType(index)");
	int index = lua_tointeger(L, 1);
	if (index < 1)
		return luaL_argerror(L, 1, "invalid index");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, index != g_current_node ? "REACHABLE" : "CURRENT"); /* NONE, CURRENT, REACHABLE, DISTANT */
	return 1;
}

static int luaAPI_TaxiNodePosition(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: TaxiNodePosition(index)");
	int index = lua_tointeger(L, 1);
	if (index < 1)
		return luaL_argerror(L, 1, "invalid index");
	index--;
	if ((size_t)index >= g_wow->map->taxi.nodes_count)
		return luaL_argerror(L, 1, "invalid index");
	struct taxi_node *node = &g_wow->map->taxi.nodes[index];
	lua_pushnumber(L, 1 - (wow_dbc_get_flt(&node->dbc, 12) - g_wow->map->worldmap.minx) / g_wow->map->worldmap.width);
	lua_pushnumber(L, (wow_dbc_get_flt(&node->dbc, 8) - g_wow->map->worldmap.miny) / g_wow->map->worldmap.height);
	return 2;
}

static int luaAPI_TaxiNodeName(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: TaxiNodeName(index)");
	int index = lua_tointeger(L, 1);
	if (index < 1)
		return luaL_argerror(L, 1, "invalid index");
	index--;
	if ((size_t)index >= g_wow->map->taxi.nodes_count)
		return luaL_argerror(L, 1, "invalid index");
	lua_pushstring(L, wow_dbc_get_str(&g_wow->map->taxi.nodes[index].dbc, 28));
	return 1;
}

static int luaAPI_GetNumRoutes(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetNumRoutes(index)");
	int index = lua_tointeger(L, 1);
	if (index < 1)
		return luaL_argerror(L, 1, "invalid index");
	map_gen_taxi_path(g_wow->map, g_current_node - 1, index - 1);
	lua_pushinteger(L, g_wow->map->taxi.path_nodes_count ? g_wow->map->taxi.path_nodes_count - 1 : 0);
	return 1;
}

static int luaAPI_TaxiGetSrcX(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: TaxiGetSrcX(index, route)");
	int index = lua_tointeger(L, 1);
	if (index < 1)
		return luaL_argerror(L, 1, "invalid index");
	int route = lua_tointeger(L, 2);
	if (route < 1)
		return luaL_argerror(L, 2, "invalid route");
	route--;
	map_gen_taxi_path(g_wow->map, g_current_node - 1, index - 1);
	if ((size_t)route >= g_wow->map->taxi.path_nodes_count)
		return luaL_argerror(L, 2, "invalid route");
	lua_pushnumber(L, 1 - (wow_dbc_get_flt(&g_wow->map->taxi.path_nodes[route]->dbc, 12) - g_wow->map->worldmap.minx) / g_wow->map->worldmap.width);
	return 1;
}

static int luaAPI_TaxiGetSrcY(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: TaxiGetSrcY(index, route)");
	int index = lua_tointeger(L, 1);
	if (index < 1)
		return luaL_argerror(L, 1, "invalid index");
	int route = lua_tointeger(L, 2);
	if (route < 1)
		return luaL_argerror(L, 2, "invalid route");
	route--;
	map_gen_taxi_path(g_wow->map, g_current_node - 1, index - 1);
	if ((size_t)route >= g_wow->map->taxi.path_nodes_count)
		return luaL_argerror(L, 2, "invalid route");
	lua_pushnumber(L, (wow_dbc_get_flt(&g_wow->map->taxi.path_nodes[route]->dbc, 8) - g_wow->map->worldmap.miny) / g_wow->map->worldmap.height);
	return 1;
}

static int luaAPI_TaxiGetDestX(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: TaxiGetDestX(index, route)");
	int index = lua_tointeger(L, 1);
	if (index < 1)
		return luaL_argerror(L, 1, "invalid index");
	int route = lua_tointeger(L, 2);
	if (route < 1)
		return luaL_argerror(L, 2, "invalid route");
	route--;
	map_gen_taxi_path(g_wow->map, g_current_node - 1, index - 1);
	if ((size_t)route + 1 >= g_wow->map->taxi.path_nodes_count)
		return luaL_argerror(L, 2, "invalid route");
	lua_pushnumber(L, 1 - (wow_dbc_get_flt(&g_wow->map->taxi.path_nodes[route + 1]->dbc, 12) - g_wow->map->worldmap.minx) / g_wow->map->worldmap.width);
	return 1;
}

static int luaAPI_TaxiGetDestY(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: TaxiGetDestY(index, route)");
	int index = lua_tointeger(L, 1);
	if (index < 1)
		return luaL_argerror(L, 1, "invalid index");
	int route = lua_tointeger(L, 2);
	if (route < 1)
		return luaL_argerror(L, 2, "invalid route");
	route--;
	map_gen_taxi_path(g_wow->map, g_current_node - 1, index - 1);
	if ((size_t)route + 1 >= g_wow->map->taxi.path_nodes_count)
		return luaL_argerror(L, 2, "invalid route");
	lua_pushnumber(L, (wow_dbc_get_flt(&g_wow->map->taxi.path_nodes[route + 1]->dbc, 8) - g_wow->map->worldmap.miny) / g_wow->map->worldmap.height);
	return 1;
}

static int luaAPI_TaxiNodeCost(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: TaxiNodeCost(index)");
	int index = lua_tointeger(L, 1);
	if (index < 1)
		return luaL_argerror(L, 1, "invalid index");
	map_gen_taxi_path(g_wow->map, g_current_node - 1, index - 1);
	if (!g_wow->map->taxi.path_nodes_count)
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	lua_pushnumber(L, g_wow->map->taxi.path_nodes[0]->pf_price);
	return 1;
}

static int luaAPI_TaxiNodeSetCurrent(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: TaxiNodeSetCurrent(index)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_TakeTaxiNode(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: TaxiNodeSetCurrent(index)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

void register_taxi_functions(lua_State *L)
{
	LUA_REGISTER_FN(SetTaxiMap);
	LUA_REGISTER_FN(NumTaxiNodes);
	LUA_REGISTER_FN(TaxiNodeGetType);
	LUA_REGISTER_FN(TaxiNodePosition);
	LUA_REGISTER_FN(TaxiNodeName);
	LUA_REGISTER_FN(GetNumRoutes);
	LUA_REGISTER_FN(TaxiGetSrcX);
	LUA_REGISTER_FN(TaxiGetSrcY);
	LUA_REGISTER_FN(TaxiGetDestX);
	LUA_REGISTER_FN(TaxiGetDestY);
	LUA_REGISTER_FN(TaxiNodeCost);
	LUA_REGISTER_FN(TaxiNodeSetCurrent);
	LUA_REGISTER_FN(TakeTaxiNode);
}
