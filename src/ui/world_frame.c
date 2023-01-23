#include "ui/world_frame.h"

#include "xml/world_frame.h"

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_WORLD_FRAME() LUA_METHOD(WorldFrame, world_frame)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_FRAME->region)
#define UI_FRAME (&world_frame->frame)

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_frame_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_world_frame *world_frame = (struct ui_world_frame*)object;
	UI_OBJECT->mask |= UI_OBJECT_world_frame;
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
UI_INH1(frame, void, on_mouse_move, gfx_pointer_event_t*, event);
UI_INH1(frame, void, on_mouse_down, gfx_mouse_event_t*, event);
UI_INH1(frame, void, on_mouse_up, gfx_mouse_event_t*, event);
UI_INH1(frame, void, on_mouse_scroll, gfx_scroll_event_t*, event);
UI_INH1(frame, bool, on_key_down, gfx_key_event_t*, event);
UI_INH1(frame, bool, on_key_up, gfx_key_event_t*, event);
UI_INH0(frame, struct ui_font_instance*, as_font_instance);
UI_INH0(frame, const char*, get_name);

const struct ui_object_vtable ui_world_frame_vtable =
{
	UI_OBJECT_VTABLE("WorldFrame")
};
