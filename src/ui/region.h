#ifndef UI_REGION_H
#define UI_REGION_H

#include "ui/dimension.h"
#include "ui/anchor.h"
#include "ui/object.h"
#include "ui/inset.h"

#include <gfx/events.h>

#include <jks/array.h>

#ifdef interface
# undef interface
#endif

struct xml_layout_frame;

struct ui_region
{
	struct ui_object object;
	struct ui_region *parent; /* TODO: Check that there isn't an anchor relative to child; check that there isn't loop in anchors / parents */
	struct optional_ui_dimension size;
	struct jks_array related_anchors; /* struct ui_anchor* */
	struct jks_array anchors; /* struct ui_anchor* */
	char *real_name;
	char *name;
	struct ui_inset hit_rect_insets;
	bool hidden;
	bool hovered;
	bool clicked;
	bool mouse_enabled;
	bool keyboard_enabled;
	bool dirty_alpha;
	float effective_alpha;
	bool dirty_coords; /* if pos or size is dirty. Should update position, clear dirsty flag, update related anchors */
	int32_t left;
	int32_t right;
	int32_t top;
	int32_t bottom;
};

extern const struct ui_object_vtable ui_region_vtable;

void ui_region_print_debug(struct ui_region *region);
void ui_region_calc_effective_alpha(struct ui_region *region);
void ui_region_set_size(struct ui_region *region, int32_t width, int32_t height);
bool ui_region_is_visible(struct ui_region *region);
const struct ui_anchor *ui_region_get_center_anchor(const struct ui_region *region);
const struct ui_anchor *ui_region_get_left_anchor(const struct ui_region *region);
const struct ui_anchor *ui_region_get_right_anchor(const struct ui_region *region);
const struct ui_anchor *ui_region_get_top_anchor(const struct ui_region *region);
const struct ui_anchor *ui_region_get_bottom_anchor(const struct ui_region *region);
int32_t ui_region_get_left(struct ui_region *region);
int32_t ui_region_get_right(struct ui_region *region);
int32_t ui_region_get_top(struct ui_region *region);
int32_t ui_region_get_bottom(struct ui_region *region);
int32_t ui_region_get_width(struct ui_region *region);
int32_t ui_region_get_height(struct ui_region *region);
void ui_region_clear_anchors(struct ui_region *region);
void ui_region_add_anchor(struct ui_region *region, struct ui_anchor *anchor);
void ui_region_add_related_anchor(struct ui_region *region, struct ui_anchor *anchor);
void ui_region_remove_related_anchor(struct ui_region *region, struct ui_anchor *anchor);

#endif
