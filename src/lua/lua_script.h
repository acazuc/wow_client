#ifndef UI_LUA_SCRIPT_H
#define UI_LUA_SCRIPT_H

#include <stdbool.h>

typedef struct lua_State lua_State;

struct lua_script
{
	lua_State *L;
	int ref;
};

struct lua_script *lua_script_new(lua_State *L, const char *script, const char *name);
struct lua_script *lua_script_new_ref(lua_State *L, int ref);
struct lua_script *lua_script_dup(struct lua_script *script);
void lua_script_delete(struct lua_script *script);
void lua_script_push_pointer(struct lua_script *script);
bool lua_script_call(struct lua_script *script, int nargs, int nresults);
bool lua_script_execute(struct lua_script *script);

#endif
