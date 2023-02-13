#include "ui/color_select.h"

#include "xml/color_select.h"

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_COLOR_SELECT() LUA_METHOD(ColorSelect, color_select)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_FRAME->region)
#define UI_FRAME (&color_select->frame)

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_frame_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_color_select *color_select = (struct ui_color_select*)object;
	UI_OBJECT->mask |= UI_OBJECT_color_select;
	return true;
}

static void dtr(struct ui_object *object)
{
	ui_frame_vtable.dtr(object);
}

static void render(struct ui_object *object)
{
	ui_frame_vtable.render(object);
}

static bool register_methods(struct jks_array *methods)
{
	/*
	   GetColorHSV
	   GetColorRGB
	   GetColorValueTexture
	   GetColorValueThumbTexture
	   GetColorWheelTexture
	   GetColorWheelThumbTexture
	   SetColorHSV
	   SetColorRGB
	   SetColorValueTexture
	   SetColorValueThumbTexture
	   SetColorWheelTexture
	   SetColorWheelThumbTexture
	 */
	return ui_frame_vtable.register_methods(methods);
}

UI_INH1(frame, void, load_xml, const struct xml_layout_frame*, layout_frame);
UI_INH0(frame, void, post_load);
UI_INH0(frame, void, register_in_interface);
UI_INH0(frame, void, unregister_in_interface);
UI_INH0(frame, void, eval_name);
UI_INH1(frame, void, on_click, enum gfx_mouse_button, button);
UI_INH0(frame, float, get_alpha);
UI_INH1(frame, void, set_alpha, float, alpha);
UI_INH1(frame, void, set_hidden, bool, hidden);
UI_INH2(frame, void, get_size, int32_t*, x, int32_t*, y);
UI_INH0(frame, void, set_dirty_coords);
UI_INH1(frame, void, on_mouse_move, struct gfx_pointer_event*, event);
UI_INH1(frame, void, on_mouse_down, struct gfx_mouse_event*, event);
UI_INH1(frame, void, on_mouse_up, struct gfx_mouse_event*, event);
UI_INH1(frame, void, on_mouse_scroll, struct gfx_scroll_event*, event);
UI_INH1(frame, bool, on_key_down, struct gfx_key_event*, event);
UI_INH1(frame, bool, on_key_up, struct gfx_key_event*, event);
UI_INH0(frame, struct ui_font_instance*, as_font_instance);
UI_INH0(frame, const char *, get_name);

const struct ui_object_vtable ui_color_select_vtable =
{
	UI_OBJECT_VTABLE("ColorSelect")
};
