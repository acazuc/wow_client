#include "itf/interface.h"

#include "ui/font_string.h"
#include "ui/texture.h"
#include "ui/button.h"
#include "ui/font.h"

#include "lua/functions.h"

#include "xml/button.h"

#include "wow_lua.h"
#include "memory.h"
#include "log.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_BUTTON() LUA_METHOD(Button, button)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_FRAME->region)
#define UI_FRAME  (&button->frame)

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_frame_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_button *button = (struct ui_button*)object;
	UI_OBJECT->mask |= UI_OBJECT_button;
	button->render_text = NULL;
	button->normal_texture = NULL;
	button->pushed_texture = NULL;
	button->disabled_texture = NULL;
	button->highlight_texture = NULL;
	button->normal_font = NULL;
	button->disabled_font = NULL;
	button->highlight_font = NULL;
	button->text = NULL;
	button->disabled = false;
	button->highlight_locked = false;
	UI_REGION->mouse_enabled = true;
	OPTIONAL_UNSET(button->normal_color);
	OPTIONAL_UNSET(button->disabled_color);
	OPTIONAL_UNSET(button->highlight_color);
	return true;
}

static void dtr(struct ui_object *object)
{
	struct ui_button *button = (struct ui_button*)object;
	ui_object_delete((struct ui_object*)button->normal_font);
	ui_object_delete((struct ui_object*)button->disabled_font);
	ui_object_delete((struct ui_object*)button->highlight_font);
	mem_free(MEM_UI, button->text);
	ui_frame_vtable.dtr(object);
}

static void load_xml(struct ui_object *object, const struct xml_layout_frame *layout_frame)
{
	ui_frame_vtable.load_xml(object, layout_frame);
	struct ui_button *button = (struct ui_button*)object;
	const struct xml_button *xml_button = xml_clayout_frame_as_button(layout_frame);
	if (xml_button)
	{
		if (OPTIONAL_ISSET(xml_button->button_text))
		{
			if (!button->render_text)
			{
				button->render_text = ui_font_string_new(UI_OBJECT->interface, ((struct xml_layout_frame*)&OPTIONAL_GET(xml_button->button_text))->name, UI_REGION);
				if (button->render_text)
				{
					button->render_text->layered_region.draw_layer = DRAWLAYER_OVERLAY;
					if (!jks_array_push_back(&UI_FRAME->regions_to_load, &button->render_text))
						LOG_ERROR("failed to add region to load");
					if (!jks_array_push_back(&UI_FRAME->layers[DRAWLAYER_OVERLAY], &button->render_text))
						LOG_ERROR("failed to add region to layer");
				}
			}
			ui_object_load_xml((struct ui_object*)button->render_text, (struct xml_layout_frame*)&OPTIONAL_GET(xml_button->button_text));
			if (button->text)
				ui_font_string_set_text(button->render_text, button->text);
		}
		if (OPTIONAL_ISSET(xml_button->normal_texture))
		{
			if (!button->normal_texture)
			{
				button->normal_texture = ui_texture_new(UI_OBJECT->interface, ((struct xml_layout_frame*)&OPTIONAL_GET(xml_button->normal_texture))->name, UI_REGION);
				if (button->normal_texture)
				{
					button->normal_texture->layered_region.draw_layer = DRAWLAYER_BORDER;
					if (!jks_array_push_back(&UI_FRAME->regions_to_load, &button->normal_texture))
						LOG_ERROR("failed to add region to load");
					if (!jks_array_push_back(&UI_FRAME->layers[DRAWLAYER_BORDER], &button->normal_texture))
						LOG_ERROR("failed to add region to layer");
					ui_object_set_hidden((struct ui_object*)button->normal_texture, true);
				}
			}
			ui_object_load_xml((struct ui_object*)button->normal_texture, (struct xml_layout_frame*)&OPTIONAL_GET(xml_button->normal_texture));
		}
		if (OPTIONAL_ISSET(xml_button->pushed_texture))
		{
			if (!button->pushed_texture)
			{
				button->pushed_texture = ui_texture_new(UI_OBJECT->interface, ((struct xml_layout_frame*)&OPTIONAL_GET(xml_button->pushed_texture))->name, UI_REGION);
				if (button->pushed_texture)
				{
					button->pushed_texture->layered_region.draw_layer = DRAWLAYER_BORDER;
					if (!jks_array_push_back(&UI_FRAME->regions_to_load, &button->pushed_texture))
						LOG_ERROR("failed to add region to load");
					if (!jks_array_push_back(&UI_FRAME->layers[DRAWLAYER_BORDER], &button->pushed_texture))
						LOG_ERROR("failed to add region to layer");
					ui_object_set_hidden((struct ui_object*)button->pushed_texture, true);
				}
			}
			ui_object_load_xml((struct ui_object*)button->pushed_texture, (struct xml_layout_frame*)&OPTIONAL_GET(xml_button->pushed_texture));
		}
		if (OPTIONAL_ISSET(xml_button->disabled_texture))
		{
			if (!button->disabled_texture)
			{
				button->disabled_texture = ui_texture_new(UI_OBJECT->interface, ((struct xml_layout_frame*)&OPTIONAL_GET(xml_button->disabled_texture))->name, UI_REGION);
				if (button->disabled_texture)
				{
					button->disabled_texture->layered_region.draw_layer = DRAWLAYER_BORDER;
					if (!jks_array_push_back(&UI_FRAME->regions_to_load, &button->disabled_texture))
						LOG_ERROR("failed to add region to load");
					if (!jks_array_push_back(&UI_FRAME->layers[DRAWLAYER_BORDER], &button->disabled_texture))
						LOG_ERROR("failed to add region to layer");
					ui_object_set_hidden((struct ui_object*)button->disabled_texture, true);
				}
			}
			ui_object_load_xml((struct ui_object*)button->disabled_texture, (struct xml_layout_frame*)&OPTIONAL_GET(xml_button->disabled_texture));
		}
		if (OPTIONAL_ISSET(xml_button->highlight_texture))
		{
			if (!button->highlight_texture)
			{
				button->highlight_texture = ui_texture_new(UI_OBJECT->interface, ((struct xml_layout_frame*)&OPTIONAL_GET(xml_button->highlight_texture))->name, UI_REGION);
				if (button->highlight_texture)
				{
					button->highlight_texture->layered_region.draw_layer = DRAWLAYER_HIGHLIGHT;
					if (!jks_array_push_back(&UI_FRAME->regions_to_load, &button->highlight_texture))
						LOG_ERROR("failed to add region to load");
					if (!jks_array_push_back(&UI_FRAME->layers[DRAWLAYER_HIGHLIGHT], &button->highlight_texture))
						LOG_ERROR("failed to add region to layer");
					ui_object_set_hidden((struct ui_object*)button->highlight_texture, true);
				}
			}
			ui_object_load_xml((struct ui_object*)button->highlight_texture, (struct xml_layout_frame*)&OPTIONAL_GET(xml_button->highlight_texture));
		}
		if (OPTIONAL_ISSET(xml_button->normal_color))
		{
			OPTIONAL_SET(button->normal_color);
			ui_color_init_xml(&OPTIONAL_GET(button->normal_color), &OPTIONAL_GET(xml_button->normal_color));
		}
		if (OPTIONAL_ISSET(xml_button->disabled_color))
		{
			OPTIONAL_SET(button->disabled_color);
			ui_color_init_xml(&OPTIONAL_GET(button->disabled_color), &OPTIONAL_GET(xml_button->disabled_color));
		}
		if (OPTIONAL_ISSET(xml_button->highlight_color))
		{
			OPTIONAL_SET(button->highlight_color);
			ui_color_init_xml(&OPTIONAL_GET(button->highlight_color), &OPTIONAL_GET(xml_button->highlight_color));
		}
		if (OPTIONAL_ISSET(xml_button->normal_font))
		{
			if (!button->normal_font)
				button->normal_font = ui_font_new(UI_OBJECT->interface, OPTIONAL_GET(xml_button->normal_font).name, NULL);
			ui_object_load_xml((struct ui_object*)button->normal_font, (const struct xml_layout_frame*)&OPTIONAL_GET(xml_button->normal_font));
		}
		if (OPTIONAL_ISSET(xml_button->disabled_font))
		{
			if (!button->disabled_font)
				button->disabled_font = ui_font_new(UI_OBJECT->interface, OPTIONAL_GET(xml_button->disabled_font).name, NULL);
			ui_object_load_xml((struct ui_object*)button->disabled_font, (const struct xml_layout_frame*)&OPTIONAL_GET(xml_button->disabled_font));
		}
		if (OPTIONAL_ISSET(xml_button->highlight_font))
		{
			if (!button->highlight_font)
				button->highlight_font = ui_font_new(UI_OBJECT->interface, OPTIONAL_GET(xml_button->highlight_font).name, NULL);
			ui_object_load_xml((struct ui_object*)button->highlight_font, (const struct xml_layout_frame*)&OPTIONAL_GET(xml_button->highlight_font));
		}
		if (xml_button->text)
		{
			struct lua_State *L = ui_object_get_L(UI_OBJECT);
			lua_getglobal(L, xml_button->text);
			const char *s = lua_tostring(L, -1);
			if (s)
				button->text = mem_strdup(MEM_UI, s);
			else
				button->text = mem_strdup(MEM_UI, xml_button->text);
			if (button->text)
			{
				if (button->render_text)
					ui_font_string_set_text(button->render_text, button->text);
			}
			else
			{
				LOG_ERROR("failed to duplicate text");
			}
			lua_pop(L, 1);
		}
	}
}

static void post_load(struct ui_object *object)
{
	struct ui_button *button = (struct ui_button*)object;
	if (!button->render_text)
	{
		button->render_text = ui_font_string_new(UI_OBJECT->interface, "", UI_REGION);
		if (button->render_text)
		{
			button->render_text->layered_region.draw_layer = DRAWLAYER_OVERLAY;
			if (!jks_array_push_back(&UI_FRAME->regions_to_load, &button->render_text))
				LOG_ERROR("failed to add region to load");
			if (button->text)
				ui_font_string_set_text(button->render_text, button->text);
			if (!jks_array_push_back(&UI_FRAME->layers[DRAWLAYER_OVERLAY], &button->render_text))
				LOG_ERROR("failed to add region to layer");
		}
	}
	ui_font_string_set_font_instance(button->render_text, button->normal_font); /* default font (ie. for text size OnLoad) */
	ui_frame_vtable.post_load(object);
}

static void render(struct ui_object *object)
{
	struct ui_button *button = (struct ui_button*)object;
	if (UI_REGION->hidden)
		return;
	struct ui_texture *texture = NULL;
	if (UI_REGION->clicked && button->pushed_texture)
		texture = button->pushed_texture;
	else if (button->normal_texture)
		texture = button->normal_texture;
	if ((UI_REGION->hovered || button->highlight_locked) && !button->disabled)
	{
		if (button->highlight_texture)
			ui_object_set_hidden((struct ui_object*)button->highlight_texture, false);
	}
	if (texture)
		ui_object_set_hidden((struct ui_object*)texture, false);
	if ((UI_REGION->hovered || button->highlight_locked) && button->disabled && button->highlight_font)
		ui_font_string_set_font_instance(button->render_text, button->highlight_font);
	else
		ui_font_string_set_font_instance(button->render_text, button->normal_font);
	if ((UI_REGION->hovered || button->highlight_locked) && !button->disabled && OPTIONAL_ISSET(button->highlight_color))
		ui_font_instance_set_color(&button->render_text->font_instance, &OPTIONAL_GET(button->highlight_color));
	else if (OPTIONAL_ISSET(button->normal_color))
		ui_font_instance_set_color(&button->render_text->font_instance, &OPTIONAL_GET(button->normal_color));
	ui_frame_vtable.render(object);
	if (texture)
		ui_object_set_hidden((struct ui_object*)texture, true);
	if (button->highlight_texture)
		ui_object_set_hidden((struct ui_object*)button->highlight_texture, true);
}

void ui_button_set_disabled(struct ui_button *button, bool disabled)
{
	if (button->disabled == disabled)
		return;
	button->disabled = disabled;
	/* TODO event ? */
}

void ui_button_set_text(struct ui_button *button, const char *text)
{
	mem_free(MEM_UI, button->text);
	button->text = mem_strdup(MEM_UI, text);
	if (button->text)
		ui_font_string_set_text(button->render_text, text);
	else
		LOG_ERROR("failed to duplicate text");
	/* TODO event ? */
}

static int lua_Enable(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:Enable()");
	ui_button_set_disabled(button, false);
	return 0;
}

static int lua_Disable(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:Disable()");
	ui_button_set_disabled(button, true);
	return 0;
}

static int lua_IsEnabled(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:IsEnabled()");
	lua_pushboolean(L, button->disabled);
	return 1;
}

static int lua_RegisterForClicks(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc < 2)
		return luaL_error(L, "Usage: Button:RegisterForClicks(...)");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_GetNormalTexture(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:GetNormalTexture()");
	if (button->normal_texture)
		ui_push_lua_object(L, (struct ui_object*)button->normal_texture);
	else
		lua_pushnil(L);
	return 1;
}

static int lua_SetNormalTexture(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 2)
		return luaL_error(L, "Usage: Button:SetNormalTexture(texture or \"file\")");
	if (lua_type(L, 2) == LUA_TSTRING)
	{
		const char *str = lua_tostring(L, 2);
		if (!str)
			return 0;
		if (!button->normal_texture)
		{
			button->normal_texture = ui_texture_new(UI_OBJECT->interface, "", UI_REGION);
			if (!button->normal_texture)
			{
				LOG_ERROR("failed to create texture");
				return 0;
			}
			ui_object_set_hidden((struct ui_object*)button->normal_texture, true);
			if (!jks_array_push_back(&UI_FRAME->layers[DRAWLAYER_BORDER], &button->normal_texture))
				LOG_ERROR("failed to add region to layer");
		}
		ui_texture_set_file(button->normal_texture, str);
		return 0;
	}
	else if (lua_type(L, 2) == LUA_TTABLE)
	{
		struct ui_texture *texture = ui_get_lua_texture(L, 2);
		if (!texture)
			return luaL_argerror(L, 2, "texture expected");
		ui_object_delete((struct ui_object*)button->normal_texture);
		button->normal_texture = texture;
		return 0;
	}
	LOG_ERROR("Invalid parameter type");
	return 0;
}

static int lua_GetHighlightTexture(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:GetHighlighTexture()");
	if (button->highlight_texture)
		ui_push_lua_object(L, (struct ui_object*)button->highlight_texture);
	else
		lua_pushnil(L);
	return 1;
}

static int lua_SetHighlightTexture(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 2)
		return luaL_error(L, "Usage: Button:SetHighlightTexture(texture or \"file\")");
	if (lua_type(L, 2) == LUA_TSTRING)
	{
		const char *str = lua_tostring(L, 2);
		if (!str)
			return 0;
		if (!button->highlight_texture)
		{
			button->highlight_texture = ui_texture_new(UI_OBJECT->interface, "", UI_REGION);
			if (!button->highlight_texture)
			{
				LOG_ERROR("failed to create texture");
				return 0;
			}
			ui_object_set_hidden((struct ui_object*)button->highlight_texture, true);
			if (!jks_array_push_back(&UI_FRAME->layers[DRAWLAYER_HIGHLIGHT], &button->highlight_texture))
				LOG_ERROR("failed to add region to layer");
		}
		ui_texture_set_file(button->highlight_texture, str);
		return 0;
	}
	else if (lua_type(L, 2) == LUA_TTABLE)
	{
		struct ui_texture *texture = ui_get_lua_texture(L, 2);
		if (!texture)
			return luaL_argerror(L, 2, "texture expected");
		ui_object_delete((struct ui_object*)button->highlight_texture);
		button->highlight_texture = texture;
		return 0;
	}
	else
	{
		return luaL_argerror(L, 2, "string or table expected");
	}
	return 0;
}

static int lua_GetPushedTexture(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:GetPushedTexture()");
	if (button->pushed_texture)
		ui_push_lua_object(L, (struct ui_object*)button->pushed_texture);
	else
		lua_pushnil(L);
	return 1;
}

static int lua_SetPushedTexture(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 2)
		return luaL_error(L, "Usage: Button:SetPushedTexture(texture or \"file\")");
	if (lua_type(L, 2) == LUA_TSTRING)
	{
		const char *str = lua_tostring(L, 2);
		if (!str)
			return 0;
		if (!button->pushed_texture)
		{
			button->pushed_texture = ui_texture_new(UI_OBJECT->interface, "", UI_REGION);
			if (!button->pushed_texture)
			{
				LOG_ERROR("failed to create texture");
				return false;
			}
			ui_object_set_hidden((struct ui_object*)button->pushed_texture, true);
			if (!jks_array_push_back(&UI_FRAME->layers[DRAWLAYER_BORDER], &button->pushed_texture))
				LOG_ERROR("failed to add region to layer");
		}
		ui_texture_set_file(button->pushed_texture, str);
		return 0;
	}
	else if (lua_type(L, 2) == LUA_TTABLE)
	{
		struct ui_texture *texture = ui_get_lua_texture(L, 2);
		if (!texture)
			return luaL_argerror(L, 2, "texture expected");
		ui_object_delete((struct ui_object*)button->pushed_texture);
		button->pushed_texture = texture;
		return 0;
	}
	else
	{
		return luaL_argerror(L, 2, "string or table expected");
	}
	return 0;
}

static int lua_GetDisabledTexture(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:GetDisabledTexture()");
	if (button->disabled_texture)
		ui_push_lua_object(L, (struct ui_object*)button->disabled_texture);
	else
		lua_pushnil(L);
	return 1;
}

static int lua_SetDisabledTexture(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 2)
		return luaL_error(L, "Usage: Button:SetDisabledTexture(texture or \"file\")");
	if (lua_type(L, 2) == LUA_TSTRING)
	{
		const char *str = lua_tostring(L, 2);
		if (!str)
			return 0;
		if (!button->disabled_texture)
		{
			button->disabled_texture = ui_texture_new(UI_OBJECT->interface, "", UI_REGION);
			if (!button->disabled_texture)
			{
				LOG_ERROR("failed to create texture");
				return 0;
			}
			ui_object_set_hidden((struct ui_object*)button->disabled_texture, true);
			if (!jks_array_push_back(&UI_FRAME->layers[DRAWLAYER_BORDER], &button->disabled_texture))
				LOG_ERROR("failed to add region to layer");
		}
		ui_texture_set_file(button->disabled_texture, str);
		return 0;
	}
	else if (lua_type(L, 2) == LUA_TTABLE)
	{
		struct ui_texture *texture = ui_get_lua_texture(L, 2);
		if (!texture)
			return luaL_argerror(L, 2, "texture expected");
		ui_object_delete((struct ui_object*)button->disabled_texture);
		button->disabled_texture = texture;
		return 0;
	}
	else
	{
		return luaL_argerror(L, 2, "string or table expected");
	}
	return 0;
}

static int lua_GetTextColor(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:GetTextColor()");
	ui_push_lua_color_optional(L, &button->normal_color);
	return 4;
}

static int lua_SetTextColor(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc < 4 || argc > 5)
		return luaL_error(L, "Usage: Button:SetTextColor(r, g, b [, a])");
	struct ui_color color;
	if (!ui_get_lua_color(L, 2, &color))
		return luaL_argerror(L, 2, "color expected");
	OPTIONAL_CTR(button->normal_color, color);
	return 0;
}

static int lua_GetHighlightTextColor(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:GetHighlightTextColor()");
	ui_push_lua_color_optional(L, &button->highlight_color);
	return 4;
}

static int lua_SetHighlightTextColor(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc < 4 || argc > 5)
		return luaL_error(L, "Usage: Button:SetHighlightTextColor(r, g, b [, a])");
	struct ui_color color;
	if (!ui_get_lua_color(L, 2, &color))
		return luaL_argerror(L, 2, "color expected");
	OPTIONAL_CTR(button->highlight_color, color);
	return 0;
}

static int lua_GetDisabledTextColor(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:GetDisabledTextColor()");
	ui_push_lua_color_optional(L, &button->disabled_color);
	return 4;
}

static int lua_SetDisabledTextColor(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc < 4 || argc > 5)
		return luaL_error(L, "Usage: Button:SetDisabledTextColor(r, g, b  [, a]");
	struct ui_color color;
	if (!ui_get_lua_color(L, 2, &color))
		return luaL_argerror(L, 2, "color expected");
	OPTIONAL_CTR(button->disabled_color, color);
	return 0;
}

static int lua_GetText(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:GetText()");
	lua_pushstring(L, button->text);
	return 1;
}

static int lua_SetText(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 2)
		return luaL_error(L, "Usage: Button:SetText(\"text\")");
	const char *str = lua_tostring(L, 2);
	ui_button_set_text(button, str ? str : "");
	return 0;
}

static int lua_GetTextWidth(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:GetTextWidth()");
	lua_pushnumber(L, ui_font_string_get_text_width(button->render_text));
	return 1;
}

static int lua_GetTextHeight(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:GetTextHeight()");
	lua_pushnumber(L, ui_font_string_get_text_height(button->render_text));
	return 1;
}

static int lua_GetButtonState(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:GetButtonState()");
	if (!button->disabled && UI_REGION->clicked)
		lua_pushstring(L, "PUSHED");
	else
		lua_pushstring(L, "NORMAL");
	return 1;
}

static int lua_SetButtonState(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc < 2 || argc > 3)
		return luaL_error(L, "Usage: Button:SetButtonState(\"state\" [, locked])");
	if (!lua_isstring(L, 2))
		return luaL_argerror(L, 2, "string expected");
	const char *state = lua_tostring(L, 2);
	if (!state)
		return 0;
	if (argc == 3)
	{
		LOG_WARN("unimplemented lock argument");
	}
	if (!strcmp(state, "NORMAL"))
		UI_REGION->clicked = false;
	else if (!strcmp(state, "PUSHED"))
		UI_REGION->clicked = true;
	else
		LOG_ERROR("unknown button state: %s", state);
	return 0;
}

static int lua_LockHighlight(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:LockHighlight()");
	button->highlight_locked = true;
	return 0;
}

static int lua_UnlockHighlight(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:UnlockHighlight()");
	button->highlight_locked = false;
	return 0;
}

static int lua_GetFontString(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:GetFontString()");
	ui_push_lua_object(L, (struct ui_object*)button->render_text);
	return 1;
}

static int lua_SetFontString(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 2)
		return luaL_error(L, "Usage: Button:SetFontString(fontString)");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_GetFont(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:GetFont()");
	struct ui_font *font;
	if (button->disabled)
	{
		if (button->disabled_font)
			font = button->disabled_font;
		else
			font = button->normal_font;
	}
	else
	{
		if (UI_REGION->hovered && button->highlight_font)
			font = button->highlight_font;
		else
			font = button->normal_font;
	}
	if (font)
	{
		lua_pushstring(L, font->font_instance.font);
		lua_pushnumber(L, 0); /* size */
		lua_pushstring(L, ""); /* flags (OUTLINE, THICKOUTLINE, MONOCHROME) */
	}
	else
	{
		lua_pushstring(L, "");
		lua_pushnumber(L, 0);
		lua_pushstring(L, "");
	}
	return 3;
}

static int lua_SetFont(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc < 3 || argc > 4)
		return luaL_error(L, "Usage: Button:SetFont(\"path\", height, \"flags\")");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_Click(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc > 2)
		return luaL_error(L, "Usage: Button:Click([button])");
	lua_pushstring(L, "button");
	lua_pushstring(L, "LeftButton");
	lua_pushstring(L, "down");
	lua_pushboolean(L, false);
	ui_frame_execute_script(UI_FRAME, "PreClick", 2);
	ui_object_on_click(UI_OBJECT, GFX_MOUSE_BUTTON_LEFT);
	lua_pushstring(L, "button");
	lua_pushstring(L, "LeftButton");
	lua_pushstring(L, "down");
	lua_pushboolean(L, false);
	ui_frame_execute_script(UI_FRAME, "PostClick", 2);
	return 0;
}

static int lua_GetTextFontObject(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:GetTextFontObject()");
	if (button->normal_font)
		ui_push_lua_object(L, (struct ui_object*)button->normal_font);
	else
		lua_pushnil(L);
	return 1;
}

static int lua_SetTextFontObject(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 2)
		return luaL_error(L, "Usage: Button:SetTextFontObject(font)");
	switch (lua_type(L, 2))
	{
		case LUA_TSTRING:
		{
			const char *font_name = lua_tostring(L, 2);
			if (!font_name)
				return 0;
			struct ui_font *font = interface_get_font(UI_OBJECT->interface, font_name);
			if (!font)
			{
				LOG_WARN("unknown font: %s", font_name);
				return 0;
			}
			button->normal_font = font;
			break;
		}
		case LUA_TTABLE:
		{
			struct ui_font *font = ui_get_lua_font(L, 2);
			if (!font)
				return luaL_argerror(L, 2, "Font object expected");
			button->normal_font = font;
			break;
		}
		default:
			return luaL_argerror(L, 2, "string or table expected");
	}
	return 0;
}

static int lua_GetHighlightFontObject(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:GetHighlightFontObject()");
	if (button->highlight_font)
		ui_push_lua_object(L, (struct ui_object*)button->highlight_font);
	else
		lua_pushnil(L);
	return 1;
}

static int lua_SetHighlightFontObject(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 2)
		return luaL_error(L, "Usage: Button:SetHighlightFontObject(font)");
	switch (lua_type(L, 2))
	{
		case LUA_TSTRING:
		{
			const char *font_name = lua_tostring(L, 2);
			if (!font_name)
				return 0;
			struct ui_font *font = interface_get_font(UI_OBJECT->interface, font_name);
			if (!font)
			{
				LOG_WARN("unknown font: %s", font_name);
				return 0;
			}
			button->highlight_font = font;
			break;
		}
		case LUA_TTABLE:
		{
			struct ui_font *font = ui_get_lua_font(L, 2);
			if (!font)
				return luaL_argerror(L, 2, "Font object expected");
			button->highlight_font = font;
			break;
		}
		default:
			return luaL_argerror(L, 2, "string or table expected");
	}
	return 0;
}

static int lua_GetDisabledFontObject(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 1)
		return luaL_error(L, "Usage: Button:GetDisabledFontObject()");
	if (button->disabled_font)
		ui_push_lua_object(L, (struct ui_object*)button->disabled_font);
	else
		lua_pushnil(L);
	return 1;
}

static int lua_SetDisabledFontObject(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc != 2)
		return luaL_error(L, "Usage: Button:SetDisabledFontObject(font)");
	switch (lua_type(L, 2))
	{
		case LUA_TSTRING:
		{
			const char *font_name = lua_tostring(L, 2);
			if (!font_name)
				return 0;
			struct ui_font *font = interface_get_font(UI_OBJECT->interface, font_name);
			if (!font)
			{
				LOG_WARN("unknown font: %s", font_name);
				return 0;
			}
			button->disabled_font = font;
			break;
		}
		case LUA_TTABLE:
		{
			struct ui_font *font = ui_get_lua_font(L, 2);
			if (!font)
				return luaL_argerror(L, 2, "Font object expected");
			button->disabled_font = font;
			break;
		}
		default:
			return luaL_argerror(L, 2, "string or table expected");
	}
	return 0;
}

static int lua_SetFormattedText(lua_State *L)
{
	LUA_METHOD_BUTTON();
	if (argc < 1)
		return luaL_error(L, "Usage: Button:SetFormattedText(\"format\", ...)");
	char buffer[4096];
	lua_format(L, 1, buffer, sizeof(buffer));
	ui_button_set_text(button, buffer);
	return 0;
}

static bool register_methods(struct jks_array *methods)
{
	UI_REGISTER_METHOD(Enable);
	UI_REGISTER_METHOD(Disable);
	UI_REGISTER_METHOD(IsEnabled);
	UI_REGISTER_METHOD(RegisterForClicks);
	UI_REGISTER_METHOD(GetNormalTexture);
	UI_REGISTER_METHOD(SetNormalTexture);
	UI_REGISTER_METHOD(GetHighlightTexture);
	UI_REGISTER_METHOD(SetHighlightTexture);
	UI_REGISTER_METHOD(GetPushedTexture);
	UI_REGISTER_METHOD(SetPushedTexture);
	UI_REGISTER_METHOD(GetDisabledTexture);
	UI_REGISTER_METHOD(SetDisabledTexture);
	UI_REGISTER_METHOD(GetTextColor);
	UI_REGISTER_METHOD(SetTextColor);
	UI_REGISTER_METHOD(GetHighlightTextColor);
	UI_REGISTER_METHOD(SetHighlightTextColor);
	UI_REGISTER_METHOD(GetDisabledTextColor);
	UI_REGISTER_METHOD(SetDisabledTextColor);
	UI_REGISTER_METHOD(GetText);
	UI_REGISTER_METHOD(SetText);
	UI_REGISTER_METHOD(SetFormattedText);
	UI_REGISTER_METHOD(GetTextWidth);
	UI_REGISTER_METHOD(GetTextHeight);
	UI_REGISTER_METHOD(GetButtonState);
	UI_REGISTER_METHOD(SetButtonState);
	UI_REGISTER_METHOD(LockHighlight);
	UI_REGISTER_METHOD(UnlockHighlight);
	UI_REGISTER_METHOD(GetFontString);
	UI_REGISTER_METHOD(SetFontString);
	UI_REGISTER_METHOD(GetFont);
	UI_REGISTER_METHOD(SetFont);
	UI_REGISTER_METHOD(Click);
	UI_REGISTER_METHOD(GetTextFontObject);
	UI_REGISTER_METHOD(SetTextFontObject);
	UI_REGISTER_METHOD(GetHighlightFontObject);
	UI_REGISTER_METHOD(SetHighlightFontObject);
	UI_REGISTER_METHOD(GetDisabledFontObject);
	UI_REGISTER_METHOD(SetDisabledFontObject);
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
UI_INH1(frame, void, on_mouse_scroll, gfx_scroll_event_t*, event);
UI_INH1(frame, bool, on_key_down, gfx_key_event_t*, event);
UI_INH1(frame, bool, on_key_up, gfx_key_event_t*, event);
UI_INH0(frame, struct ui_font_instance*, as_font_instance);
UI_INH0(frame, const char *, get_name);

const struct ui_object_vtable ui_button_vtable =
{
	UI_OBJECT_VTABLE("Button")
};
