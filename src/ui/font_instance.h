#ifndef UI_FONT_INSTANCE_H
#define UI_FONT_INSTANCE_H

#include "itf/enum.h"

#include "ui/object.h"
#include "ui/value.h"
#include "ui/color.h"

#ifdef interface
# undef interface
#endif

struct interface_font;
struct interface;
struct ui_font;

typedef void (*ui_font_instance_changed_cb_t)(struct ui_object *object);

struct ui_font_instance_callbacks
{
	ui_font_instance_changed_cb_t on_font_height_changed;
	ui_font_instance_changed_cb_t on_color_changed;
	ui_font_instance_changed_cb_t on_shadow_color_changed;
	ui_font_instance_changed_cb_t on_spacing_changed;
	ui_font_instance_changed_cb_t on_outline_changed;
	ui_font_instance_changed_cb_t on_monochrome_changed;
	ui_font_instance_changed_cb_t on_justify_h_changed;
	ui_font_instance_changed_cb_t on_justify_v_changed;
};

struct ui_font_instance
{
	struct interface *interface;
	struct optional_ui_value font_height;
	struct optional_ui_color color;
	struct optional_ui_color shadow_color;
	struct optional_float spacing;
	struct optional_outline_type outline;
	struct optional_bool monochrome;
	struct optional_justify_h_type justify_h;
	struct optional_justify_v_type justify_v;
	struct ui_font *font_instance;
	struct interface_font *render_font;
	char *font;
	struct ui_object *object;
	const struct ui_font_instance_callbacks *callbacks;
};

void ui_font_instance_init(struct interface *interface, struct ui_font_instance *font_instance, struct ui_object *object, const struct ui_font_instance_callbacks *callbacks);
void ui_font_instance_destroy(struct ui_font_instance *font_instance);
struct interface_font *ui_font_instance_get_render_font(const struct ui_font_instance *font_instance);
const struct ui_value *ui_font_instance_get_font_height(const struct ui_font_instance *font_instance);
const struct ui_color *ui_font_instance_get_color(const struct ui_font_instance *font_instance);
void ui_font_instance_set_color(struct ui_font_instance *font_instance, const struct ui_color *color);
const struct ui_color *ui_font_instance_get_shadow_color(const struct ui_font_instance *font_instance);
void ui_font_instance_set_shadow_color(struct ui_font_instance *font_instance, const struct ui_color *color);
float ui_font_instance_get_spacing(const struct ui_font_instance *font_instance);
void ui_font_instance_set_spacing(struct ui_font_instance *font_instance, float spacing);
enum outline_type ui_font_instance_get_outline(const struct ui_font_instance *font_instance);
void ui_font_instance_set_outline(struct ui_font_instance *font_instance, enum outline_type outline);
bool ui_font_instance_get_monochrome(const struct ui_font_instance *font_instance);
void ui_font_instance_set_monochrome(struct ui_font_instance *font_instance, bool monochrome);
enum justify_v_type ui_font_instance_get_justify_v(const struct ui_font_instance *font_instance);
void ui_font_instance_set_justify_v(struct ui_font_instance *font_instance, enum justify_v_type justify);
enum justify_h_type ui_font_instance_get_justify_h(const struct ui_font_instance *font_instance);
void ui_font_instance_set_justify_h(struct ui_font_instance *font_instance, enum justify_h_type justify);
bool ui_font_instance_register_methods(struct jks_array *methods);
struct ui_font_instance *ui_get_lua_font_instance(lua_State *L, int index);

#endif
