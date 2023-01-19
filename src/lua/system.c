#include "functions.h"

#include "itf/interface.h"
#include "wow_lua.h"
#include "log.h"
#include "wow.h"

#include <stdint.h>

static int luaAPI_IsWindowsClient(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: IsWindowsClient()");
#ifdef _WIN32
	lua_pushboolean(L, true);
#else
	lua_pushboolean(L, false);
#endif
	return 1;
}

static int luaAPI_IsMacClient(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: IsMacClient()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, false);
	return 1;
}

static int luaAPI_IsLinuxClient(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: IsLinuxClient()");
#ifdef __linux__
	lua_pushboolean(L, true);
#else
	lua_pushboolean(L, false);
#endif
	return 1;
}

static int luaAPI_GetBuildInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetBuildInfo()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "Version");
#ifdef NDEBUG
	lua_pushstring(L, "Release");
#else
	lua_pushstring(L, "Debug");
#endif
	lua_pushstring(L, "2.4.3");
	lua_pushnumber(L, 8606);
	lua_pushstring(L, "Jul 10 2008");
	lua_pushnumber(L, 20400);
	return 6;
}

static int luaAPI_IsDebugBuild(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: IsDebugBuild()");
#ifdef NDEBUG
	lua_pushboolean(L, true);
#else
	lua_pushboolean(L, false);
#endif
	return 1;
}

static int luaAPI_GetRefreshRates(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetRefreshRates()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 60);
	return 1;
}

static int luaAPI_GetCurrentMultisampleFormat(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetCurrentMultisampleFormat()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 1);
	return 0;
}

static int luaAPI_GetMultisampleFormats(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetMultisampleFormats()");
	LUA_UNIMPLEMENTED_FN();
	uint32_t colorsBits[] = {16, 24};
	uint32_t depthBits[] = {24};
	uint32_t multisamples[] = {1, 2, 4, 8};
	for (size_t i = 0; i < sizeof(colorsBits) / sizeof(*colorsBits); ++i)
	{
		for (size_t j = 0; j < sizeof(depthBits) / sizeof(*depthBits); ++j)
		{
			for (size_t k = 0; k < sizeof(multisamples) / sizeof(*multisamples); ++k)
			{
				lua_pushinteger(L, colorsBits[i]);
				lua_pushinteger(L, depthBits[j]);
				lua_pushinteger(L, multisamples[k]);
			}
		}
	}
	return sizeof(colorsBits) / sizeof(*colorsBits) * sizeof(depthBits) / sizeof(*depthBits) * sizeof(multisamples) / sizeof(*multisamples) * 3;
}

static int luaAPI_SetMultisampleFormat(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetMultisampleFormat(index)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetNetStats(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetNetStats()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushnumber(L, 0); //In bandwidth
	lua_pushnumber(L, 0); //Out bandwidth
	lua_pushnumber(L, 0); //Latency
	return 3;
}

static int luaAPI_GetVideoCaps(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetVideoCaps()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 1); //Anisotropics
	lua_pushinteger(L, 1); //Fragment shader
	lua_pushinteger(L, 1); //Vertex shader
	lua_pushinteger(L, 1); //Trilinear
	lua_pushinteger(L, 1); //Triple buffering
	lua_pushinteger(L, 16); //Max anisotropic
	lua_pushinteger(L, 1); //Hardware cursor
	return 7;
}

static int luaAPI_GetGamma(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetGamma()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushnil(L);
	return 1;
}

static int luaAPI_SetGamma(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetGamma(index)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetFramerate(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetFramerate()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushnumber(L, 60);
	return 1;
}

static int luaAPI_GetCurrentResolution(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetCurrentResolution()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 1); //index into resolutions array
	return 1;
}

static int luaAPI_GetLocale(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetLocale()");
	lua_pushstring(L, g_wow->locale);
	return 1;
}

static int luaAPI_GetExistingLocales(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetExistingLocales()");
	lua_createtable(L, 0, 1);
	lua_pushinteger(L, 1);
	lua_pushstring(L, g_wow->locale);
	lua_settable(L, -3);
	return 1;
}

static int luaAPI_GetScreenWidth(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetScreenWidth()");
	lua_pushnumber(L, g_wow->interface->width);
	return 1;
}

static int luaAPI_GetScreenHeight(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetScreenHeight()");
	lua_pushnumber(L, g_wow->interface->height);
	return 1;
}

static int luaAPI_GetScreenResolutions(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetScreenResolutions()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "1920x1080"); //TODO more serious stuff
	return 1;
}

static int luaAPI_SetScreenResolution(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetScreenResolution(index)");
	LUA_UNIMPLEMENTED_FN();
	return 1;
}

static int luaAPI_GetCursorPosition(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetCursorPosition()");
	lua_pushnumber(L, g_wow->interface->width - g_wow->interface->mouse_x);
	lua_pushnumber(L, g_wow->interface->height - g_wow->interface->mouse_y);
	return 2;
}

static int luaAPI_ResetCursor(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: ResetCuror()");
	interface_set_cursor(g_wow->interface, CURSOR_POINT);
	return 0;
}

static int luaAPI_SetCursor(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetCursor(\"cursor\")");
	const char *cursor_str = lua_tostring(L, 1);
	if (!cursor_str)
		return luaL_argerror(L, 1, "failed to get string");
	enum cursor_type cursor;
	if (!cursor_type_from_string(cursor_str, &cursor))
		return luaL_argerror(L, 1, "invalid cursor");
	interface_set_cursor(g_wow->interface, cursor);
	return 0;
}

static int luaAPI_ShowCursor(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: ShowCursor()");
	LUA_UNIMPLEMENTED_FN();
	//gfx_set_native_cursor(g_wow->window, current_cursor);
	return 0;
}

static int luaAPI_HideCursor(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: HideCuror()");
	LUA_UNIMPLEMENTED_FN();
	//gfx_set_native_cursor(g_wow->window, GFX_CURSOR_BLANK);
	return 0;
}

void register_system_functions(lua_State *L)
{
	LUA_REGISTER_FN(GetBuildInfo);
	LUA_REGISTER_FN(IsWindowsClient);
	LUA_REGISTER_FN(IsMacClient);
	LUA_REGISTER_FN(IsLinuxClient);
	LUA_REGISTER_FN(IsDebugBuild);
	LUA_REGISTER_FN(GetRefreshRates);
	LUA_REGISTER_FN(GetCurrentMultisampleFormat);
	LUA_REGISTER_FN(GetMultisampleFormats);
	LUA_REGISTER_FN(SetMultisampleFormat);
	LUA_REGISTER_FN(GetNetStats);
	LUA_REGISTER_FN(GetVideoCaps);
	LUA_REGISTER_FN(GetGamma);
	LUA_REGISTER_FN(SetGamma);
	LUA_REGISTER_FN(GetFramerate);
	LUA_REGISTER_FN(GetCurrentResolution);
	LUA_REGISTER_FN(GetLocale);
	LUA_REGISTER_FN(GetExistingLocales);
	LUA_REGISTER_FN(GetScreenWidth);
	LUA_REGISTER_FN(GetScreenHeight);
	LUA_REGISTER_FN(GetScreenResolutions);
	LUA_REGISTER_FN(SetScreenResolution);
	LUA_REGISTER_FN(GetCursorPosition);
	LUA_REGISTER_FN(ResetCursor);
	LUA_REGISTER_FN(SetCursor);
	LUA_REGISTER_FN(ShowCursor);
	LUA_REGISTER_FN(HideCursor);
}
