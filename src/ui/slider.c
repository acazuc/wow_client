#include "ui/texture.h"
#include "ui/slider.h"

#include "xml/slider.h"

#include "wow_lua.h"
#include "log.h"

#include <math.h>

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_SLIDER() LUA_METHOD(Slider, slider)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_FRAME->region)
#define UI_FRAME (&slider->frame)

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_frame_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_slider *slider = (struct ui_slider*)object;
	UI_OBJECT->mask |= UI_OBJECT_slider;
	UI_REGION->mouse_enabled = true;
	slider->thumb_texture = NULL;
	slider->draw_layer = DRAWLAYER_OVERLAY;
	slider->min_value = 0;
	slider->max_value = 0;
	slider->value_step = 0;
	slider->default_value = 0;
	slider->value = slider->default_value;
	slider->orientation = ORIENTATION_VERTICAL;
	return true;
}

static void dtr(struct ui_object *object)
{
	struct ui_slider *slider = (struct ui_slider*)object;
	ui_object_delete((struct ui_object*)slider->thumb_texture);
	ui_frame_vtable.dtr(object);
}

static void load_xml(struct ui_object *object, const struct xml_layout_frame *layout_frame)
{
	ui_frame_vtable.load_xml(object, layout_frame);
	struct ui_slider *slider = (struct ui_slider*)object;
	const struct xml_slider *xml_slider = xml_clayout_frame_as_slider(layout_frame);
	if (xml_slider)
	{
		if (OPTIONAL_ISSET(xml_slider->thumb_texture))
		{
			if (!slider->thumb_texture)
			{
				slider->thumb_texture = ui_texture_new(UI_OBJECT->interface, ((struct xml_layout_frame*)&OPTIONAL_GET(xml_slider->thumb_texture))->name, UI_REGION);
				if (slider->thumb_texture)
				{
					if (!jks_array_push_back(&UI_FRAME->regions_to_load, &slider->thumb_texture))
						LOG_ERROR("failed to push region to load");
				}
			}
			ui_object_load_xml((struct ui_object*)slider->thumb_texture, (struct xml_layout_frame*)&OPTIONAL_GET(xml_slider->thumb_texture));
		}
		if (xml_slider->draw_layer)
		{
			if (!draw_layer_from_string(xml_slider->draw_layer, &slider->draw_layer))
				LOG_ERROR("invalid draw layer: %s", xml_slider->draw_layer);
		}
		if (OPTIONAL_ISSET(xml_slider->min_value))
			slider->min_value = OPTIONAL_GET(xml_slider->min_value);
		if (OPTIONAL_ISSET(xml_slider->max_value))
			slider->max_value = OPTIONAL_GET(xml_slider->max_value);
		if (OPTIONAL_ISSET(xml_slider->value_step))
			slider->value_step = OPTIONAL_GET(xml_slider->value_step);
		if (OPTIONAL_ISSET(xml_slider->default_value))
		{
			slider->default_value = OPTIONAL_GET(xml_slider->default_value);
			slider->value = slider->default_value;
		}
		if (xml_slider->orientation)
		{
			if (!orientation_from_string(xml_slider->orientation, &slider->orientation))
				LOG_ERROR("invalid orientation value: %s", xml_slider->orientation);
		}
	}
}

static void post_load(struct ui_object *object)
{
	struct ui_slider *slider = (struct ui_slider*)object;
	ui_frame_vtable.post_load(object);
	if (slider->thumb_texture)
	{
		ui_region_clear_anchors((struct ui_region*)slider->thumb_texture);
		if (slider->orientation == ORIENTATION_HORIZONTAL)
			ui_region_add_anchor((struct ui_region*)slider->thumb_texture, ui_anchor_new(UI_OBJECT->interface, NULL, ANCHOR_LEFT, ANCHOR_LEFT, "$parent", (struct ui_region*)slider->thumb_texture));
		else
			ui_region_add_anchor((struct ui_region*)slider->thumb_texture, ui_anchor_new(UI_OBJECT->interface, NULL, ANCHOR_TOP, ANCHOR_TOP, "$parent", (struct ui_region*)slider->thumb_texture));
	}
}

static void render(struct ui_object *object)
{
	struct ui_slider *slider = (struct ui_slider*)object;
	ui_frame_vtable.render(object);
	if (slider->thumb_texture)
	{
		float pos = (slider->value - slider->min_value) / (slider->max_value - slider->min_value);
		if (slider->orientation == ORIENTATION_HORIZONTAL)
		{
			struct ui_anchor *anchor = (struct ui_anchor*)ui_region_get_left_anchor((struct ui_region*)slider->thumb_texture);
			if (anchor)
			{
				int32_t off = pos * (ui_region_get_width(UI_REGION) - ui_region_get_width((struct ui_region*)slider->thumb_texture));
				anchor->offset.abs.x = off;
				anchor->offset.abs.y = 0;
				ui_object_set_dirty_coords((struct ui_object*)slider->thumb_texture);
			}
		}
		else
		{
			struct ui_anchor *anchor = (struct ui_anchor*)ui_region_get_top_anchor((struct ui_region*)slider->thumb_texture);
			if (anchor)
			{
				int32_t off = pos * (ui_region_get_height(UI_REGION) - ui_region_get_height((struct ui_region*)slider->thumb_texture));
				anchor->offset.abs.x = 0;
				anchor->offset.abs.y = -off;
				ui_object_set_dirty_coords((struct ui_object*)slider->thumb_texture);
			}
		}
		ui_object_render((struct ui_object*)slider->thumb_texture);
	}
}

static void on_mouse_move(struct ui_object *object, struct gfx_pointer_event *event)
{
	struct ui_slider *slider = (struct ui_slider*)object;
	if (!UI_REGION->clicked)
	{
		ui_frame_vtable.on_mouse_move(object, event);
		return;
	}
	ui_frame_vtable.on_mouse_move(object, event);
	float pct;
	if (slider->orientation == ORIENTATION_HORIZONTAL)
	{
		int32_t min_x;
		int32_t max_x = ui_region_get_width(UI_REGION);
		if (slider->thumb_texture)
		{
			min_x = ui_region_get_left(UI_REGION) + ui_region_get_width((struct ui_region*)slider->thumb_texture) / 2;
			max_x -= ui_region_get_width((struct ui_region*)slider->thumb_texture);
		}
		else
		{
			min_x = ui_region_get_left(UI_REGION);
		}
		int32_t off = event->x - min_x;
		if (off < 0)
			off = 0;
		if (off > max_x)
			off = max_x;
		pct = (off / (float)max_x) * (slider->max_value - slider->min_value);
	}
	else
	{
		int32_t min_y;
		int32_t max_y = ui_region_get_height(UI_REGION);
		if (slider->thumb_texture)
		{
			min_y = ui_region_get_top(UI_REGION) + ui_region_get_height((struct ui_region*)slider->thumb_texture) / 2;
			max_y -= ui_region_get_height((struct ui_region*)slider->thumb_texture);
		}
		else
		{
			min_y = ui_region_get_top(UI_REGION);
		}
		int32_t off = event->y - min_y;
		if (off < 0)
			off = 0;
		if (off > max_y)
			off = max_y;
		pct = (off / (float)max_y) * (slider->max_value - slider->min_value);
	}
	pct += slider->value_step / 2;
	float value = slider->min_value + pct;
	if (slider->value_step > 0)
		value -= fmod(pct, slider->value_step);
	ui_slider_set_value(slider, value);
	event->used = true;
}

void ui_slider_set_min_value(struct ui_slider *slider, float value)
{
	if (slider->min_value == value)
		return;
	slider->min_value = value;
	/* push minmax ? */
}

void ui_slider_set_max_value(struct ui_slider *slider, float value)
{
	if (slider->max_value == value)
		return;
	slider->max_value = value;
	/* push minmax ? */
}

void ui_slider_set_value_step(struct ui_slider *slider, float step)
{
	if (slider->value_step == step)
		return;
	slider->value_step = step;
}

void ui_slider_set_default_value(struct ui_slider *slider, float value)
{
	if (slider->default_value == value)
		return;
	slider->default_value = value;
}

void ui_slider_set_value(struct ui_slider *slider, float value)
{
	if (value < slider->min_value)
		value = slider->min_value;
	if (value > slider->max_value)
		value = slider->max_value;
	if (slider->value == value)
		return;
	slider->value = value;
	struct lua_State *L = ui_object_get_L(UI_OBJECT);
	lua_pushstring(L, "value");
	lua_pushnumber(L, slider->value);
	ui_frame_execute_script(UI_FRAME, "OnValueChanged", 1);
}

static int lua_GetMinMaxValues(lua_State *L)
{
	LUA_METHOD_SLIDER();
	if (argc != 1)
		return luaL_error(L, "Usage: Slider:GetMinMaxValues()");
	lua_pushnumber(L, slider->min_value);
	lua_pushnumber(L, slider->max_value);
	return 2;
}

static int lua_SetMinMaxValues(lua_State *L)
{
	LUA_METHOD_SLIDER();
	if (argc != 3)
		return luaL_error(L, "Usage: Slider:SetMinMaxValues(min, max)");
	ui_slider_set_min_value(slider, lua_tonumber(L, 2));
	ui_slider_set_max_value(slider, lua_tonumber(L, 3));
	return 0;
}

static int lua_GetValue(lua_State *L)
{
	LUA_METHOD_SLIDER();
	if (argc != 1)
		return luaL_error(L, "Usage: Slider:GetValue()");
	lua_pushnumber(L, slider->value);
	return 1;
}

static int lua_SetValue(lua_State *L)
{
	LUA_METHOD_SLIDER();
	if (argc != 2)
		return luaL_error(L, "Usage: Slider:SetValue(value)");
	ui_slider_set_value(slider, lua_tonumber(L, 2));
	return 0;
}

static int lua_GetValueStep(lua_State *L)
{
	LUA_METHOD_SLIDER();
	if (argc != 1)
		return luaL_error(L, "Usage: Slider:GetValueStep()");
	lua_pushnumber(L, slider->value_step);
	return 1;
}

static int lua_SetValueStep(lua_State *L)
{
	LUA_METHOD_SLIDER();
	if (argc != 2)
		return luaL_error(L, "Usage: Slider:SetValueStep(step)");
	ui_slider_set_value_step(slider, lua_tonumber(L, 2));
	return 0;
}

static int lua_GetThumbTexture(lua_State *L)
{
	LUA_METHOD_SLIDER();
	if (argc != 1)
		return luaL_error(L, "Usage: Slider:GetThumbTexture()");
	if (slider->thumb_texture)
		ui_push_lua_object(L, (struct ui_object*)slider->thumb_texture);
	else
		lua_pushnil(L);
	return 1;
}

static int lua_SetThumbTexture(lua_State *L)
{
	LUA_METHOD_SLIDER();
	if (argc != 2)
		return luaL_error(L, "Usage: Slider:SetThumbTexture()");
	if (lua_type(L, 2) == LUA_TSTRING)
	{
		const char *str = lua_tostring(L, 2);
		if (!str)
			return 0;
		if (!slider->thumb_texture)
		{
			slider->thumb_texture = ui_texture_new(UI_OBJECT->interface, "", UI_REGION);
			if (!slider->thumb_texture)
				return 0;
		}
		ui_texture_set_file(slider->thumb_texture, str);
	}
	else if (lua_type(L, 2) == LUA_TTABLE)
	{
		struct ui_texture *texture = ui_get_lua_texture(L, 2);
		if (!texture)
			return 0;
		ui_object_delete((struct ui_object*)slider->thumb_texture);
		slider->thumb_texture = texture;
	}
	else
	{
		return luaL_argerror(L, 2, "string or table expected");
	}
	return 0;
}

static bool register_methods(struct jks_array *methods)
{
	/*
	   GetOrientation
	   SetOrientation
	 */
	UI_REGISTER_METHOD(GetMinMaxValues);
	UI_REGISTER_METHOD(SetMinMaxValues);
	UI_REGISTER_METHOD(GetValue);
	UI_REGISTER_METHOD(SetValue);
	UI_REGISTER_METHOD(GetValueStep);
	UI_REGISTER_METHOD(SetValueStep);
	UI_REGISTER_METHOD(GetThumbTexture);
	UI_REGISTER_METHOD(SetThumbTexture);
	return ui_frame_vtable.register_methods(methods);
}

UI_INH1(frame, void, on_click, enum gfx_mouse_button, button);
UI_INH0(frame, void, register_in_interface);
UI_INH0(frame, void, unregister_in_interface);
UI_INH0(frame, void, eval_name);
UI_INH0(frame, float, get_alpha);
UI_INH1(frame, void, set_alpha, float, alpha);
UI_INH1(frame, void, set_hidden, bool, hidden);
UI_INH2(frame, void, get_size, int32_t*, x, int32_t*, y);
UI_INH0(frame, void, set_dirty_coords);
UI_INH1(frame, void, on_mouse_down, struct gfx_mouse_event*, event);
UI_INH1(frame, void, on_mouse_up, struct gfx_mouse_event*, event);
UI_INH1(frame, void, on_mouse_scroll, struct gfx_scroll_event*, event);
UI_INH1(frame, bool, on_key_down, struct gfx_key_event*, event);
UI_INH1(frame, bool, on_key_up, struct gfx_key_event*, event);
UI_INH0(frame, struct ui_font_instance*, as_font_instance);
UI_INH0(frame, const char*, get_name);

const struct ui_object_vtable ui_slider_vtable =
{
	UI_OBJECT_VTABLE("Slider")
};
