#ifndef UI_FRAME_H
#define UI_FRAME_H

#include "itf/enum.h"

#include "ui/region.h"

#include <jks/hmap.h>

#ifdef interface
# undef interface
#endif

struct ui_layered_region;
struct ui_backdrop;

struct ui_frame_script
{
	struct lua_script *script;
	bool is_method; /* should self pointer be pushed on stack first */
};

struct ui_frame
{
	struct ui_region region;
	struct ui_backdrop *backdrop;
	struct jks_hmap attributes; /* char*, int */
	struct jks_hmap scripts; /* char*, struct ui_frame_script* */
	struct jks_array layers[DRAWLAYER_HIGHLIGHT + 1]; /* ui_layered_region_t* */
	struct jks_array registered_events; /* enum event_type */
	struct jks_array regions_to_load; /* ui_region_t* */
	struct jks_array frames; /* ui_frame_t* */
	char *parent_name;
	enum frame_strata strata;
	bool top_level;
	bool movable;
	bool resizable;
	int level;
	bool clamped_to_screen;
	bool is_protected;
	bool mouse_wheel_enabled;
	int id;
	float scale;
};

extern const struct ui_object_vtable ui_frame_vtable;

void ui_frame_render_layers(struct ui_frame *frame);
void ui_frame_render_childs(struct ui_frame *frame);
bool ui_frame_execute_script(struct ui_frame *frame, const char *name, size_t args);
void ui_frame_add_child(struct ui_frame *frame, struct ui_frame *child);
void ui_frame_remove_child(struct ui_frame *frame, struct ui_frame *child);
void ui_frame_set_id(struct ui_frame *frame, int id);
void ui_frame_set_strata(struct ui_frame *frame, enum frame_strata strata);
bool ui_frame_has_registered_event(struct ui_frame *frame, enum event_type event);
void ui_frame_register_event(struct ui_frame *frame, enum event_type event);
void ui_frame_unregister_event(struct ui_frame *frame, enum event_type event);

#endif
