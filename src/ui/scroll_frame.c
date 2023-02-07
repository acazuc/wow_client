#include "ui/scroll_frame.h"
#include "ui/backdrop.h"

#include "itf/interface.h"

#include "xml/scroll_frame.h"

#include "wow_lua.h"
#include "log.h"
#include "wow.h"

#include <gfx/device.h>

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_SCROLL_FRAME() LUA_METHOD(ScrollFrame, scroll_frame)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_FRAME->region)
#define UI_FRAME (&scroll_frame->frame)

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_frame_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_scroll_frame *scroll_frame = (struct ui_scroll_frame*)object;
	UI_OBJECT->mask |= UI_OBJECT_scroll_frame;
	scroll_frame->scroll_child = NULL;
	scroll_frame->vertical_scroll = 0;
	scroll_frame->horizontal_scroll = 0;
	return true;
}

static void dtr(struct ui_object *object)
{
	ui_frame_vtable.dtr(object);
}

static void load_xml(struct ui_object *object, const struct xml_layout_frame *layout_frame)
{
	ui_frame_vtable.load_xml(object, layout_frame);
	struct ui_scroll_frame *scroll_frame = (struct ui_scroll_frame*)object;
	const struct xml_scroll_frame *xml_scroll_frame = xml_clayout_frame_as_scroll_frame(layout_frame);
	if (xml_scroll_frame)
	{
		if (OPTIONAL_ISSET(xml_scroll_frame->scroll_child) && OPTIONAL_GET(xml_scroll_frame->scroll_child).frame)
		{
			ui_object_delete((struct ui_object*)scroll_frame->scroll_child);
			struct xml_layout_frame *child = OPTIONAL_GET(xml_scroll_frame->scroll_child).frame;
			scroll_frame->scroll_child = (struct ui_frame*)xml_load_interface(child, UI_OBJECT->interface, UI_REGION);
			ui_object_load_xml((struct ui_object*)scroll_frame->scroll_child, OPTIONAL_GET(xml_scroll_frame->scroll_child).frame);
			if (!jks_array_push_back(&UI_FRAME->regions_to_load, &scroll_frame->scroll_child))
				LOG_ERROR("failed to push region to load");
		}
	}
}

static void post_load(struct ui_object *object)
{
	struct ui_scroll_frame *scroll_frame = (struct ui_scroll_frame*)object;
	ui_frame_vtable.post_load(object);
	if (scroll_frame->scroll_child)
	{
		ui_region_clear_anchors((struct ui_region*)scroll_frame->scroll_child);
		ui_region_add_anchor((struct ui_region*)scroll_frame->scroll_child, ui_anchor_new(UI_OBJECT->interface, NULL, ANCHOR_LEFT, ANCHOR_LEFT, "$parent", (struct ui_region*)scroll_frame->scroll_child));
		ui_region_add_anchor((struct ui_region*)scroll_frame->scroll_child, ui_anchor_new(UI_OBJECT->interface, NULL, ANCHOR_TOP, ANCHOR_TOP, "$parent", (struct ui_region*)scroll_frame->scroll_child));
	}
}

static void render(struct ui_object *object)
{
	struct ui_scroll_frame *scroll_frame = (struct ui_scroll_frame*)object;
	if (UI_REGION->hidden)
		return;
	struct lua_State *L = ui_object_get_L(UI_OBJECT);
	lua_pushstring(L, "elapsed");
	lua_pushnumber(L, (g_wow->frametime - g_wow->lastframetime) / 1000000000.);
	ui_frame_execute_script(UI_FRAME, "OnUpdate", 1);
	if (UI_FRAME->backdrop)
		ui_backdrop_render(UI_FRAME->backdrop);
	int32_t left = ui_region_get_left(UI_REGION) * (g_wow->render_width / (float)UI_OBJECT->interface->width);
	int32_t top = ui_region_get_top(UI_REGION) * (g_wow->render_height / (float)UI_OBJECT->interface->height);
	int32_t width = ui_region_get_width(UI_REGION) * (g_wow->render_width / (float)UI_OBJECT->interface->width);
	int32_t height = ui_region_get_height(UI_REGION) * (g_wow->render_height / (float)UI_OBJECT->interface->height);
	ui_frame_render_layers(UI_FRAME);
	bool hidden = true;
	if (scroll_frame->scroll_child)
	{
		hidden = scroll_frame->scroll_child->region.hidden;
		ui_object_set_hidden((struct ui_object*)scroll_frame->scroll_child, true);
	}
	ui_frame_render_childs(UI_FRAME);
	if (scroll_frame->scroll_child)
	{
		ui_object_set_hidden((struct ui_object*)scroll_frame->scroll_child, hidden);
		struct ui_anchor *left_anchor = (struct ui_anchor*)ui_region_get_left_anchor((struct ui_region*)scroll_frame->scroll_child);
		if (left_anchor)
		{
			left_anchor->offset.abs.x = scroll_frame->horizontal_scroll;
			left_anchor->offset.abs.y = 0;
			ui_object_set_dirty_coords((struct ui_object*)scroll_frame->scroll_child);
		}
		struct ui_anchor *top_anchor = (struct ui_anchor*)ui_region_get_top_anchor((struct ui_region*)scroll_frame->scroll_child);
		if (top_anchor)
		{
			top_anchor->offset.abs.x = 0;
			top_anchor->offset.abs.y = scroll_frame->vertical_scroll;
			ui_object_set_dirty_coords((struct ui_object*)scroll_frame->scroll_child);
		}
		interface_enable_scissor(left, top, width, height);
		ui_object_render((struct ui_object*)scroll_frame->scroll_child);
		interface_disable_scissor();
	}
	ui_region_vtable.render(object);
}

static void on_mouse_scroll(struct ui_object *object, gfx_scroll_event_t *event)
{
	struct ui_scroll_frame *scroll_frame = (struct ui_scroll_frame*)object;
	if (event->used)
		return;
	/* if (UI_REGION->hovered) XXX: ? */
	{
		ui_scroll_frame_set_vertical_scroll(scroll_frame, scroll_frame->vertical_scroll - event->y * 50);
		ui_scroll_frame_set_horizontal_scroll(scroll_frame, scroll_frame->horizontal_scroll - event->x * 50);
		event->used = true;
	}
	ui_frame_vtable.on_mouse_scroll(object, event);
}

void ui_scroll_frame_set_vertical_scroll(struct ui_scroll_frame *scroll_frame, float scroll)
{
	if (scroll < 0)
		scroll = 0;
	if (scroll_frame->vertical_scroll == scroll)
		return;
	scroll_frame->vertical_scroll = scroll;
	struct lua_State *L = ui_object_get_L(UI_OBJECT);
	lua_pushstring(L, "offset");
	lua_pushnumber(L, scroll);
	ui_frame_execute_script(UI_FRAME, "OnVerticalScroll", 1);
}

void ui_scroll_frame_set_horizontal_scroll(struct ui_scroll_frame *scroll_frame, float scroll)
{
	if (scroll < 0)
		scroll = 0;
	if (scroll_frame->horizontal_scroll == scroll)
		return;
	scroll_frame->horizontal_scroll = scroll;
	struct lua_State *L = ui_object_get_L(UI_OBJECT);
	lua_pushstring(L, "offset");
	lua_pushnumber(L, scroll);
	ui_frame_execute_script(UI_FRAME, "OnHorizontalScroll", 1);
}

static int lua_GetVerticalScrollRange(lua_State *L)
{
	LUA_METHOD_SCROLL_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: ScrollFrame:GetVerticalScrollRange()");
	LUA_UNIMPLEMENTED_METHOD();
	lua_pushnumber(L, 200);
	return 1;
}

static int lua_GetVerticalScroll(lua_State *L)
{
	LUA_METHOD_SCROLL_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: ScrollFrame:GetVerticalScroll()");
	lua_pushnumber(L, scroll_frame->vertical_scroll);
	return 1;
}

static int lua_SetVerticalScroll(lua_State *L)
{
	LUA_METHOD_SCROLL_FRAME();
	if (argc != 2)
		return luaL_error(L, "Usage: ScrollFrame:SetVerticalScroll(scroll)");
	if (!lua_isnumber(L, 2))
		return luaL_argerror(L, 2, "number expected");
	ui_scroll_frame_set_vertical_scroll(scroll_frame, lua_tonumber(L, 2));
	return 0;
}

static int lua_GetHorizontalScrollRange(lua_State *L)
{
	LUA_METHOD_SCROLL_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: ScrollFrame:GetHorizontalScrollRange()");
	LUA_UNIMPLEMENTED_METHOD();
	lua_pushnumber(L, 200);
	return 1;
}

static int lua_GetHorizontalScroll(lua_State *L)
{
	LUA_METHOD_SCROLL_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: ScrollFrame:GetHorizontalScroll()");
	lua_pushnumber(L, scroll_frame->horizontal_scroll);
	return 1;
}

static int lua_SetHorizontalScroll(lua_State *L)
{
	LUA_METHOD_SCROLL_FRAME();
	if (argc != 2)
		return luaL_error(L, "Usage: ScrollFrame:SetHorizontalScroll(scroll)");
	if (!lua_isnumber(L, 2))
		return luaL_argerror(L, 2, "number expected");
	ui_scroll_frame_set_horizontal_scroll(scroll_frame, lua_tonumber(L, 2));
	return 0;
}

static int lua_GetScrollChild(lua_State *L)
{
	LUA_METHOD_SCROLL_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: ScrollFrame:GetScrollChild()");
	if (!scroll_frame->scroll_child)
	{
		lua_pushnil(L);
		return 1;
	}
	ui_push_lua_object(L, (struct ui_object*)scroll_frame->scroll_child);
	return 1;
}

static int lua_SetScrollChild(lua_State *L)
{
	LUA_METHOD_SCROLL_FRAME();
	if (argc != 2)
		return luaL_error(L, "Usage: ScrollFrame:SetScrollChild(child)");
	if (lua_type(L, 2) != LUA_TTABLE)
		return luaL_argerror(L, 2, "table expected");
	struct ui_frame *frame = ui_get_lua_frame(L, 2);
	if (!frame)
	{
		LOG_ERROR("Invalid parameter, expected Frame");
		return 0;
	}
	scroll_frame->scroll_child = frame;
	return 0;
}

static int lua_UpdateScrollChildRect(lua_State *L)
{
	LUA_METHOD_SCROLL_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: ScrollFrame:UpdateScrollChildRect()");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static bool register_methods(struct jks_array *methods)
{
	UI_REGISTER_METHOD(GetVerticalScrollRange);
	UI_REGISTER_METHOD(GetVerticalScroll);
	UI_REGISTER_METHOD(SetVerticalScroll);
	UI_REGISTER_METHOD(GetHorizontalScrollRange);
	UI_REGISTER_METHOD(GetHorizontalScroll);
	UI_REGISTER_METHOD(SetHorizontalScroll);
	UI_REGISTER_METHOD(GetScrollChild);
	UI_REGISTER_METHOD(SetScrollChild);
	UI_REGISTER_METHOD(UpdateScrollChildRect);
	return ui_frame_vtable.register_methods(methods);
}

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
UI_INH1(frame, bool, on_key_down, gfx_key_event_t*, event);
UI_INH1(frame, bool, on_key_up, gfx_key_event_t*, event);
UI_INH0(frame, struct ui_font_instance*, as_font_instance);
UI_INH0(frame, const char*, get_name);

const struct ui_object_vtable ui_scroll_frame_vtable =
{
	UI_OBJECT_VTABLE("ScrollFrame")
};
