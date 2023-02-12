#ifndef UI_INTERFACE_H
#define UI_INTERFACE_H

#include "itf/enum.h"

#include <jks/array.h>
#include <jks/mat4.h>
#include <jks/hmap.h>

#include <gfx/objects.h>
#include <gfx/events.h>

#include <stdbool.h>
#include <stddef.h>

#ifdef interface
# undef interface
#endif

struct xml_layout_frame;
typedef struct lua_State lua_State;
struct interface_font;
struct lua_script;
struct addon;
typedef void* gfx_cursor_t;

struct ui_edit_box;
struct ui_object;
struct ui_region;
struct ui_frame;
struct ui_font;

enum interface_blend_state
{
	INTERFACE_BLEND_OPAQUE,
	INTERFACE_BLEND_ALPHA,
	INTERFACE_BLEND_ADD,
	INTERFACE_BLEND_MOD,
	INTERFACE_BLEND_LAST
};

struct interface
{
	gfx_rasterizer_state_t rasterizer_state;
	gfx_pipeline_state_t pipeline_states[INTERFACE_BLEND_LAST];
	gfx_input_layout_t input_layout;
	gfx_blend_state_t blend_states[INTERFACE_BLEND_LAST];
	gfx_cursor_t *cursors[CURSOR_LAST];
	struct jks_array root_frames[FRAMESTRATA_PARENT + 1]; /* ui_frame_t* */
	struct jks_array frames_events[EVENT_LAST]; /* ui_frame_t* */
	struct jks_array ext_addons; /* addon_t* */
	struct jks_array xml_ui; /* xml_ui_t */
	struct jks_array addons; /* addon_t* */
	struct jks_hmap virtual_layout_frames; /* char*, xml_layout_frame_t* */
	struct jks_hmap render_fonts; /* char*, interface_font_model_t */
	struct jks_hmap regions; /* char*, ui_region_t* */
	struct jks_hmap frames; /* char*, ui_frame_t* */
	struct jks_hmap fonts; /* char*, ui_font_t* */
	gfx_attributes_state_t attributes_state;
	gfx_depth_stencil_state_t depth_stencil_state;
	struct lua_script *error_script;
	lua_State *L;
	struct ui_edit_box *active_input;
	gfx_texture_t white_pixel;
	struct mat4f mat;
	int32_t width;
	int32_t height;
	int32_t mouse_x;
	int32_t mouse_y;
	bool switch_framescreen;
	bool switch_gluescreen;
	bool initialized;
	bool is_gluescreen;
};

struct interface *interface_new(void);
void interface_delete(struct interface *interface);
void interface_clear(struct interface *interface);
void interface_render(struct interface *interface);
void interface_enable_scissor(int32_t x, int32_t y, int32_t width, int32_t height);
void interface_disable_scissor(void);
void interface_on_window_resized(struct interface *interface, gfx_resize_event_t *event);
void interface_on_mouse_move(struct interface *interface, gfx_pointer_event_t *event);
bool interface_on_mouse_down(struct interface *interface, gfx_mouse_event_t *event);
bool interface_on_mouse_up(struct interface *interface, gfx_mouse_event_t *event);
bool interface_on_mouse_scroll(struct interface *interface, gfx_scroll_event_t *event);
bool interface_on_key_down(struct interface *interface, gfx_key_event_t *event);
bool interface_on_key_up(struct interface *interface, gfx_key_event_t *event);
bool interface_on_key_press(struct interface *interface, gfx_key_event_t *event);
bool interface_on_char(struct interface *interface, gfx_char_event_t *event);
void interface_set_cursor(struct interface *interface, enum cursor_type cursor);
bool interface_load_xml(struct interface *interface, struct addon *addon, const char *filename, const char *data, size_t len);
bool interface_load_lua(struct interface *interface, const char *data, size_t len, const char *source);
void interface_set_active_input(struct interface *interface, struct ui_edit_box *edit_box);
struct interface_font *interface_ref_render_font(struct interface *interface, const char *name, uint32_t size);
void interface_unref_render_font(struct interface *interface, struct interface_font *font);
void interface_register_root_frame(struct interface *interface, struct ui_frame *frame);
void interface_unregister_root_frame(struct interface *interface, struct ui_frame *frame);
void interface_register_frame_event(struct interface *interface, struct ui_frame *frame, enum event_type event);
void interface_unregister_frame_event(struct interface *interface, struct ui_frame *frame, enum event_type event);
void interface_execute_event(struct interface *interface, enum event_type event, int params);
void interface_register_virtual_layout_frame(struct interface *interface, const char *name, struct xml_layout_frame *layout_frame);
struct xml_layout_frame *interface_get_virtual_layout_frame(struct interface *interface, const char *name);
void interface_unregister_virtual_layout_frame(struct interface *interface, const char *name);
void interface_register_region(struct interface *interface, const char *name, struct ui_region *region);
struct ui_region *interface_get_region(struct interface *interface, const char *name);
void interface_unregister_region(struct interface *interface, const char *name);
void interface_register_frame(struct interface *interface, const char *name, struct ui_frame *frame);
struct ui_frame *interface_get_frame(struct interface *interface, const char *name);
void interface_unregister_frame(struct interface *interface, const char *name);
void interface_register_font(struct interface *interface, const char *name, struct ui_font *font);
struct ui_font *interface_get_font(struct interface *interface, const char *name);
void interface_unregister_font(struct interface *interface, const char *name);
void interface_set_error_script(struct interface *interface, struct lua_script *script);
struct addon *interface_get_addon(struct interface *interface, const char *name);
const char *lua_gfx_mouse_to_string(enum gfx_mouse_button button);
const char *lua_gfx_key_to_string(enum gfx_key_code key);

#endif
