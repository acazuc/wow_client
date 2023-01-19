#ifndef UI_LUA_FUNCTIONS_H
#define UI_LUA_FUNCTIONS_H

#define LUA_REGISTER_FN(fn) lua_register(L, #fn, luaAPI_##fn)

//#define LUA_VERBOSE_ENABLE

#ifdef LUA_VERBOSE_ENABLE
# include "log.h"
# define LUA_VERBOSE_FN() LOG_DEBUG("%s called", __func__)
#else
# define LUA_VERBOSE_FN()
#endif

//#define LUA_UNIMPLEMENTED_VERBOSE
#ifdef LUA_UNIMPLEMENTED_VERBOSE
# include "log.h"
# define LUA_UNIMPLEMENTED_FN() LOG_DEBUG("unimplemented function: %s", __func__);
#else
# define LUA_UNIMPLEMENTED_FN()
#endif

#include <stddef.h>

typedef struct lua_State lua_State;

typedef void (*register_lua_functions_t)(lua_State *L);

void register_misc_functions(lua_State *L);
void register_std_functions(lua_State *L);
void register_objects_functions(lua_State *L);
void register_unit_functions(lua_State *L);
void register_pet_functions(lua_State *L);
void register_guild_functions(lua_State *L);
void register_system_functions(lua_State *L);
void register_voice_functions(lua_State *L);
void register_party_functions(lua_State *L);
void register_input_functions(lua_State *L);
void register_audio_functions(lua_State *L);
void register_loot_functions(lua_State *L);
void register_secure_functions(lua_State *L);
void register_battle_ground_functions(lua_State *L);
void register_chat_functions(lua_State *L);
void register_addon_functions(lua_State *L);
void register_lfg_functions(lua_State *L);
void register_skill_functions(lua_State *L);
void register_quest_functions(lua_State *L);
void register_world_map_functions(lua_State *L);
void register_arena_functions(lua_State *L);
void register_glue_functions(lua_State *L);
void register_social_functions(lua_State *L);
void register_kb_functions(lua_State *L);
void register_spell_functions(lua_State *L);
void register_pvp_functions(lua_State *L);
void register_talent_functions(lua_State *L);
void register_auction_functions(lua_State *L);
void register_macro_functions(lua_State *L);
void register_inbox_functions(lua_State *L);
void register_trade_functions(lua_State *L);
void register_combat_log_functions(lua_State *L);
void register_craft_functions(lua_State *L);
void register_bindings_functions(lua_State *L);
void register_taxi_functions(lua_State *L);

void lua_format(lua_State *L, int arg_idx, char *buffer, size_t buffer_size);
struct unit *lua_get_unit(lua_State *L, int arg);

#endif
