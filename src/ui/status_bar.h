#ifndef UI_STATUS_BAR_H
#define UI_STATUS_BAR_H

#include "ui/frame.h"
#include "ui/color.h"

#ifdef interface
# undef interface
#endif

struct ui_texture;

struct ui_status_bar
{
	struct ui_frame frame;
	struct ui_texture *texture;
	struct ui_color color;
	float min_value;
	float max_value;
	float default_value;
	float value;
};

extern const struct ui_object_vtable ui_status_bar_vtable;

void ui_status_bar_set_color(struct ui_status_bar *status_bar, const struct ui_color *color);
void ui_status_bar_set_min_value(struct ui_status_bar *status_bar, float value);
void ui_status_bar_set_max_value(struct ui_status_bar *status_bar, float value);
void ui_status_bar_set_value(struct ui_status_bar *status_bar, float value);

#endif
