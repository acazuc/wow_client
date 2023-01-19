#ifndef UI_SLIDER_H
#define UI_SLIDER_H

#include "ui/frame.h"

#ifdef interface
# undef interface
#endif

struct ui_texture;

struct ui_slider
{
	struct ui_frame frame;
	struct ui_texture *thumb_texture;
	enum draw_layer draw_layer;
	float min_value;
	float max_value;
	float value_step;
	float default_value;
	float value;
	enum orientation orientation;
};

extern const struct ui_object_vtable ui_slider_vtable;

void ui_slider_set_min_value(struct ui_slider *slider, float value);
void ui_slider_set_max_value(struct ui_slider *slider, float value);
void ui_slider_set_value_step(struct ui_slider *slider, float value);
void ui_slider_set_default_value(struct ui_slider *slider, float value);
void ui_slider_set_value(struct ui_slider *slider, float value);

#endif
