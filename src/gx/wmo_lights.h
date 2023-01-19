#ifndef GX_WMO_LIGHTS_H
#define GX_WMO_LIGHTS_H

#include "shaders.h"

#include <jks/mat4.h>

#include <gfx/objects.h>

#include <stdbool.h>

struct gx_wmo_lights_init_data;
struct wow_molt_data;

struct gx_wmo_lights
{
	struct gx_wmo_lights_init_data *init_data;
	struct shader_basic_model_block model_block;
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t vertexes_buffer;
	uint32_t lights_nb;
};

void gx_wmo_lights_init(struct gx_wmo_lights *lights);
void gx_wmo_lights_destroy(struct gx_wmo_lights *lights);
bool gx_wmo_lights_load(struct gx_wmo_lights *lights, const struct wow_molt_data *molt, uint32_t nb);
void gx_wmo_lights_initialize(struct gx_wmo_lights *lights);
void gx_wmo_lights_update(struct gx_wmo_lights *lights, const struct mat4f *mvp);
void gx_wmo_lights_render(struct gx_wmo_lights *lights);

#endif
