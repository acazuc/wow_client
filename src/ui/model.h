#ifndef UI_MODEL_H
#define UI_MODEL_H

#include "ui/frame.h"
#include "ui/color.h"

#include <gfx/objects.h>

#ifdef interface
# undef interface
#endif

struct gx_m2_instance;

struct ui_model
{
	struct ui_frame frame;
	struct gx_m2_instance *m2;
	gfx_buffer_t particles_uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	struct ui_color fog_color;
	uint32_t camera;
	float scale;
	float fog_near;
	float fog_far;
};

extern const struct ui_object_vtable ui_model_vtable;

void ui_model_set_model(struct ui_model *model, const char *file);
void ui_model_set_scale(struct ui_model *model, float scale);
void ui_model_set_camera(struct ui_model *model, uint32_t camera);
void ui_model_set_fog_near(struct ui_model *model, float fog_near);
void ui_model_set_fog_far(struct ui_model *model, float fog_far);
void ui_model_set_fog_color(struct ui_model *model, const struct ui_color *color);

#endif
