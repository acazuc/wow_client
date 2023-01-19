#include "functions.h"

#include "wow_lua.h"
#include "log.h"
#include "wow.h"
#include "dbc.h"

static int luaAPI_GetNumMacros(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetNumMacros()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushnumber(L, 10);
	lua_pushnumber(L, 10);
	return 2;
}

static int luaAPI_GetMacroInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetMacroInfo(index)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "test");
	lua_pushstring(L, "Interface\\Icons\\Spell_Shadow_SoulGem.blp");
	lua_pushstring(L, "/run coucou mdr");
	lua_pushboolean(L, true);
	return 4;
}

static int luaAPI_EditMacro(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc < 3 || argc > 4)
		return luaL_error(L, "Usage: EditMacro(index, \"name\", \"icon\"[, \"text\"])");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetNumMacroIcons(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetNumMacroIcons()");
	lua_pushnumber(L, g_wow->dbc.spell_icon->file->header.record_count);
	return 1;
}

static int luaAPI_GetMacroIconInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetMacroIconInfo(index)");
	int index = lua_tointeger(L, 1);
	index--;
	if (index < 0 || (unsigned)index >= g_wow->dbc.spell_icon->file->header.record_count)
		return luaL_argerror(L, 1, "invalid index");
	wow_dbc_row_t row = dbc_get_row(g_wow->dbc.spell_icon, index);
	lua_pushstring(L, wow_dbc_get_str(&row, 4));
	return 1;
}

void register_macro_functions(lua_State *L)
{
	LUA_REGISTER_FN(GetNumMacros);
	LUA_REGISTER_FN(GetMacroInfo);
	LUA_REGISTER_FN(EditMacro);
	LUA_REGISTER_FN(GetNumMacroIcons);
	LUA_REGISTER_FN(GetMacroIconInfo);
}
