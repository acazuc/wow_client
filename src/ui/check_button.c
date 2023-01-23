#include "ui/check_button.h"
#include "ui/texture.h"

#include "xml/check_button.h"

#include "wow_lua.h"
#include "log.h"

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_CHECKBUTTON() LUA_METHOD(CheckButton, check_button)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_FRAME->region)
#define UI_FRAME  (&UI_BUTTON->frame)
#define UI_BUTTON (&check_button->button)

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_button_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_check_button *check_button = (struct ui_check_button*)object;
	UI_OBJECT->mask |= UI_OBJECT_check_button;
	check_button->disabled_checked_texture = NULL;
	check_button->checked_texture = NULL;
	check_button->checked = false;
	return true;
}

static void dtr(struct ui_object *object)
{
	ui_button_vtable.dtr(object);
}

static void load_xml(struct ui_object *object, const struct xml_layout_frame *layout_frame)
{
	ui_button_vtable.load_xml(object, layout_frame);
	struct ui_check_button *check_button = (struct ui_check_button*)object;
	const struct xml_check_button *xml_check_button = xml_clayout_frame_as_check_button(layout_frame);
	if (xml_check_button)
	{
		if (OPTIONAL_ISSET(xml_check_button->disabled_checked_texture))
		{
			if (!check_button->disabled_checked_texture)
			{
				check_button->disabled_checked_texture = ui_texture_new(UI_OBJECT->interface, ((struct xml_layout_frame*)&OPTIONAL_GET(xml_check_button->disabled_checked_texture))->name, UI_REGION);
				if (check_button->disabled_checked_texture)
				{
					check_button->disabled_checked_texture->layered_region.draw_layer = DRAWLAYER_ARTWORK;
					if (!jks_array_push_back(&UI_FRAME->regions_to_load, &check_button->disabled_checked_texture))
						LOG_ERROR("failed to push region to load");
					if (!jks_array_push_back(&UI_FRAME->layers[DRAWLAYER_ARTWORK], &check_button->disabled_checked_texture))
						LOG_ERROR("failed to push region to layer");
					ui_object_set_hidden((struct ui_object*)check_button->disabled_checked_texture, true);
				}
			}
			ui_object_load_xml((struct ui_object*)check_button->disabled_checked_texture, (struct xml_layout_frame*)&OPTIONAL_GET(xml_check_button->disabled_checked_texture));
		}
		if (OPTIONAL_ISSET(xml_check_button->checked_texture))
		{
			if (!check_button->checked_texture)
			{
				check_button->checked_texture = ui_texture_new(UI_OBJECT->interface, ((struct xml_layout_frame*)&OPTIONAL_GET(xml_check_button->checked_texture))->name, UI_REGION);
				if (check_button->checked_texture)
				{
					check_button->checked_texture->layered_region.draw_layer = DRAWLAYER_ARTWORK;
					if (!jks_array_push_back(&UI_FRAME->regions_to_load, &check_button->checked_texture))
						LOG_ERROR("failed to push region to load");
					if (!jks_array_push_back(&UI_FRAME->layers[DRAWLAYER_ARTWORK], &check_button->checked_texture))
						LOG_ERROR("failed to push region to layer");
					ui_object_set_hidden((struct ui_object*)check_button->checked_texture, true);
				}
			}
			ui_object_load_xml((struct ui_object*)check_button->checked_texture, (struct xml_layout_frame*)&OPTIONAL_GET(xml_check_button->checked_texture));
		}
		if (OPTIONAL_ISSET(xml_check_button->checked))
			check_button->checked = OPTIONAL_GET(xml_check_button->checked);
	}
}

static void render(struct ui_object *object)
{
	struct ui_check_button *check_button = (struct ui_check_button*)object;
	if (check_button->checked)
	{
		if (UI_BUTTON->disabled)
		{
			if (check_button->disabled_checked_texture)
				ui_object_set_hidden((struct ui_object*)check_button->disabled_checked_texture, false);
		}
		else if (check_button->checked_texture)
			ui_object_set_hidden((struct ui_object*)check_button->checked_texture, false);
	}
	ui_button_vtable.render(object);
	if (check_button->disabled_checked_texture)
		ui_object_set_hidden((struct ui_object*)check_button->disabled_checked_texture, true);
	if (check_button->checked_texture)
		ui_object_set_hidden((struct ui_object*)check_button->checked_texture, true);
}

static void on_click(struct ui_object *object, enum gfx_mouse_button button)
{
	struct ui_check_button *check_button = (struct ui_check_button*)object;
	if (!UI_BUTTON->disabled)
		ui_check_button_set_checked(check_button, !check_button->checked);
	ui_button_vtable.on_click(object, button);
}

void ui_check_button_set_checked(struct ui_check_button *check_button, bool checked)
{
	if (check_button->checked == checked)
		return;
	check_button->checked = checked;
}

static int lua_GetChecked(lua_State *L)
{
	LUA_METHOD_CHECKBUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: CheckButton:GetChecked()");
	lua_pushboolean(L, check_button->checked);
	return 1;
}

static int lua_SetChecked(lua_State *L)
{
	LUA_METHOD_CHECKBUTTON();
	if (argc == 1)
	{
		ui_check_button_set_checked(check_button, true);
	}
	else if (argc >= 2)
	{
		ui_check_button_set_checked(check_button, lua_tointeger(L, 2));
	}
	else
	{
		return luaL_error(L, "Usage: CheckButton:SetChecked([checked])");
	}
	return 0;
}

static int lua_GetCheckedTexture(lua_State *L)
{
	LUA_METHOD_CHECKBUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: CheckButton:GetCheckedTexture()");
	if (!check_button->checked_texture)
	{
		lua_pushnil(L);
		return 1;
	}
	ui_push_lua_object(L, (struct ui_object*)check_button->checked_texture);
	return 1;
}

static int lua_SetCheckedTexture(lua_State *L)
{
	LUA_METHOD_CHECKBUTTON();
	if (argc != 2)
		return luaL_error(L, "Usage: CheckButton:SetCheckedTexture(texture or \"file\")");
	if (lua_type(L, 2) == LUA_TSTRING)
	{
		const char *str = lua_tostring(L, 2);
		if (!str)
			return 0;
		if (!check_button->checked_texture)
		{
			check_button->checked_texture = ui_texture_new(UI_OBJECT->interface, "", UI_REGION);
			if (!check_button->checked_texture)
			{
				LOG_ERROR("failed to create texture");
				return 0;
			}
			ui_object_set_hidden((struct ui_object*)check_button->checked_texture, true);
			if (!jks_array_push_back(&UI_FRAME->layers[DRAWLAYER_BORDER], &check_button->checked_texture))
				LOG_ERROR("failed to push region to layer");
		}
		ui_texture_set_file(check_button->checked_texture, str);
	}
	else if (lua_type(L, 2) == LUA_TTABLE)
	{
		struct ui_texture *texture = ui_get_lua_texture(L, 2);
		if (!texture)
			return luaL_argerror(L, 2, "texture expected");
		check_button->checked_texture = texture;
	}
	else
	{
		return luaL_argerror(L, 2, "string or table expected");
	}
	return 0;
}

static int lua_GetDisabledCheckedTexture(lua_State *L)
{
	LUA_METHOD_CHECKBUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: CheckButton:GetDisabledCheckedTexture()");
	if (!check_button->disabled_checked_texture)
	{
		lua_pushnil(L);
		return 1;
	}
	ui_push_lua_object(L, (struct ui_object*)check_button->disabled_checked_texture);
	return 1;
}

static int lua_SetDisabledCheckedTexture(lua_State *L)
{
	LUA_METHOD_CHECKBUTTON();
	if (argc != 2)
		return luaL_error(L, "Usage: CheckButton:SetDisabledCheckedTexture(texture or \"file\")");
	if (lua_type(L, 2) == LUA_TSTRING)
	{
		const char *str = lua_tostring(L, 2);
		if (!str)
			return 0;
		if (!check_button->disabled_checked_texture)
		{
			check_button->disabled_checked_texture = ui_texture_new(UI_OBJECT->interface, "", UI_REGION);
			if (!check_button->disabled_checked_texture)
			{
				LOG_ERROR("failed to create texture");
				return 0;
			}
			ui_object_set_hidden((struct ui_object*)check_button->disabled_checked_texture, true);
			if (!jks_array_push_back(&UI_FRAME->layers[DRAWLAYER_BORDER], &check_button->disabled_checked_texture))
				LOG_ERROR("failed to push region to layer");
		}
		ui_texture_set_file(check_button->disabled_checked_texture, str);
		return 0;
	}
	else if (lua_type(L, 2) == LUA_TTABLE)
	{
		struct ui_texture *texture = ui_get_lua_texture(L, 2);
		if (!texture)
			return luaL_argerror(L, 2, "texture expected");
		check_button->disabled_checked_texture = texture;
		return 0;
	}
	else
	{
		return luaL_argerror(L, 2, "string or table expected");
	}
	return 0;
}

static bool register_methods(struct jks_array *methods)
{
	UI_REGISTER_METHOD(GetChecked);
	UI_REGISTER_METHOD(SetChecked);
	UI_REGISTER_METHOD(GetCheckedTexture);
	UI_REGISTER_METHOD(SetCheckedTexture);
	UI_REGISTER_METHOD(GetDisabledCheckedTexture);
	UI_REGISTER_METHOD(SetDisabledCheckedTexture);
	return ui_button_vtable.register_methods(methods);
}

UI_INH0(button, void, post_load);
UI_INH0(button, void, register_in_interface);
UI_INH0(button, void, unregister_in_interface);
UI_INH0(button, void, eval_name);
UI_INH0(button, float, get_alpha);
UI_INH1(button, void, set_alpha, float, alpha);
UI_INH1(button, void, set_hidden, bool, hidden);
UI_INH2(button, void, get_size, int32_t*, x, int32_t*, y);
UI_INH0(button, void, set_dirty_coords);
UI_INH1(button, void, on_mouse_move, gfx_pointer_event_t*, event);
UI_INH1(button, void, on_mouse_down, gfx_mouse_event_t*, event);
UI_INH1(button, void, on_mouse_up, gfx_mouse_event_t*, event);
UI_INH1(button, void, on_mouse_scroll, gfx_scroll_event_t*, event);
UI_INH1(button, bool, on_key_down, gfx_key_event_t*, event);
UI_INH1(button, bool, on_key_up, gfx_key_event_t*, event);
UI_INH0(button, struct ui_font_instance*, as_font_instance);
UI_INH0(button, const char *, get_name);

const struct ui_object_vtable ui_check_button_vtable =
{
	UI_OBJECT_VTABLE("CheckButton")
};
