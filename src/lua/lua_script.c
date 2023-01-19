#include "lua_script.h"

#include "itf/interface.h"

#include "wow_lua.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <string.h>

static void print_error(struct lua_script *script);

struct lua_script *lua_script_new(lua_State *L, const char *source, const char *name)
{
	struct lua_script *script = mem_malloc(MEM_LUA, sizeof(*script));
	if (!script)
	{
		LOG_ERROR("failed to malloc script");
		return NULL;
	}
	script->L = L;
	int len = strlen(source);
	if (len >= 3 && !strncmp(source, "\xef\xbb\xbf", 3))
	{
		source += 3;
		len -= 3;
	}
	int stack_size = lua_gettop(L);
	int err = luaL_loadbuffer(L, source, len, name);
	if (err)
	{
		lua_settop(L, stack_size);
		print_error(script);
		LOG_ERROR("failed to load lua buffer: %d", err);
		script->ref = LUA_NOREF;
		return script;
	}
	lua_settop(L, stack_size + 1);
	script->ref = luaL_ref(L, LUA_REGISTRYINDEX);
	return script;
}

struct lua_script *lua_script_new_ref(lua_State *L, int ref)
{
	struct lua_script *script = mem_malloc(MEM_LUA, sizeof(*script));
	if (!script)
	{
		LOG_ERROR("failed to malloc script");
		return NULL;
	}
	script->L = L;
	script->ref = ref;
	return script;
}

struct lua_script *lua_script_dup(struct lua_script *dup)
{
	struct lua_script *script = mem_malloc(MEM_LUA, sizeof(*script));
	if (!script)
	{
		LOG_ERROR("failed to malloc script");
		return NULL;
	}
	script->L = dup->L;
	lua_rawgeti(script->L, LUA_REGISTRYINDEX, dup->ref);
	script->ref = luaL_ref(script->L, LUA_REGISTRYINDEX);
	return script;
}

void lua_script_delete(struct lua_script *script)
{
	if (!script)
		return;
	if (script->ref != LUA_NOREF && script->ref != LUA_REFNIL)
		luaL_unref(script->L, LUA_REGISTRYINDEX, script->ref);
	mem_free(MEM_LUA, script);
}

static void print_error(struct lua_script *script)
{
	const char *s = lua_tostring(script->L, -1);
	if (!s)
	{
		LOG_ERROR("lua error with NULL");
		return;
	}
	LOG_ERROR("lua error: %s", s);
}

void lua_script_push_pointer(struct lua_script *script)
{
	lua_rawgeti(script->L, LUA_REGISTRYINDEX, script->ref);
}

bool lua_script_call(struct lua_script *script, int nargs, int nresults)
{
	int stack_top = lua_gettop(script->L);
	int ret = lua_pcall(script->L, nargs, nresults, 0);
	if (ret)
	{
		print_error(script);
		struct lua_script *error_script = g_wow->interface->error_script;
		if (error_script && error_script != script)
		{
			lua_script_push_pointer(error_script);
			lua_pushvalue(script->L, -2);
			lua_script_call(error_script, 1, 0);
		}
		lua_settop(script->L, stack_top - 1 - nargs);
		return false;
	}
	lua_settop(script->L, stack_top - 1 - nargs);
	return true;
}

bool lua_script_execute(struct lua_script *script)
{
	lua_script_push_pointer(script);
	return lua_script_call(script, 0, 0);
}
