#ifndef GX_M2_LIGHTS_H
#define GX_M2_LIGHTS_H

#include <gfx/objects.h>

#include <stdbool.h>
#include <stddef.h>

struct gx_m2_lights_init_data;
struct gx_m2_instance;
struct wow_m2_light;

struct gx_m2_lights
{
	struct gx_m2_lights_init_data *init_data;
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t vertexes_buffer;
	uint32_t lights_nb;
};

void gx_m2_lights_init(struct gx_m2_lights *lights);
void gx_m2_lights_destroy(struct gx_m2_lights *lights);
bool gx_m2_lights_load(struct gx_m2_lights *lights, const struct wow_m2_light *wow_lights, size_t nb);
void gx_m2_lights_initialize(struct gx_m2_lights *lights);
void gx_m2_lights_render(struct gx_m2_lights *lights, const struct gx_m2_instance **instances, size_t nb);

#endif
