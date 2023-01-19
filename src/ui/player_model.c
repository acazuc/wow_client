#include "player_model.h"

#include "xml/player_model.h"

#include "wow_lua.h"
#include "log.h"

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_PLAYER_MODEL() LUA_METHOD(PlayerModel, player_model)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_FRAME->region)
#define UI_FRAME (&UI_MODEL->frame)
#define UI_MODEL (&player_model->model)

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_model_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_player_model *player_model = (struct ui_player_model*)object;
	UI_OBJECT->mask |= UI_OBJECT_player_model;
	return true;
}

static void dtr(struct ui_object *object)
{
	ui_model_vtable.dtr(object);
}

static void render(struct ui_object *object)
{
	ui_model_vtable.render(object);
}

static int lua_SetRotation(lua_State *L)
{
	LUA_METHOD_PLAYER_MODEL();
	if (argc != 2)
		return luaL_error(L, "Usage: PlayerModel:SetRotation(rotation)");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_SetUnit(lua_State *L)
{
	LUA_METHOD_PLAYER_MODEL();
	if (argc != 2)
		return luaL_error(L, "Usage: PlayerModel:SetUnit(\"unit\")");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_RefreshUnit(lua_State *L)
{
	LUA_METHOD_PLAYER_MODEL();
	if (argc != 1)
		return luaL_error(L, "Usage: RefreshUnit()");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static bool register_methods(struct jks_array *methods)
{
	/*
	   SetCreature
	   SetDisplayInfo
	 */
	UI_REGISTER_METHOD(SetRotation);
	UI_REGISTER_METHOD(SetUnit);
	UI_REGISTER_METHOD(RefreshUnit);
	return ui_model_vtable.register_methods(methods);
}

UI_INH1(model, void, load_xml, const struct xml_layout_frame*, layout_frame);
UI_INH0(model, void, post_load);
UI_INH0(model, void, register_in_interface);
UI_INH0(model, void, unregister_in_interface);
UI_INH0(model, void, eval_name);
UI_INH1(model, void, on_click, enum gfx_mouse_button, button);
UI_INH0(model, float, get_alpha);
UI_INH1(model, void, set_alpha, float, alpha);
UI_INH1(model, void, set_hidden, bool, hidden);
UI_INH2(model, void, get_size, int32_t*, x, int32_t*, y);
UI_INH0(model, void, set_dirty_coords);
UI_INH1(model, void, on_mouse_move, gfx_pointer_event_t*, event);
UI_INH1(model, void, on_mouse_down, gfx_mouse_event_t*, event);
UI_INH1(model, void, on_mouse_up, gfx_mouse_event_t*, event);
UI_INH1(model, void, on_mouse_scroll, gfx_scroll_event_t*, event);
UI_INH1(model, bool, on_key_down, gfx_key_event_t*, event);
UI_INH1(model, bool, on_key_up, gfx_key_event_t*, event);
UI_INH0(model, struct ui_font_instance*, as_font_instance);
UI_INH0(model, const char*, get_name);

const struct ui_object_vtable ui_player_model_vtable =
{
	UI_OBJECT_VTABLE("PlayerModel")
};
