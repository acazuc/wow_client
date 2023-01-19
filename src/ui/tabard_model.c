#include "tabard_model.h"

#include "xml/tabard_model.h"

#include "wow_lua.h"
#include "log.h"

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_TABARD_MODEL() LUA_METHOD(TabardModel, tabard_model)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_FRAME->region)
#define UI_FRAME (&UI_MODEL->frame)
#define UI_MODEL (&UI_PLAYER_MODEL->model)
#define UI_PLAYER_MODEL (&tabard_model->player_model)

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_player_model_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_tabard_model *tabard_model = (struct ui_tabard_model*)object;
	UI_OBJECT->mask |= UI_OBJECT_tabard_model;
	return true;
}

static void dtr(struct ui_object *object)
{
	ui_player_model_vtable.dtr(object);
}

static void render(struct ui_object *object)
{
	ui_player_model_vtable.render(object);
}

static int lua_Save(lua_State *L)
{
	LUA_METHOD_TABARD_MODEL();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static bool register_methods(struct jks_array *methods)
{
	/*
	   CanSaveTabardNow
	   CycleVariation
	   GetLowerBackgroundFileName
	   GetLowerEmblemFileName
	   GetLowerEmblemTexture
	   GetUpperBackgroundFileName
	   GetUpperEmblemFileName
	   GetUpperEmblemTexture
	   InitializeTabardColors
	 */
	UI_REGISTER_METHOD(Save);
	return ui_player_model_vtable.register_methods(methods);
}

UI_INH1(player_model, void, load_xml, const struct xml_layout_frame*, layout_frame);
UI_INH0(player_model, void, post_load);
UI_INH0(player_model, void, register_in_interface);
UI_INH0(player_model, void, unregister_in_interface);
UI_INH0(player_model, void, eval_name);
UI_INH1(player_model, void, on_click, enum gfx_mouse_button, button);
UI_INH0(player_model, float, get_alpha);
UI_INH1(player_model, void, set_alpha, float, alpha);
UI_INH1(player_model, void, set_hidden, bool, hidden);
UI_INH2(player_model, void, get_size, int32_t*, x, int32_t*, y);
UI_INH0(player_model, void, set_dirty_coords);
UI_INH1(player_model, void, on_mouse_move, gfx_pointer_event_t*, event);
UI_INH1(player_model, void, on_mouse_down, gfx_mouse_event_t*, event);
UI_INH1(player_model, void, on_mouse_up, gfx_mouse_event_t*, event);
UI_INH1(player_model, void, on_mouse_scroll, gfx_scroll_event_t*, event);
UI_INH1(player_model, bool, on_key_down, gfx_key_event_t*, event);
UI_INH1(player_model, bool, on_key_up, gfx_key_event_t*, event);
UI_INH0(player_model, struct ui_font_instance*, as_font_instance);
UI_INH0(player_model, const char*, get_name);

const struct ui_object_vtable ui_tabard_model_vtable =
{
	UI_OBJECT_VTABLE("TabardModel")
};
