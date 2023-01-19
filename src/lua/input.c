#include "functions.h"

#include "wow_lua.h"
#include "log.h"
#include "wow.h"

#include <gfx/window.h>

static int luaAPI_IsModifiedClick(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc > 1)
		return luaL_error(L, "Usage: IsModifiedClick([\"action\"])");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, false);
	return 1;
}

static int luaAPI_GetModifiedClick(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetModifiedClick(\"action\")");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "NONE"); //ALT, CTRL, SHIFT, NONE
	return 1;
}

static int luaAPI_IsModifierKeyDown(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: IsModifierKeyDown()");
	lua_pushboolean(L, gfx_is_key_down(g_wow->window, GFX_KEY_LSHIFT) || gfx_is_key_down(g_wow->window, GFX_KEY_RSHIFT)
	                || gfx_is_key_down(g_wow->window, GFX_KEY_LCONTROL) || gfx_is_key_down(g_wow->window, GFX_KEY_RCONTROL)
	                || gfx_is_key_down(g_wow->window, GFX_KEY_LALT) || gfx_is_key_down(g_wow->window, GFX_KEY_RALT));
	return 1;
}

static int luaAPI_IsShiftKeyDown(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: IsShiftKeyDown()");
	lua_pushboolean(L, gfx_is_key_down(g_wow->window, GFX_KEY_LSHIFT) || gfx_is_key_down(g_wow->window, GFX_KEY_RSHIFT));
	return 1;
}

static int luaAPI_IsLeftShiftKeyDown(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: IsLeftShiftKeyDown()");
	lua_pushboolean(L, gfx_is_key_down(g_wow->window, GFX_KEY_LSHIFT));
	return 1;
}

static int luaAPI_IsRightShiftKeyDown(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: IsRightShiftKeyDown()");
	lua_pushboolean(L, gfx_is_key_down(g_wow->window, GFX_KEY_RSHIFT));
	return 1;
}

static int luaAPI_IsControlKeyDown(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: IsControlKeyDown()");
	lua_pushboolean(L, gfx_is_key_down(g_wow->window, GFX_KEY_LCONTROL) || gfx_is_key_down(g_wow->window, GFX_KEY_RCONTROL));
	return 1;
}

static int luaAPI_IsLeftControlKeyDown(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: IsLeftControlKeyDown()");
	lua_pushboolean(L, gfx_is_key_down(g_wow->window, GFX_KEY_LCONTROL));
	return 1;
}

static int luaAPI_IsRightControlKeyDown(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: IsRightControlKeyDown()");
	lua_pushboolean(L, gfx_is_key_down(g_wow->window, GFX_KEY_RCONTROL));
	return 1;
}

static int luaAPI_IsAltKeyDown(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: IsAltKeyDown()");
	lua_pushboolean(L, gfx_is_key_down(g_wow->window, GFX_KEY_LALT) || gfx_is_key_down(g_wow->window, GFX_KEY_RALT));
	return 1;
}

static int luaAPI_IsLeftAltKeyDown(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: IsLeftAltKeyDown()");
	lua_pushboolean(L, gfx_is_key_down(g_wow->window, GFX_KEY_LALT));
	return 1;
}

static int luaAPI_IsRightAltKeyDown(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: IsRightAltKeyDown()");
	lua_pushboolean(L, gfx_is_key_down(g_wow->window, GFX_KEY_RALT));
	return 1;
}

void register_input_functions(lua_State *L)
{
	LUA_REGISTER_FN(IsModifiedClick);
	LUA_REGISTER_FN(IsModifierKeyDown);
	LUA_REGISTER_FN(IsShiftKeyDown);
	LUA_REGISTER_FN(IsLeftShiftKeyDown);
	LUA_REGISTER_FN(IsRightShiftKeyDown);
	LUA_REGISTER_FN(IsControlKeyDown);
	LUA_REGISTER_FN(IsLeftControlKeyDown);
	LUA_REGISTER_FN(IsRightControlKeyDown);
	LUA_REGISTER_FN(IsAltKeyDown);
	LUA_REGISTER_FN(IsLeftAltKeyDown);
	LUA_REGISTER_FN(IsRightAltKeyDown);
	LUA_REGISTER_FN(GetModifiedClick);
}
