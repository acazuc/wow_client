#ifndef UI_OBJECT_H
#define UI_OBJECT_H

#include "ui/color.h"

#include <gfx/events.h>

#include <jks/array.h>

#include <stdbool.h>
#include <stddef.h>

#ifdef interface
# undef interface
#endif

#if 0
# include "log.h"
# define LUA_VERBOSE_METHOD(obj) ui_object_print_args(L, #obj, __func__)
#else
# define LUA_VERBOSE_METHOD(obj)
#endif

#if 0
#include "log.h"
#define LUA_UNIMPLEMENTED_METHOD() LOG_DEBUG("unimplemented method: %s", __func__)
#else
#define LUA_UNIMPLEMENTED_METHOD()
#endif

#define UI_INH0(inh, ret, name) \
static ret name(struct ui_object *object) \
{ \
	return ui_##inh##_vtable.name(object); \
}

#define UI_INH1(inh, ret, name, t1, n1) \
static ret name(struct ui_object *object, t1 n1) \
{ \
	return ui_##inh##_vtable.name(object, n1); \
}

#define UI_INH2(inh, ret, name, t1, n1, t2, n2) \
static ret name(struct ui_object *object, t1 n1, t2 n2) \
{ \
	return ui_##inh##_vtable.name(object, n1, n2); \
}

#define UI_INH3(inh, ret, name, t1, n1, t2, n2, t3, n3) \
static ret name(struct ui_object *object, t1 n1, t2 n2, t3 n3) \
{ \
	return ui_##inh##_vtable.name(object, n1, n2, n3); \
}

struct xml_layout_frame;
struct lua_script;
struct interface;

typedef struct lua_State lua_State;
typedef struct luaL_Reg luaL_Reg;

enum ui_object_mask
{
	UI_OBJECT_button                  = 0x0000001,
	UI_OBJECT_check_button            = 0x0000002,
	UI_OBJECT_color_select            = 0x0000004,
	UI_OBJECT_cooldown                = 0x0000008,
	UI_OBJECT_dress_up_model          = 0x0000010,
	UI_OBJECT_edit_box                = 0x0000020,
	UI_OBJECT_font                    = 0x0000040,
	UI_OBJECT_font_string             = 0x0000080,
	UI_OBJECT_frame                   = 0x0000100,
	UI_OBJECT_game_tooltip            = 0x0000200,
	UI_OBJECT_layered_region          = 0x0000400,
	UI_OBJECT_message_frame           = 0x0000800,
	UI_OBJECT_minimap                 = 0x0001000,
	UI_OBJECT_model                   = 0x0002000,
	UI_OBJECT_model_ffx               = 0x0004000,
	UI_OBJECT_movie_frame             = 0x0008000,
	UI_OBJECT_player_model            = 0x0010000,
	UI_OBJECT_region                  = 0x0020000,
	UI_OBJECT_scroll_frame            = 0x0040000,
	UI_OBJECT_scrolling_message_frame = 0x0080000,
	UI_OBJECT_simple_html             = 0x0100000,
	UI_OBJECT_slider                  = 0x0200000,
	UI_OBJECT_status_bar              = 0x0400000,
	UI_OBJECT_tabard_model            = 0x0800000,
	UI_OBJECT_taxi_route_frame        = 0x1000000,
	UI_OBJECT_texture                 = 0x2000000,
	UI_OBJECT_world_frame             = 0x4000000,
};

struct ui_font_instance;
struct ui_object;
struct ui_region;

struct ui_object_vtable
{
	const char *name;
	bool (*ctr)(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent);
	void (*dtr)(struct ui_object *object);
	void (*load_xml)(struct ui_object *object, const struct xml_layout_frame *layout_frame);
	void (*post_load)(struct ui_object *object);
	void (*register_in_interface)(struct ui_object *object);
	void (*unregister_in_interface)(struct ui_object *object);
	void (*eval_name)(struct ui_object *object);
	void (*render)(struct ui_object *object);
	void (*on_click)(struct ui_object *object, enum gfx_mouse_button button);
	float (*get_alpha)(struct ui_object *object);
	void (*set_alpha)(struct ui_object *object, float alpha);
	void (*set_hidden)(struct ui_object *object, bool hidden);
	void (*get_size)(struct ui_object *object, int32_t *x, int32_t *y);
	void (*set_dirty_coords)(struct ui_object *object);
	void (*on_mouse_move)(struct ui_object *object, gfx_pointer_event_t *event);
	void (*on_mouse_down)(struct ui_object *object, gfx_mouse_event_t *event);
	void (*on_mouse_up)(struct ui_object *object, gfx_mouse_event_t *event);
	void (*on_mouse_scroll)(struct ui_object *object, gfx_scroll_event_t *event);
	bool (*on_key_down)(struct ui_object *object, gfx_key_event_t *event);
	bool (*on_key_up)(struct ui_object *object, gfx_key_event_t *event);
	struct ui_font_instance *(*as_font_instance)(struct ui_object *object);
	const char *(*get_name)(struct ui_object *object);
	bool (*register_methods)(struct jks_array *methods);
};

/* this macro enforce definition of every field, assuring safer definitions */
#define UI_OBJECT_VTABLE(n) \
	.name = n, \
	.ctr = ctr, \
	.dtr = dtr, \
	.load_xml = load_xml, \
	.post_load = post_load, \
	.register_in_interface = register_in_interface, \
	.unregister_in_interface = unregister_in_interface, \
	.eval_name = eval_name, \
	.render = render, \
	.on_click = on_click, \
	.get_alpha = get_alpha, \
	.set_alpha = set_alpha, \
	.set_hidden = set_hidden, \
	.get_size = get_size, \
	.set_dirty_coords = set_dirty_coords, \
	.on_mouse_move = on_mouse_move, \
	.on_mouse_down = on_mouse_down, \
	.on_mouse_up = on_mouse_up, \
	.on_mouse_scroll = on_mouse_scroll, \
	.on_key_down = on_key_down, \
	.on_key_up = on_key_up, \
	.as_font_instance = as_font_instance, \
	.get_name = get_name, \
	.register_methods = register_methods

struct ui_object
{
	const struct ui_object_vtable *vtable;
	struct interface *interface;
	uint32_t mask;
	int lua_reference;
	float alpha;
};

extern const struct ui_object_vtable ui_object_vtable;
extern const luaL_Reg *ui_object_metatable;

void ui_object_delete(struct ui_object *object);
struct lua_State *ui_object_get_L(struct ui_object *object);
void ui_object_load_xml(struct ui_object *object, const struct xml_layout_frame *layout_frame);
void ui_object_post_load(struct ui_object *object);
void ui_object_register_in_interface(struct ui_object *object);
void ui_object_unregister_in_interface(struct ui_object *object);
void ui_object_eval_name(struct ui_object *object);
void ui_object_render(struct ui_object *object);
void ui_object_on_click(struct ui_object *object, enum gfx_mouse_button button);
float ui_object_get_alpha(struct ui_object *object);
void ui_object_set_alpha(struct ui_object *object, float alpha);
void ui_object_set_hidden(struct ui_object *object, bool hidden);
void ui_object_get_size(struct ui_object *object, int32_t *x, int32_t *y);
void ui_object_set_dirty_coords(struct ui_object *object);
void ui_object_on_mouse_move(struct ui_object *object, gfx_pointer_event_t *event);
void ui_object_on_mouse_down(struct ui_object *object, gfx_mouse_event_t *event);
void ui_object_on_mouse_up(struct ui_object *object, gfx_mouse_event_t *event);
void ui_object_on_mouse_scroll(struct ui_object *object, gfx_scroll_event_t *event);
bool ui_object_on_key_down(struct ui_object *object, gfx_key_event_t *event);
bool ui_object_on_key_up(struct ui_object *object, gfx_key_event_t *event);
const char *ui_object_get_name(struct ui_object *object);
void ui_push_lua_object(lua_State *L, struct ui_object *object);
struct ui_object *ui_get_lua_object(lua_State *L, int index);
struct ui_font_instance *ui_get_lua_font_instance(lua_State *L, int index);
bool ui_get_lua_color(lua_State *L, int index, struct ui_color *color);
void ui_push_lua_color(lua_State *L, const struct ui_color *color);
void ui_push_lua_color_optional(lua_State *L, const struct optional_ui_color *color);
void ui_object_print_args(lua_State *L, const char *name, const char *func);

#define OBJECT_DECLARE_CAST(name) \
	struct ui_##name; \
	const struct ui_##name *ui_cobject_as_##name(const struct ui_object *object); \
	struct ui_##name *ui_object_as_##name(struct ui_object *object); \
	struct ui_##name *ui_get_lua_##name(lua_State *L, int index); \
	struct ui_##name *ui_##name##_new(struct interface *interface, const char *name, struct ui_region *parent);

	OBJECT_DECLARE_CAST(button);
	OBJECT_DECLARE_CAST(check_button);
	OBJECT_DECLARE_CAST(color_select);
	OBJECT_DECLARE_CAST(cooldown);
	OBJECT_DECLARE_CAST(dress_up_model);
	OBJECT_DECLARE_CAST(edit_box);
	OBJECT_DECLARE_CAST(font);
	OBJECT_DECLARE_CAST(font_string);
	OBJECT_DECLARE_CAST(frame);
	OBJECT_DECLARE_CAST(game_tooltip);
	OBJECT_DECLARE_CAST(layered_region);
	OBJECT_DECLARE_CAST(message_frame);
	OBJECT_DECLARE_CAST(minimap);
	OBJECT_DECLARE_CAST(model);
	OBJECT_DECLARE_CAST(model_ffx);
	OBJECT_DECLARE_CAST(movie_frame);
	OBJECT_DECLARE_CAST(player_model);
	OBJECT_DECLARE_CAST(region);
	OBJECT_DECLARE_CAST(scroll_frame);
	OBJECT_DECLARE_CAST(scrolling_message_frame);
	OBJECT_DECLARE_CAST(simple_html);
	OBJECT_DECLARE_CAST(slider);
	OBJECT_DECLARE_CAST(status_bar);
	OBJECT_DECLARE_CAST(tabard_model);
	OBJECT_DECLARE_CAST(taxi_route_frame);
	OBJECT_DECLARE_CAST(texture);
	OBJECT_DECLARE_CAST(world_frame);

#undef OBJECT_DECLARE_CAST

bool ui_register_method(struct jks_array *methods, const char *name, int (*func)(lua_State*));

#define UI_REGISTER_METHOD(method) \
	do \
	{ \
		if (!ui_register_method(methods, #method, lua_##method)) \
			return false; \
	} while (0)

#define LUA_METHOD(name, type) \
	LUA_VERBOSE_METHOD(name); \
	int argc = lua_gettop(L); \
	if (argc < 1) \
		return luaL_error(L, #name " expected"); \
	struct ui_##type *type = ui_get_lua_##type(L, 1); \
	if (!type) \
		return luaL_argerror(L, 1, #name " expected"); \

#endif
