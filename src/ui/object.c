#include "ui/scrolling_message_frame.h"
#include "ui/taxi_route_frame.h"
#include "ui/layered_region.h"
#include "ui/dress_up_model.h"
#include "ui/message_frame.h"
#include "ui/color_select.h"
#include "ui/check_button.h"
#include "ui/scroll_frame.h"
#include "ui/game_tooltip.h"
#include "ui/player_model.h"
#include "ui/tabard_model.h"
#include "ui/movie_frame.h"
#include "ui/font_string.h"
#include "ui/simple_html.h"
#include "ui/world_frame.h"
#include "ui/status_bar.h"
#include "ui/model_ffx.h"
#include "ui/cooldown.h"
#include "ui/edit_box.h"
#include "ui/minimap.h"
#include "ui/texture.h"
#include "ui/region.h"
#include "ui/button.h"
#include "ui/slider.h"
#include "ui/object.h"
#include "ui/model.h"
#include "ui/color.h"
#include "ui/font.h"

#include "itf/interface.h"

#include "lua/lua_script.h"

#include "wow_lua.h"
#include "memory.h"
#include "log.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_OBJECT() LUA_METHOD(Object, object)

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	(void)name;
	(void)parent;
	object->interface = interface;
	object->lua_reference = LUA_NOREF;
	object->alpha = 1;
	object->mask = 0;
	return true;
}

static void dtr(struct ui_object *object)
{
	luaL_unref(ui_object_get_L(object), LUA_REGISTRYINDEX, object->lua_reference);
}

struct lua_State *ui_object_get_L(struct ui_object *object)
{
	return object->interface->L;
}

static void load_xml(struct ui_object *object, const struct xml_layout_frame *layout_frame)
{
	(void)object;
	(void)layout_frame;
}

static void post_load(struct ui_object *object)
{
	(void)object;
}

static void register_in_interface(struct ui_object *object)
{
	(void)object;
}

static void unregister_in_interface(struct ui_object *object)
{
	(void)object;
}

static void eval_name(struct ui_object *object)
{
	(void)object;
}

static void render(struct ui_object *object)
{
	(void)object;
}

static void on_click(struct ui_object *object, enum gfx_mouse_button button)
{
	(void)object;
	(void)button;
}

static void set_alpha(struct ui_object *object, float alpha)
{
	object->alpha = alpha;
}

static float get_alpha(struct ui_object *object)
{
	return object->alpha;
}

static void set_hidden(struct ui_object *object, bool hidden)
{
	(void)object;
	(void)hidden;
}

static void get_size(struct ui_object *object, int32_t *x, int32_t *y)
{
	(void)object;
	if (x)
		*x = 0;
	if (y)
		*y = 0;
}

static void set_dirty_coords(struct ui_object *object)
{
	(void)object;
}

static void on_mouse_move(struct ui_object *object, gfx_pointer_event_t *event)
{
	(void)object;
	(void)event;
}

static void on_mouse_down(struct ui_object *object, gfx_mouse_event_t *event)
{
	(void)object;
	(void)event;
}

static void on_mouse_up(struct ui_object *object, gfx_mouse_event_t *event)
{
	(void)object;
	(void)event;
}

static void on_mouse_scroll(struct ui_object *object, gfx_scroll_event_t *event)
{
	(void)object;
	(void)event;
}

static bool on_key_down(struct ui_object *object, gfx_key_event_t *event)
{
	(void)object;
	(void)event;
	return false;
}

static bool on_key_up(struct ui_object *object, gfx_key_event_t *event)
{
	(void)object;
	(void)event;
	return false;
}

static struct ui_font_instance *as_font_instance(struct ui_object *object)
{
	(void)object;
	return NULL;
}

static const char *get_name(struct ui_object *object)
{
	(void)object;
	return "";
}

static int local_lua_gc(lua_State *L)
{
	/* LOG_DEBUG("lua_gc"); */
	(void)L;
	return 0;
}

static int lua_GetAlpha(lua_State *L)
{
	LUA_METHOD_OBJECT();
	if (argc != 1)
		return luaL_error(L, "Usage: Object:GetAlpha()");
	lua_pushnumber(L, ui_object_get_alpha(object));
	return 1;
}

static int lua_SetAlpha(lua_State *L)
{
	LUA_METHOD_OBJECT();
	if (argc != 2)
		return luaL_error(L, "Usage: Object:SetAlpha(alpha)");
	if (!lua_isnumber(L, 2))
		return luaL_argerror(L, 2, "number expected");
	ui_object_set_alpha(object, lua_tonumber(L, 2));
	return 0;
}

static int lua_GetName(lua_State *L)
{
	LUA_METHOD_OBJECT();
	if (argc != 1)
		return luaL_error(L, "Usage: Object:GetName()");
	lua_pushstring(L, ui_object_get_name(object));
	return 1;
}

static int lua_GetObjectType(lua_State *L)
{
	LUA_METHOD_OBJECT();
	if (argc != 1)
		return luaL_error(L, "Usage: Object:GetObjectType()");
	lua_pushstring(L, object->vtable->name);
	return 1;
}

static int lua_IsObjectType(lua_State *L)
{
	LUA_METHOD_OBJECT();
	if (argc != 2)
		return luaL_error(L, "Usage: Object:IsObjectType()");
	const char *asked = lua_tostring(L, 2);
	if (!asked)
		return 0;
	bool is = false;

#define OBJECT_TYPE_TEST(name, type) \
	if (!strcmp(asked, name)) \
	{ \
		if (ui_object_as_##type(object)) \
			is = true; \
	}

	OBJECT_TYPE_TEST("Button", button);
	OBJECT_TYPE_TEST("CheckButton", check_button);
	OBJECT_TYPE_TEST("ColorSelect", color_select);
	OBJECT_TYPE_TEST("Cooldown", cooldown);
	OBJECT_TYPE_TEST("DressUpModel", dress_up_model);
	OBJECT_TYPE_TEST("EditBox", edit_box);
	OBJECT_TYPE_TEST("Font", font);
	OBJECT_TYPE_TEST("FontString", font_string);
	OBJECT_TYPE_TEST("Frame", frame);
	OBJECT_TYPE_TEST("GameTooltip", game_tooltip);
	OBJECT_TYPE_TEST("LayeredRegion", layered_region);
	OBJECT_TYPE_TEST("MessageFrame", message_frame);
	OBJECT_TYPE_TEST("Minimap", minimap);
	OBJECT_TYPE_TEST("Model", model);
	OBJECT_TYPE_TEST("ModelFFX", model_ffx);
	OBJECT_TYPE_TEST("MovieFrame", movie_frame);
	OBJECT_TYPE_TEST("PlayerModel", player_model);
	OBJECT_TYPE_TEST("Region", region);
	OBJECT_TYPE_TEST("ScrollFrame", scroll_frame);
	OBJECT_TYPE_TEST("ScrollingMessageFrame", scrolling_message_frame);
	OBJECT_TYPE_TEST("SimpleHTML", simple_html);
	OBJECT_TYPE_TEST("Slider", slider);
	OBJECT_TYPE_TEST("StatusBar", status_bar);
	OBJECT_TYPE_TEST("TabardModel", tabard_model);
	OBJECT_TYPE_TEST("TaxiRouteFrame", taxi_route_frame);
	OBJECT_TYPE_TEST("Texture", texture);
	OBJECT_TYPE_TEST("WorldFrame", world_frame);

#undef OBJECT_TYPE_TEST
	lua_pushboolean(L, is);
	return 1;
}

static bool register_methods(struct jks_array *methods)
{
	UI_REGISTER_METHOD(GetAlpha);
	UI_REGISTER_METHOD(SetAlpha);
	UI_REGISTER_METHOD(GetName);
	UI_REGISTER_METHOD(GetObjectType);
	UI_REGISTER_METHOD(IsObjectType);
	return true;
}

const struct ui_object_vtable ui_object_vtable =
{
	UI_OBJECT_VTABLE("Object")
};

const luaL_Reg local_ui_object_metatable[] =
{
	{"__gc", local_lua_gc},
	{NULL, NULL}
};

const luaL_Reg *ui_object_metatable = local_ui_object_metatable;

bool ui_register_method(struct jks_array *methods, const char *name, int (*func)(lua_State*))
{
	for (size_t i = 0; i < methods->size; ++i)
	{
		luaL_Reg *method = JKS_ARRAY_GET(methods, i, luaL_Reg);
		if (!strcmp(method->name, name))
		{
			LOG_WARN("method %s already defined", name);
			return true;
		}
	}
	luaL_Reg reg = {name, func};
	if (!jks_array_push_back(methods, &reg))
	{
		LOG_ERROR("failed to push method %s", name);
		return false;
	}
	return true;
}

void ui_object_delete(struct ui_object *object)
{
	if (!object)
		return;
	object->vtable->dtr(object);
	mem_free(MEM_UI, object);
}

void ui_object_load_xml(struct ui_object *object, const struct xml_layout_frame *layout_frame)
{
	object->vtable->load_xml(object, layout_frame);
}

void ui_object_post_load(struct ui_object *object)
{
	object->vtable->post_load(object);
}

void ui_object_unregister_in_interface(struct ui_object *object)
{
	object->vtable->unregister_in_interface(object);
}

void ui_object_register_in_interface(struct ui_object *object)
{
	object->vtable->register_in_interface(object);
}

void ui_object_eval_name(struct ui_object *object)
{
	object->vtable->eval_name(object);
}

void ui_object_render(struct ui_object *object)
{
	object->vtable->render(object);
}

void ui_object_on_click(struct ui_object *object, enum gfx_mouse_button button)
{
	object->vtable->on_click(object, button);
}

float ui_object_get_alpha(struct ui_object *object)
{
	return object->vtable->get_alpha(object);
}

void ui_object_set_alpha(struct ui_object *object, float alpha)
{
	object->vtable->set_alpha(object, alpha);
}

void ui_object_set_hidden(struct ui_object *object, bool hidden)
{
	object->vtable->set_hidden(object, hidden);
}

void ui_object_get_size(struct ui_object *object, int32_t *x, int32_t *y)
{
	return object->vtable->get_size(object, x, y);
}

void ui_object_set_dirty_coords(struct ui_object *object)
{
	object->vtable->set_dirty_coords(object);
}

void ui_object_on_mouse_move(struct ui_object *object, gfx_pointer_event_t *event)
{
	object->vtable->on_mouse_move(object, event);
}

void ui_object_on_mouse_down(struct ui_object *object, gfx_mouse_event_t *event)
{
	object->vtable->on_mouse_down(object, event);
}

void ui_object_on_mouse_up(struct ui_object *object, gfx_mouse_event_t *event)
{
	object->vtable->on_mouse_up(object, event);
}

void ui_object_on_mouse_scroll(struct ui_object *object, gfx_scroll_event_t *event)
{
	object->vtable->on_mouse_scroll(object, event);
}

bool ui_object_on_key_down(struct ui_object *object, gfx_key_event_t *event)
{
	return object->vtable->on_key_down(object, event);
}

bool ui_object_on_key_up(struct ui_object *object, gfx_key_event_t *event)
{
	return object->vtable->on_key_up(object, event);
}

const char *ui_object_get_name(struct ui_object *object)
{
	return object->vtable->get_name(object);
}

void ui_push_lua_object(lua_State *L, struct ui_object *object)
{
	if (object->lua_reference == LUA_NOREF)
	{
		lua_newtable(L);
		lua_pushstring(L, "native_object_ptr");
		struct ui_object **ptr = (struct ui_object**)lua_newuserdata(L, sizeof(*ptr));
		*ptr = object;
		/*luaL_getmetatable(L, this->vtable->name);
		lua_setmetatable(L, -2);*/
		lua_settable(L, -3);
		object->lua_reference = luaL_ref(L, LUA_REGISTRYINDEX);
	}
	lua_rawgeti(L, LUA_REGISTRYINDEX, object->lua_reference);
	luaL_getmetatable(L, object->vtable->name);
	lua_setmetatable(L, -2);
}

struct ui_object *ui_get_lua_object(lua_State *L, int index)
{
	if (lua_gettop(L) < index)
	{
		LOG_DEBUG("Invalid index");
		return NULL;
	}
	if (lua_type(L, index) != LUA_TTABLE)
	{
		LOG_DEBUG("arg isn't table");
		return NULL;
	}
	lua_pushstring(L, "native_object_ptr");
	lua_gettable(L, index);
	if (lua_type(L, -1) != LUA_TUSERDATA)
	{
		LOG_DEBUG("native_object_ptr isn't TUSERDATA");
		lua_pop(L, 2);
		return NULL;
	}
	struct ui_object **object = (struct ui_object**)lua_touserdata(L, -1);
	if (!object)
	{
		LOG_DEBUG("lua_touserdata failed");
		lua_pop(L, 1);
		return NULL;
	}
	struct ui_object *ret = *object;
	lua_pop(L, 1);
	return ret;
}

struct ui_font_instance *ui_get_lua_font_instance(lua_State *L, int index)
{
	struct ui_object *object = ui_get_lua_object(L, index);
	if (!object)
		return NULL;
	return object->vtable->as_font_instance(object);
}

bool ui_get_lua_color(lua_State *L, int index, struct ui_color *color)
{
	int argc = lua_gettop(L);
	if (argc < index + 2 || argc > index + 3)
	{
		LOG_DEBUG("luacolor: invalid arg");
		return false;
	}
	float r = lua_tonumber(L, index + 0);
	float g = lua_tonumber(L, index + 1);
	float b = lua_tonumber(L, index + 2);
	float a;
	if (argc == index + 3)
	{
		if (!lua_isnumber(L, index + 3))
			return false;
		a = lua_tonumber(L, index + 3);
	}
	else
	{
		a = 1;
	}
	color->r = r;
	color->g = g;
	color->b = b;
	color->a = a;
	return true;
}

void ui_push_lua_color(lua_State *L, const struct ui_color *color)
{
	if (color)
	{
		lua_pushnumber(L, color->r);
		lua_pushnumber(L, color->g);
		lua_pushnumber(L, color->b);
		lua_pushnumber(L, color->a);
	}
	else
	{
		lua_pushnumber(L, 1);
		lua_pushnumber(L, 1);
		lua_pushnumber(L, 1);
		lua_pushnumber(L, 1);
	}
}

void ui_push_lua_color_optional(lua_State *L, const struct optional_ui_color *color)
{
	ui_push_lua_color(L, color ? (OPTIONAL_ISSET(*color) ? &OPTIONAL_GET(*color) : NULL) : NULL);
}

void ui_object_print_args(lua_State *L, const char *name, const char *func)
{
	int argc = lua_gettop(L);
	char args[4096] = "";
	if (argc >= 1)
	{
		struct ui_region *region = ui_get_lua_region(L, 1);
		if (region)
			snprintf(args, sizeof(args), "%s", region->name);
		else
			snprintf(args, sizeof(args), "this");
		for (int i = 2; i <= argc; ++i)
		{
			char arg[128];
			switch (lua_type(L, i))
			{
				case LUA_TNUMBER:
					snprintf(arg, sizeof(arg), ", %lf", lua_tonumber(L, i));
					break;
				case LUA_TSTRING:
					snprintf(arg, sizeof(arg), ", \"%s\"", lua_tostring(L, i));
					break;
				case LUA_TNIL:
					snprintf(arg, sizeof(arg), ", nil");
					break;
				case LUA_TBOOLEAN:
					snprintf(arg, sizeof(arg), ", %s", lua_toboolean(L, i) ? "true" : "false");
					break;
				case LUA_TFUNCTION:
				{
					lua_Debug ar;
					lua_pushvalue(L, i);
					lua_getinfo(L, ">n", &ar);
					if (ar.name)
						snprintf(arg, sizeof(arg), ", fn:%s", ar.name);
					else
						snprintf(arg, sizeof(arg), ", function");
					break;
				}
				default:
					snprintf(arg, sizeof(arg), ", LUA_%s", lua_typename(L, lua_type(L, i)));
					break;
			}
			strcat(args, arg);
		}
	}
	LOG_DEBUG("%s:%s(%s)", name, func + 4, args);
}

#define OBJECT_CAST_DEFINE(name) \
const struct ui_##name *ui_cobject_as_##name(const struct ui_object *object) \
{ \
	if (!(object->mask & UI_OBJECT_##name)) \
		return NULL; \
	return (const struct ui_##name*)object; \
} \
struct ui_##name *ui_object_as_##name(struct ui_object *object) \
{ \
	if (!(object->mask & UI_OBJECT_##name)) \
		return NULL; \
	return (struct ui_##name*)object; \
} \
struct ui_##name *ui_get_lua_##name(lua_State *L, int index) \
{ \
	struct ui_object *object = ui_get_lua_object(L, index); \
	if (!object) \
		return NULL; \
	return ui_object_as_##name(object); \
} \
struct ui_##name *ui_##name##_new(struct interface *interface, const char *n, struct ui_region *parent) \
{ \
	struct ui_##name *name = mem_malloc(MEM_UI, sizeof(*name)); \
	if (!name) \
	{ \
		LOG_ERROR("allocation failed"); \
		return NULL; \
	} \
	struct ui_object *obj = (struct ui_object*)name; \
	obj->vtable = &ui_##name##_vtable; \
	if (!obj->vtable->ctr(obj, interface, n, parent)) \
	{ \
		LOG_ERROR("failed to ctr " #name); \
		mem_free(MEM_UI, name); \
		return NULL; \
	} \
	return name; \
}

OBJECT_CAST_DEFINE(button);
OBJECT_CAST_DEFINE(check_button);
OBJECT_CAST_DEFINE(color_select);
OBJECT_CAST_DEFINE(cooldown);
OBJECT_CAST_DEFINE(dress_up_model);
OBJECT_CAST_DEFINE(edit_box);
OBJECT_CAST_DEFINE(font);
OBJECT_CAST_DEFINE(font_string);
OBJECT_CAST_DEFINE(frame);
OBJECT_CAST_DEFINE(game_tooltip);
OBJECT_CAST_DEFINE(layered_region);
OBJECT_CAST_DEFINE(message_frame);
OBJECT_CAST_DEFINE(minimap);
OBJECT_CAST_DEFINE(model);
OBJECT_CAST_DEFINE(model_ffx);
OBJECT_CAST_DEFINE(movie_frame);
OBJECT_CAST_DEFINE(player_model);
OBJECT_CAST_DEFINE(region);
OBJECT_CAST_DEFINE(scroll_frame);
OBJECT_CAST_DEFINE(scrolling_message_frame);
OBJECT_CAST_DEFINE(simple_html);
OBJECT_CAST_DEFINE(slider);
OBJECT_CAST_DEFINE(status_bar);
OBJECT_CAST_DEFINE(tabard_model);
OBJECT_CAST_DEFINE(taxi_route_frame);
OBJECT_CAST_DEFINE(texture);
OBJECT_CAST_DEFINE(world_frame);

#undef OBJECT_CAST_DEFINE
