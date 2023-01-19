#include "font_instance.h"

#include "font/font.h"

#include "ui/font.h"

#include "itf/interface.h"
#include "itf/font.h"

#include "wow_lua.h"
#include "memory.h"
#include "cache.h"
#include "log.h"
#include "wow.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_FONT_INSTANCE() LUA_METHOD(FontInstance, font_instance)

void ui_font_instance_init(struct interface *interface, struct ui_font_instance *font_instance, struct ui_object *object, const struct ui_font_instance_callbacks *callbacks)
{
	font_instance->interface = interface;
	font_instance->font_instance = NULL;
	font_instance->render_font = NULL;
	OPTIONAL_UNSET(font_instance->font_height);
	OPTIONAL_UNSET(font_instance->color);
	OPTIONAL_UNSET(font_instance->shadow_color);
	OPTIONAL_UNSET(font_instance->spacing);
	OPTIONAL_UNSET(font_instance->outline);
	OPTIONAL_UNSET(font_instance->monochrome);
	OPTIONAL_UNSET(font_instance->justify_h);
	OPTIONAL_UNSET(font_instance->justify_v);
	font_instance->font = NULL;
	font_instance->object = object;
	font_instance->callbacks = callbacks;
}

void ui_font_instance_destroy(struct ui_font_instance *font_instance)
{
	if (font_instance->font_instance)
		ui_font_remove_child(font_instance->font_instance, font_instance);
	if (font_instance->render_font)
		interface_unref_render_font(font_instance->interface, font_instance->render_font);
	mem_free(MEM_UI, font_instance->font);
}

struct interface_font *ui_font_instance_get_render_font(const struct ui_font_instance *font_instance)
{
	if (font_instance->render_font)
		return font_instance->render_font;
	else if (font_instance->font_instance)
		return ui_font_instance_get_render_font(&font_instance->font_instance->font_instance);
	return NULL;
}

static int lua_GetJustifyH(lua_State *L)
{
	LUA_METHOD_FONT_INSTANCE();
	if (argc != 1)
		return luaL_error(L, "Usage: FontInstance:GetJustifyH()");
	lua_pushstring(L, justify_h_to_string(ui_font_instance_get_justify_h(font_instance)));
	return 1;
}

static int lua_SetJustifyH(lua_State *L)
{
	LUA_METHOD_FONT_INSTANCE();
	if (argc != 2)
		return luaL_error(L, "Usage: FontInstance:SetJustifyH(justifyH)");
	if (!lua_isstring(L, 2))
		return luaL_argerror(L, 2, "string expected");
	enum justify_h_type type;
	if (!justify_h_from_string(lua_tostring(L, 2), &type))
		return luaL_argerror(L, 2, "justifyH type expected");
	ui_font_instance_set_justify_h(font_instance, type);
	return 0;
}

static int lua_GetJustifyV(lua_State *L)
{
	LUA_METHOD_FONT_INSTANCE();
	if (argc != 1)
		return luaL_error(L, "Usage: FontInstance:GetJustifyV()");
	lua_pushstring(L, justify_v_to_string(ui_font_instance_get_justify_v(font_instance)));
	return 1;
}

static int lua_SetJustifyV(lua_State *L)
{
	LUA_METHOD_FONT_INSTANCE();
	if (argc != 2)
		return luaL_error(L, "Usage: FontInstance:SetJustifyV(justifyV)");
	if (!lua_isstring(L, 2))
		return luaL_argerror(L, 2, "string expected");
	enum justify_v_type type;
	if (!justify_v_from_string(lua_tostring(L, 2), &type))
		return luaL_argerror(L, 2, "justifyV type expected");
	ui_font_instance_set_justify_v(font_instance, type);
	return 0;
}

static int lua_GetTextColor(lua_State *L)
{
	LUA_METHOD_FONT_INSTANCE();
	if (argc != 1)
		return luaL_error(L, "Usage: FontInstance:GetTextColor()");
	const struct ui_color *color = ui_font_instance_get_color(font_instance);
	ui_push_lua_color(L, color);
	return 4;
}

static int lua_SetTextColor(lua_State *L)
{
	LUA_METHOD_FONT_INSTANCE();
	if (argc < 4 || argc > 5)
		return luaL_error(L, "Usage: FontInstance:SetTextColor(r, g, b [, a])");
	struct ui_color color;
	if (!ui_get_lua_color(L, 2, &color))
		return luaL_argerror(L, 2, "color expected");
	ui_font_instance_set_color(font_instance, &color);
	return 0;
}

static int lua_GetSpacing(lua_State *L)
{
	LUA_METHOD_FONT_INSTANCE();
	if (argc != 1)
		return luaL_error(L, "Usage: FontInstance:GetSpacing()");
	lua_pushnumber(L, ui_font_instance_get_spacing(font_instance));
	return 1;
}

static int lua_SetSpacing(lua_State *L)
{
	LUA_METHOD_FONT_INSTANCE();
	if (argc != 2)
		return luaL_error(L, "Usage: FontInstance:SetSpacing(spacing)");
	if (!lua_isnumber(L, 2))
		return luaL_argerror(L, 2, "number expected");
	ui_font_instance_set_spacing(font_instance, lua_tonumber(L, 2));
	return 0;
}

static int lua_GetFont(lua_State *L)
{
	LUA_METHOD_FONT_INSTANCE();
	if (argc != 1)
		return luaL_error(L, "Usage: FontInstance:GetFont()");
	if (font_instance->render_font)
	{
		char flags[64] = "";
		lua_pushstring(L, font_instance->font);
		lua_pushnumber(L, font_instance->render_font->font->height);
		if (OPTIONAL_ISSET(font_instance->monochrome) && OPTIONAL_GET(font_instance->monochrome))
			strcat(flags, "MONOCHROME");
		if (OPTIONAL_ISSET(font_instance->outline))
		{
			switch (OPTIONAL_GET(font_instance->outline))
			{
				case OUTLINE_THICK:
					if (flags[0])
						strcat(flags, ", ");
					strcat(flags, "THICKOUTLINE");
					break;
				case OUTLINE_NORMAL:
					if (flags[0])
						strcat(flags, ", ");
					strcat(flags, "OUTLINE");
					break;
				default:
					break;
			}
		}
		lua_pushstring(L, flags);
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
	LUA_METHOD_FONT_INSTANCE();
	if (argc < 3 || argc > 4)
		return luaL_error(L, "Usage: FontInstance:SetFont(\"file\", size [, flags])");
	if (!lua_isstring(L, 2))
		return luaL_argerror(L, 2, "string expected");
	const char *font = lua_tostring(L, 2);
	if (!font)
		return 0;
	if (!lua_isnumber(L, 3))
		return luaL_argerror(L, 3, "number expected");
	int size = lua_tonumber(L, 3);
	if (argc == 4)
	{
		/* TODO handle flags */
	}
	if (font_instance->render_font)
	{
		interface_unref_render_font(font_instance->interface, font_instance->render_font);
		font_instance->render_font = NULL;
	}
	mem_free(MEM_UI, font_instance->font);
	font_instance->font = mem_strdup(MEM_UI, font);
	if (!font_instance->font)
	{
		LOG_ERROR("failed to duplicate font name");
		return 0;
	}
	font_instance->render_font = interface_ref_render_font(font_instance->interface, font_instance->font, size);
	if (!font_instance->render_font)
		LOG_ERROR("failed to create font");
	/* TODO handle flags */
	lua_pushinteger(L, 1);
	return 1;
}

static int lua_GetShadowColor(lua_State *L)
{
	LUA_METHOD_FONT_INSTANCE();
	if (argc != 1)
		return luaL_error(L, "Usage: FontInstance:GetShadowColor()");
	const struct ui_color *color = ui_font_instance_get_shadow_color(font_instance);
	ui_push_lua_color(L, color);
	return 4;
}

static int lua_SetShadowColor(lua_State *L)
{
	LUA_METHOD_FONT_INSTANCE();
	if (argc < 4 || argc > 5)
		return luaL_error(L, "Usage: FontInstance:SetShadowColor(r, g, b [, a])");
	struct ui_color color;
	if (!ui_get_lua_color(L, 2, &color))
		return luaL_argerror(L, 2, "color expected");
	ui_font_instance_set_shadow_color(font_instance, &color);
	return 0;
}

static int lua_SetFontObject(lua_State *L)
{
	LUA_METHOD_FONT_INSTANCE();
	if (argc < 1 || argc > 2)
		return luaL_error(L, "Usage: FontInstance:SetFontObject(fontObject)");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

bool ui_font_instance_register_methods(struct jks_array *methods)
{
	/*
	   GetFontObject
	   GetShadowOffset
	   SetShadowOffset
	 */
	UI_REGISTER_METHOD(GetJustifyH);
	UI_REGISTER_METHOD(SetJustifyH);
	UI_REGISTER_METHOD(GetJustifyV);
	UI_REGISTER_METHOD(SetJustifyV);
	UI_REGISTER_METHOD(GetTextColor);
	UI_REGISTER_METHOD(SetTextColor);
	UI_REGISTER_METHOD(GetSpacing);
	UI_REGISTER_METHOD(SetSpacing);
	UI_REGISTER_METHOD(GetFont);
	UI_REGISTER_METHOD(SetFont);
	UI_REGISTER_METHOD(GetShadowColor);
	UI_REGISTER_METHOD(SetShadowColor);
	UI_REGISTER_METHOD(SetFontObject);
	return true;
}

const struct ui_value *ui_font_instance_get_font_height(const struct ui_font_instance *font_instance)
{
	if (!OPTIONAL_ISSET(font_instance->font_height))
		return NULL;
	return &OPTIONAL_GET(font_instance->font_height);
}

const struct ui_color *ui_font_instance_get_color(const struct ui_font_instance *font_instance)
{
	if (OPTIONAL_ISSET(font_instance->color))
		return &OPTIONAL_GET(font_instance->color);
	if (font_instance->font_instance)
		return ui_font_instance_get_color(&font_instance->font_instance->font_instance);
	return NULL;
}

void ui_font_instance_set_color(struct ui_font_instance *font_instance, const struct ui_color *color)
{
	if (OPTIONAL_ISSET(font_instance->color) && ui_color_eq(&OPTIONAL_GET(font_instance->color), color))
		return;
	OPTIONAL_CTR(font_instance->color, *color);
	font_instance->callbacks->on_color_changed(font_instance->object);
}

const struct ui_color *ui_font_instance_get_shadow_color(const struct ui_font_instance *font_instance)
{
	if (OPTIONAL_ISSET(font_instance->shadow_color))
		return &OPTIONAL_GET(font_instance->shadow_color);
	if (font_instance->font_instance)
		return ui_font_instance_get_shadow_color(&font_instance->font_instance->font_instance);
	return NULL;
}

void ui_font_instance_set_shadow_color(struct ui_font_instance *font_instance, const struct ui_color *color)
{
	if (OPTIONAL_ISSET(font_instance->shadow_color) && ui_color_eq(&OPTIONAL_GET(font_instance->shadow_color), color))
		return;
	OPTIONAL_CTR(font_instance->shadow_color, *color);
	font_instance->callbacks->on_shadow_color_changed(font_instance->object);
}

float ui_font_instance_get_spacing(const struct ui_font_instance *font_instance)
{
	if (OPTIONAL_ISSET(font_instance->spacing))
		return OPTIONAL_GET(font_instance->spacing);
	if (font_instance->font_instance)
		return ui_font_instance_get_spacing(&font_instance->font_instance->font_instance);
	return 0;
}

void ui_font_instance_set_spacing(struct ui_font_instance *font_instance, float spacing)
{
	if (OPTIONAL_ISSET(font_instance->spacing) && OPTIONAL_GET(font_instance->spacing) == spacing)
		return;
	OPTIONAL_CTR(font_instance->spacing, spacing);
	font_instance->callbacks->on_spacing_changed(font_instance->object);
}

enum outline_type ui_font_instance_get_outline(const struct ui_font_instance *font_instance)
{
	if (OPTIONAL_ISSET(font_instance->outline))
		return OPTIONAL_GET(font_instance->outline);
	if (font_instance->font_instance)
		return ui_font_instance_get_outline(&font_instance->font_instance->font_instance);
	return OUTLINE_NONE;
}

void ui_font_instance_set_outline(struct ui_font_instance *font_instance, enum outline_type outline)
{
	if (OPTIONAL_ISSET(font_instance->outline) && OPTIONAL_GET(font_instance->outline) == outline)
		return;
	OPTIONAL_CTR(font_instance->outline, outline);
	font_instance->callbacks->on_outline_changed(font_instance->object);
}

bool ui_font_instance_get_monochrome(const struct ui_font_instance *font_instance)
{
	if (OPTIONAL_ISSET(font_instance->monochrome))
		return OPTIONAL_GET(font_instance->monochrome);
	if (font_instance->font_instance)
		return ui_font_instance_get_monochrome(&font_instance->font_instance->font_instance);
	return false;
}

void ui_font_instance_set_monochrome(struct ui_font_instance *font_instance, bool monochrome)
{
	if (OPTIONAL_ISSET(font_instance->monochrome) && OPTIONAL_GET(font_instance->monochrome) == monochrome)
		return;
	OPTIONAL_CTR(font_instance->monochrome, monochrome);
	font_instance->callbacks->on_monochrome_changed(font_instance->object);
}

enum justify_v_type ui_font_instance_get_justify_v(const struct ui_font_instance *font_instance)
{
	if (OPTIONAL_ISSET(font_instance->justify_v))
		return OPTIONAL_GET(font_instance->justify_v);
	if (font_instance->font_instance)
		return ui_font_instance_get_justify_v(&font_instance->font_instance->font_instance);
	return JUSTIFYV_MIDDLE;
}

void ui_font_instance_set_justify_v(struct ui_font_instance *font_instance, enum justify_v_type type)
{
	if (OPTIONAL_ISSET(font_instance->justify_v) && OPTIONAL_GET(font_instance->justify_v) == type)
		return;
	OPTIONAL_CTR(font_instance->justify_v, type);
	font_instance->callbacks->on_justify_v_changed(font_instance->object);
}

enum justify_h_type ui_font_instance_get_justify_h(const struct ui_font_instance *font_instance)
{
	if (OPTIONAL_ISSET(font_instance->justify_h))
		return OPTIONAL_GET(font_instance->justify_h);
	if (font_instance->font_instance)
		return ui_font_instance_get_justify_h(&font_instance->font_instance->font_instance);
	return JUSTIFYH_CENTER;
}

void ui_font_instance_set_justify_h(struct ui_font_instance *font_instance, enum justify_h_type type)
{
	if (OPTIONAL_ISSET(font_instance->justify_h) && OPTIONAL_GET(font_instance->justify_h) == type)
		return;
	OPTIONAL_CTR(font_instance->justify_h, type);
	font_instance->callbacks->on_justify_h_changed(font_instance->object);
}
