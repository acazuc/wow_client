#define LUA_UNIMPLEMENTED_VERBOSE
#include "functions.h"

#include "itf/interface.h"
#include "itf/addon.h"

#include "wow_lua.h"
#include "log.h"
#include "wow.h"

#include <libwow/toc.h>
#include <string.h>

static bool enable_version_check = false;

static int luaAPI_IsAddOnLoaded(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: IsAddOnLoaded(\"Addon\")");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, false);
	return 1;
}

static int luaAPI_GetNumAddOns(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetNumAddOns()");
	lua_pushinteger(L, g_wow->interface->ext_addons.size);
	return 1;
}

static int luaAPI_GetAddOnMemoryUsage(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetAddOnMemoryUsage(index)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 0);
	return 1;
}

static int luaAPI_EnableAllAddOns(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: EnableAllAddOns(character)");
	for (size_t i = 0; i < g_wow->interface->ext_addons.size; ++i)
		(*JKS_ARRAY_GET(&g_wow->interface->ext_addons, i, struct addon*))->enabled = true;
	return 0;
}

static int luaAPI_DisableAllAddOns(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: DisableAllAddOns(character)");
	for (size_t i = 0; i < g_wow->interface->ext_addons.size; ++i)
		(*JKS_ARRAY_GET(&g_wow->interface->ext_addons, i, struct addon*))->enabled = false;
	return 0;
}

static int luaAPI_EnableAddOn(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc == 1)
	{
		const char *addon_name = lua_tostring(L, 1);
		if (addon_name == NULL)
			return luaL_argerror(L, 1, "failed to get string");
		struct addon *addon = interface_get_addon(g_wow->interface, addon_name);
		if (addon == NULL || addon->source != ADDON_EXT)
		{
			LOG_ERROR("can't find addon: %s", addon_name);
			return luaL_argerror(L, 1, "unknown addon");
		}
		addon->enabled = true;
	}
	else if (argc == 2)
	{
		if (!lua_isinteger(L, 2))
			return luaL_argerror(L, 2, "integer expected");
		int index = lua_tointeger(L, 2);
		if (index < 0 || (size_t)index > g_wow->interface->ext_addons.size)
			return luaL_argerror(L, 2, "out of range");
		(*JKS_ARRAY_GET(&g_wow->interface->ext_addons, index - 1, struct addon*))->enabled = true;
	}
	else
	{
		return luaL_error(L, "Usage: EnableAddOn([\"addon\"] | [character, index])");
	}
	return 0;
}

static int luaAPI_DisableAddOn(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc == 1)
	{
		const char *addon_name = lua_tostring(L, 1);
		if (addon_name == NULL)
			return luaL_argerror(L, 1, "failed to get string");
		struct addon *addon = interface_get_addon(g_wow->interface, addon_name);
		if (addon == NULL || addon->source != ADDON_EXT)
		{
			LOG_ERROR("can't find addon: %s", addon->name);
			return luaL_argerror(L, 1, "unknown addon");
		}
		addon->enabled = false;
	}
	else if (argc == 2)
	{
		if (!lua_isinteger(L, 2))
			return luaL_argerror(L, 2, "integer expected");
		int index = lua_tointeger(L, 2);
		if (index < 0 || (size_t)index > g_wow->interface->ext_addons.size)
			return luaL_argerror(L, 2, "out of range");
		(*JKS_ARRAY_GET(&g_wow->interface->ext_addons, index - 1, struct addon*))->enabled = false;
	}
	else
	{
		return luaL_error(L, "Usage: DisableAddOn([\"addon\"] | [character, index])");
	}
	return 0;
}

static int luaAPI_LoadAddOn(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: LoadAddOn(index)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true); //loaded
	lua_pushstring(L, "invalid"); //reason
	return 2;
}

static int luaAPI_GetAddOnInfo(lua_State *L)
{
	static const char *states_str[] =
	{
		[ADDON_ENABLED]               = NULL,
		[ADDON_BANNED]                = "BANNED",
		[ADDON_CORRUPT]               = "CORRUPT",
		[ADDON_DISABLED]              = "DISABLED",
		[ADDON_INCOMPATIBLE]          = "INCOMPATIBLE",
		[ADDON_INSECURE]              = "INSECURE",
		[ADDON_INTERFACE_VERSION]     = "INTERFACE_VERSION",
		[ADDON_DEP_BANNED]            = "DEP_BANNED",
		[ADDON_DEP_CORRUPT]           = "DEP_CORRUPT",
		[ADDON_DEP_DISABLED]          = "DEP_DISABLED",
		[ADDON_DEP_INCOMPATIBLE]      = "DEP_INCOMPATIBLE",
		[ADDON_DEP_INSECURE]          = "DEP_INSECURE",
		[ADDON_DEP_INTERFACE_VERSION] = "DEP_INTERFACE_VERSION",
		[ADDON_DEP_MISSING]           = "DEP_MISSING",
	};
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetAddOnInfo(index)");
	LUA_UNIMPLEMENTED_FN();
	struct addon *addon;
	if (lua_isinteger(L, 1))
	{
		int index = lua_tointeger(L, 1);
		if (index < 1 || (size_t)index > g_wow->interface->ext_addons.size)
			return luaL_argerror(L, 1, "out of range");
		addon = *JKS_ARRAY_GET(&g_wow->interface->ext_addons, index - 1, struct addon*);
	}
	else if (lua_isstring(L, 1))
	{
		const char *addon_name = lua_tostring(L, 1);
		addon = interface_get_addon(g_wow->interface, addon_name);
		if (!addon)
		{
			LOG_ERROR("can't find addon %s", addon_name);
			return 0;
		}
	}
	else
	{
		return luaL_argerror(L, 1, "invalid parameter");
	}
	enum addon_state state = addon_get_state(addon);
	if (!enable_version_check && (state == ADDON_INTERFACE_VERSION || state == ADDON_DEP_INTERFACE_VERSION))
		state = ADDON_ENABLED;
	lua_pushstring(L, addon->name);
	lua_pushstring(L, addon->toc->title);
	lua_pushstring(L, addon->toc->notes);
	lua_pushstring(L, addon->toc->url);
	lua_pushboolean(L, state == ADDON_ENABLED);
	lua_pushstring(L, state == ADDON_ENABLED ? NULL : states_str[state]);
	lua_pushstring(L, "SECURE"); //security: SECURE / UNSECURE / BANNED
	lua_pushboolean(L, false); //new version
	return 8;
}

static int luaAPI_IsAddOnLoadOnDemand(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: IsAddOnLoadOnDemand(\"addon\")");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true);
	return 1;
}

static int luaAPI_IsAddonVersionCheckEnabled(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: IsAddonVersionCheckEnabled()");
	lua_pushboolean(L, enable_version_check);
	return 1;
}

static int luaAPI_GetAddOnDependencies(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetAddOnDependencies(index)");
	if (!lua_isinteger(L, 1))
		return luaL_argerror(L, 1, "integer expected");
	int index = lua_tointeger(L, 1);
	if (index < 1 || (unsigned)index > g_wow->interface->ext_addons.size)
		return luaL_argerror(L, 1, "out of range");
	struct addon *addon = *JKS_ARRAY_GET(&g_wow->interface->ext_addons, index - 1, struct addon*);
	for (size_t i = 0; i < addon->dependencies.size; ++i)
		lua_pushstring(L, *JKS_ARRAY_GET(&addon->dependencies, i, const char*));
	return addon->dependencies.size;
}

static int luaAPI_GetAddOnEnableState(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: GetAddOnEnableState(character, addon)");
	int index = lua_tointeger(L, 2);
	if (index < 1 || (size_t)index > g_wow->interface->ext_addons.size)
		return luaL_argerror(L, 2, "invalid addon index");
	struct addon *addon = *JKS_ARRAY_GET(&g_wow->interface->ext_addons, index - 1, struct addon*);
	lua_pushinteger(L, addon->enabled); // 0: disabled, 1: enabled for some, 2: enabled for all
	return 1;
}

static int luaAPI_SetAddonVersionCheck(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetAddonVersionCheck(state)");
	enable_version_check = lua_toboolean(L, 1);
	return 0;
}

static int luaAPI_SaveAddOns(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: SaveAddOns()");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_ResetAddOns(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: ResetAddOns()");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_LaunchAddOnURL(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: LaunchAddOnURL(addon)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

void register_addon_functions(lua_State *L)
{
	LUA_REGISTER_FN(IsAddOnLoaded);
	LUA_REGISTER_FN(GetNumAddOns);
	LUA_REGISTER_FN(GetAddOnMemoryUsage);
	LUA_REGISTER_FN(EnableAllAddOns);
	LUA_REGISTER_FN(DisableAllAddOns);
	LUA_REGISTER_FN(EnableAddOn);
	LUA_REGISTER_FN(DisableAddOn);
	LUA_REGISTER_FN(LoadAddOn);
	LUA_REGISTER_FN(GetAddOnInfo);
	LUA_REGISTER_FN(IsAddOnLoadOnDemand);
	LUA_REGISTER_FN(IsAddonVersionCheckEnabled);
	LUA_REGISTER_FN(GetAddOnDependencies);
	LUA_REGISTER_FN(GetAddOnEnableState);
	LUA_REGISTER_FN(SetAddonVersionCheck);
	LUA_REGISTER_FN(SaveAddOns);
	LUA_REGISTER_FN(ResetAddOns);
	LUA_REGISTER_FN(LaunchAddOnURL);
}
