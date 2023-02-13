#include "ui/dress_up_model.h"

#include "xml/dress_up_model.h"

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_DRESS_UP_MODEL() LUA_METHOD(DressUpModel, dress_up_model)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_FRAME->region)
#define UI_FRAME (&UI_MODEL->frame)
#define UI_MODEL (&UI_PLAYER_MODEL->model)
#define UI_PLAYER_MODEL (&dress_up_model->player_model)

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_player_model_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_dress_up_model *dress_up_model = (struct ui_dress_up_model*)object;
	UI_OBJECT->mask |= UI_OBJECT_dress_up_model;
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

static bool register_methods(struct jks_array *methods)
{
	/*
	   Dress
	   TryOn
	   Undress
	 */
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
UI_INH1(player_model, void, on_mouse_move, struct gfx_pointer_event*, event);
UI_INH1(player_model, void, on_mouse_down, struct gfx_mouse_event*, event);
UI_INH1(player_model, void, on_mouse_up, struct gfx_mouse_event*, event);
UI_INH1(player_model, void, on_mouse_scroll, struct gfx_scroll_event*, event);
UI_INH1(player_model, bool, on_key_down, struct gfx_key_event*, event);
UI_INH1(player_model, bool, on_key_up, struct gfx_key_event*, event);
UI_INH0(player_model, struct ui_font_instance*, as_font_instance);
UI_INH0(player_model, const char*, get_name);

const struct ui_object_vtable ui_dress_up_model_vtable =
{
	UI_OBJECT_VTABLE("DressUpModel")
};
