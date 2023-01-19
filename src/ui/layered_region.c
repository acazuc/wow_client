#include "layered_region.h"

#include "ui/frame.h"
#include "wow_lua.h"
#include "log.h"

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_LAYERED_REGION() LUA_METHOD(LayeredRegion, layered_region)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&layered_region->region)

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_region_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_layered_region *layered_region = (struct ui_layered_region*)object;
	UI_OBJECT->mask |= UI_OBJECT_layered_region;
	ui_color_init(&layered_region->vertex_color, 1, 1, 1, 1);
	return true;
}

static void dtr(struct ui_object *object)
{
	ui_region_vtable.dtr(object);
}

static int lua_SetVertexColor(lua_State *L)
{
	LUA_METHOD_LAYERED_REGION();
	if (argc < 4 || argc > 5)
		return luaL_error(L, "Usage: LayeredRegion:SetVertexColor(r, g, b [, a])");
	if (!ui_get_lua_color(L, 2, &layered_region->vertex_color))
		return luaL_argerror(L, 2, "color expected");
	return 0;
}

static int lua_GetDrawLayer(lua_State *L)
{
	LUA_METHOD_LAYERED_REGION();
	if (argc != 1)
		return luaL_error(L, "Usage: LayeredRegion::GetDrawLayer()");
	lua_pushstring(L, draw_layer_to_string(layered_region->draw_layer));
	return 1;
}

static int lua_SetDrawLayer(lua_State *L)
{
	LUA_METHOD_LAYERED_REGION();
	if (argc != 2)
		return luaL_error(L, "Usage: LayeredRegion::SetDrawLayer(layer)");
	const char *str = lua_tostring(L, 2);
	if (!str)
		return luaL_argerror(L, 2, "failed to get string");
	enum draw_layer draw_layer;
	if (!draw_layer_from_string(str, &draw_layer))
		return luaL_argerror(L, 2, "invalid layer string");
	if (UI_REGION->parent)
	{
		struct ui_frame *parent = ui_object_as_frame((struct ui_object*)UI_REGION->parent);
		if (parent)
		{
			for (size_t i = 0; i < parent->layers[layered_region->draw_layer].size; ++i)
			{
				if (*JKS_ARRAY_GET(&parent->layers[layered_region->draw_layer], i, struct ui_layered_region*) != layered_region)
					continue;
				jks_array_erase(&parent->layers[layered_region->draw_layer], i);
				break;
			}
		}
	}
	layered_region->draw_layer = draw_layer;
	return 0;
}

static bool register_methods(struct jks_array *methods)
{
	UI_REGISTER_METHOD(SetVertexColor);
	UI_REGISTER_METHOD(GetDrawLayer);
	UI_REGISTER_METHOD(SetDrawLayer);
	return ui_region_vtable.register_methods(methods);
}

UI_INH1(region, void, load_xml, const struct xml_layout_frame*, layout_frame);
UI_INH0(region, void, post_load);
UI_INH0(region, void, register_in_interface);
UI_INH0(region, void, unregister_in_interface);
UI_INH0(region, void, eval_name);
UI_INH0(region, void, render);
UI_INH1(region, void, on_click, enum gfx_mouse_button, button);
UI_INH0(region, float, get_alpha);
UI_INH1(region, void, set_alpha, float, alpha);
UI_INH1(region, void, set_hidden, bool, hidden);
UI_INH2(region, void, get_size, int32_t*, x, int32_t*, y);
UI_INH0(region, void, set_dirty_coords);
UI_INH1(region, void, on_mouse_move, gfx_pointer_event_t*, event);
UI_INH1(region, void, on_mouse_down, gfx_mouse_event_t*, event);
UI_INH1(region, void, on_mouse_up, gfx_mouse_event_t*, event);
UI_INH1(region, void, on_mouse_scroll, gfx_scroll_event_t*, event);
UI_INH1(region, bool, on_key_down, gfx_key_event_t*, event);
UI_INH1(region, bool, on_key_up, gfx_key_event_t*, event);
UI_INH0(region, struct ui_font_instance*, as_font_instance);
UI_INH0(region, const char*, get_name);

const struct ui_object_vtable ui_layered_region_vtable =
{
	UI_OBJECT_VTABLE("LayeredRegion")
};
