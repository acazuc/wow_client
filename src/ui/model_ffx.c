#include "model_ffx.h"

#include "xml/model_ffx.h"

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_MODEL_FFX() LUA_METHOD(ModelFFX, model_ffx)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_FRAME->region)
#define UI_FRAME (&UI_MODEL->frame)
#define UI_MODEL (&model_ffx->model)

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_model_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_model_ffx *model_ffx = (struct ui_model_ffx*)object;
	UI_OBJECT->mask |= UI_OBJECT_model_ffx;
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

static bool register_methods(struct jks_array *methods)
{
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

const struct ui_object_vtable ui_model_ffx_vtable =
{
	UI_OBJECT_VTABLE("ModelFFX")
};
