#include "functions.h"

#include "itf/interface.h"

#include "ui/scrolling_message_frame.h"
#include "ui/taxi_route_frame.h"
#include "ui/dress_up_model.h"
#include "ui/message_frame.h"
#include "ui/check_button.h"
#include "ui/player_model.h"
#include "ui/tabard_model.h"
#include "ui/color_select.h"
#include "ui/scroll_frame.h"
#include "ui/game_tooltip.h"
#include "ui/simple_html.h"
#include "ui/font_string.h"
#include "ui/movie_frame.h"
#include "ui/world_frame.h"
#include "ui/status_bar.h"
#include "ui/model_ffx.h"
#include "ui/cooldown.h"
#include "ui/edit_box.h"
#include "ui/minimap.h"
#include "ui/texture.h"
#include "ui/slider.h"
#include "ui/button.h"
#include "ui/frame.h"
#include "ui/model.h"
#include "ui/font.h"

#include "wow_lua.h"
#include "memory.h"
#include "log.h"

#include <string.h>

#define OBJECT_REGISTER(obj_name) \
	do { \
		struct jks_array methods; \
		jks_array_init(&methods, sizeof(luaL_Reg), NULL, &jks_array_memory_fn_LUA); \
		if (!ui_##obj_name##_vtable.register_methods(&methods)) \
		{ \
			jks_array_destroy(&methods); \
			LOG_ERROR("failed to create vtable for %s", ui_##obj_name##_vtable.name); \
			break; \
		} \
		luaL_Reg tmp = {NULL, NULL}; \
		if (!jks_array_push_back(&methods, &tmp)) \
		{ \
			jks_array_destroy(&methods); \
			LOG_ERROR("failed to create vtable for %s", ui_##obj_name##_vtable.name); \
			break; \
		} \
		register_metatable(L, ui_##obj_name##_vtable.name, ui_object_metatable, &methods); \
		jks_array_destroy(&methods); \
	} while (0)

MEMORY_DECL(LUA);

LUALIB_API void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup) {
  luaL_checkstack(L, nup, "too many upvalues");
  for (; l->name != NULL; l++) {  /* fill the table with given functions */
    if (l->func == NULL)  /* place holder? */
      lua_pushboolean(L, 0);
    else {
      int i;
      for (i = 0; i < nup; i++)  /* copy upvalues to the top */
        lua_pushvalue(L, -nup);
      lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
    }
    lua_setfield(L, -(nup + 2), l->name);
  }
  lua_pop(L, nup);  /* remove upvalues */
}

static void register_metatable(lua_State *L, const char *name, const luaL_Reg *metatable, const struct jks_array *methods)
{
	size_t metatable_size = 0;
	while (metatable[metatable_size].func != NULL)
		++metatable_size;
	int table = luaL_newmetatable(L, name);
	int meta_id = lua_gettop(L);
	luaL_setfuncs(L, metatable, 0);

	lua_createtable(L, 0, methods->size);
	luaL_setfuncs(L, (const luaL_Reg*)methods->data, 0);
	lua_setfield(L, meta_id, "__index");

	lua_createtable(L, 0, metatable_size);
	luaL_setfuncs(L, metatable, 0);
	lua_setfield(L, meta_id, "__metatable");
	lua_pop(L, 1);
}

void register_objects_functions(lua_State *L)
{
	OBJECT_REGISTER(button);
	OBJECT_REGISTER(check_button);
	OBJECT_REGISTER(color_select);
	OBJECT_REGISTER(cooldown);
	OBJECT_REGISTER(dress_up_model);
	OBJECT_REGISTER(edit_box);
	OBJECT_REGISTER(font);
	OBJECT_REGISTER(font_string);
	OBJECT_REGISTER(frame);
	OBJECT_REGISTER(game_tooltip);
	OBJECT_REGISTER(message_frame);
	OBJECT_REGISTER(minimap);
	OBJECT_REGISTER(model);
	OBJECT_REGISTER(model_ffx);
	OBJECT_REGISTER(movie_frame);
	OBJECT_REGISTER(player_model);
	OBJECT_REGISTER(scroll_frame);
	OBJECT_REGISTER(scrolling_message_frame);
	OBJECT_REGISTER(simple_html);
	OBJECT_REGISTER(slider);
	OBJECT_REGISTER(status_bar);
	OBJECT_REGISTER(tabard_model);
	OBJECT_REGISTER(taxi_route_frame);
	OBJECT_REGISTER(texture);
	OBJECT_REGISTER(world_frame);
}
