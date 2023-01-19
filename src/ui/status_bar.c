#include "status_bar.h"

#include "xml/status_bar.h"

#include "ui/texture.h"

#include "wow_lua.h"
#include "log.h"

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_STATUS_BAR() LUA_METHOD(StatusBar, status_bar)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_FRAME->region)
#define UI_FRAME (&status_bar->frame)

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_frame_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_status_bar *status_bar = (struct ui_status_bar*)object;
	UI_OBJECT->mask |= UI_OBJECT_status_bar;
	ui_color_init(&status_bar->color, 1, 1, 1, 1);
	status_bar->texture = NULL;
	status_bar->min_value = 0;
	status_bar->max_value = 0;
	status_bar->default_value = 0;
	status_bar->value = 0;
	return true;
}

static void dtr(struct ui_object *object)
{
	ui_frame_vtable.dtr(object);
}

static void load_xml(struct ui_object *object, const struct xml_layout_frame *layout_frame)
{
	ui_frame_vtable.load_xml(object, layout_frame);
	struct ui_status_bar *status_bar = (struct ui_status_bar*)object;
	const struct xml_status_bar *xml_status_bar = xml_clayout_frame_as_status_bar(layout_frame);
	if (status_bar)
	{
		if (OPTIONAL_ISSET(xml_status_bar->min_value))
			status_bar->min_value = OPTIONAL_GET(xml_status_bar->min_value);
		if (OPTIONAL_ISSET(xml_status_bar->max_value))
			status_bar->max_value = OPTIONAL_GET(xml_status_bar->max_value);
		if (OPTIONAL_ISSET(xml_status_bar->default_value))
			status_bar->default_value = OPTIONAL_GET(xml_status_bar->default_value);
		if (OPTIONAL_ISSET(xml_status_bar->bar_texture))
		{
			if (!status_bar->texture)
			{
				status_bar->texture = ui_texture_new(UI_OBJECT->interface, ((struct xml_layout_frame*)&OPTIONAL_GET(xml_status_bar->bar_texture))->name, UI_REGION);
				if (status_bar->texture)
				{
					if (!jks_array_push_back(&UI_FRAME->regions_to_load, &status_bar->texture))
						LOG_ERROR("failed to add region to load");
				}
			}
			ui_object_load_xml((struct ui_object*)status_bar->texture, (struct xml_layout_frame*)&OPTIONAL_GET(xml_status_bar->bar_texture));
		}
		if (OPTIONAL_ISSET(xml_status_bar->bar_color))
			ui_color_init_xml(&status_bar->color, &OPTIONAL_GET(xml_status_bar->bar_color));
	}
}

static void render(struct ui_object *object)
{
	ui_frame_vtable.render(object);
}

void ui_status_bar_set_color(struct ui_status_bar *status_bar, const struct ui_color *color)
{
	status_bar->color = *color;
	/* TODO event ? */
}

void ui_status_bar_set_min_value(struct ui_status_bar *status_bar, float value)
{
	if (status_bar->min_value == value)
		return;
	status_bar->value = value;
	/* TODO event ? */
}

void ui_status_bar_set_max_value(struct ui_status_bar *status_bar, float value)
{
	if (status_bar->max_value == value)
		return;
	status_bar->max_value = value;
	/* TODO event ? */
}

void ui_status_bar_set_value(struct ui_status_bar *status_bar, float value)
{
	if (status_bar->value == value)
		return;
	status_bar->value = value;
	/* TODO event ? */
}

static int lua_GetMinMaxValues(lua_State *L)
{
	LUA_METHOD_STATUS_BAR();
	if (argc != 1)
		return luaL_error(L, "Usage: StatusBar:GetMinMaxValues()");
	lua_pushnumber(L, status_bar->min_value);
	lua_pushnumber(L, status_bar->max_value);
	return 2;
}

static int lua_SetMinMaxValues(lua_State *L)
{
	LUA_METHOD_STATUS_BAR();
	if (argc != 3)
		return luaL_error(L, "Usage: StatusBar:SetMinMaxValues(min, max)");
	if (!lua_isnumber(L, 2))
		return luaL_argerror(L, 2, "number expected");
	if (!lua_isnumber(L, 3))
		return luaL_argerror(L, 3, "number expected");
	ui_status_bar_set_min_value(status_bar, lua_tonumber(L, 2));
	ui_status_bar_set_max_value(status_bar, lua_tonumber(L, 3));
	return 0;
}

static int lua_GetValue(lua_State *L)
{
	LUA_METHOD_STATUS_BAR();
	if (argc != 1)
		return luaL_error(L, "Usage: StatusBar:GetValue()");
	lua_pushnumber(L, status_bar->value);
	return 1;
}

static int lua_SetValue(lua_State *L)
{
	LUA_METHOD_STATUS_BAR();
	if (argc != 2)
		return luaL_error(L, "Usage: StatusBar:SetValue(value)");
	if (!lua_isnumber(L, 2))
		return luaL_argerror(L, 2, "number expected");
	ui_status_bar_set_value(status_bar, lua_tonumber(L, 2));
	return 1;
}

static int lua_GetStatusBarColor(lua_State *L)
{
	LUA_METHOD_STATUS_BAR();
	if (argc != 1)
		return luaL_error(L, "Usage: StatusBar:GetStatusBarColor()");
	ui_push_lua_color(L, &status_bar->color);
	return 4;
}

static int lua_SetStatusBarColor(lua_State *L)
{
	LUA_METHOD_STATUS_BAR();
	if (argc < 4 || argc > 5)
		return luaL_error(L, "Usage: StatusBar:SetStatusBarColor(r, g, b [, a])");
	struct ui_color color;
	if (!ui_get_lua_color(L, 2, &color))
		return luaL_argerror(L, 2, "color expected");
	ui_status_bar_set_color(status_bar, &color);
	return 0;
}

static int lua_GetStatusBarTexture(lua_State *L)
{
	LUA_METHOD_STATUS_BAR();
	if (argc != 1)
		return luaL_error(L, "Usage: StatusBar:GetStatusBarTexture()");
	if (status_bar->texture)
		ui_push_lua_object(L, (struct ui_object*)status_bar->texture);
	else
		lua_pushnil(L);
	return 1;
}

static int lua_SetStatusBarTexture(lua_State *L)
{
	LUA_METHOD_STATUS_BAR();
	if (argc != 2)
		return luaL_error(L, "Usage: StatusBar:SetStatusBarTexture(\"file\" or texture)");
	if (lua_type(L, 2) == LUA_TSTRING)
	{
		const char *str = lua_tostring(L, 2);
		if (!str)
			return 0;
		if (!status_bar->texture)
		{
			status_bar->texture = ui_texture_new(UI_OBJECT->interface, "", UI_REGION);
			if (!status_bar->texture)
				return 0;
		}
		ui_texture_set_file(status_bar->texture, str);
		return 0;
	}
	else if (lua_type(L, 2) == LUA_TTABLE)
	{
		struct ui_texture *texture = ui_get_lua_texture(L, 2);
		if (!texture)
			return 0;
		ui_object_delete((struct ui_object*)status_bar->texture);
		status_bar->texture = texture;
		return 0;
	}
	else
	{
		return luaL_argerror(L, 2, "table or string expected");
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
	UI_REGISTER_METHOD(GetStatusBarColor);
	UI_REGISTER_METHOD(SetStatusBarColor);
	UI_REGISTER_METHOD(GetStatusBarTexture);
	UI_REGISTER_METHOD(SetStatusBarTexture);
	return ui_frame_vtable.register_methods(methods);
}

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

const struct ui_object_vtable ui_status_bar_vtable =
{
	UI_OBJECT_VTABLE("StatusBar")
};
