#ifndef GX_M2_BONES_H
#define GX_M2_BONES_H

#include <gfx/objects.h>

#include <stdbool.h>

struct gx_m2_bones_init_data;
struct gx_m2_instance;
struct wow_m2_bone;

struct gx_m2_bones
{
	struct gx_m2_bones_init_data *init_data;
	gfx_attributes_state_t points_attributes_state;
	gfx_attributes_state_t lines_attributes_state;
	gfx_buffer_t points_vertexes_buffer;
	gfx_buffer_t lines_vertexes_buffer;
	uint32_t points_indices_nb;
	uint32_t lines_indices_nb;
};

void gx_m2_bones_init(struct gx_m2_bones *bones);
void gx_m2_bones_destroy(struct gx_m2_bones *bones);
bool gx_m2_bones_load(struct gx_m2_bones *bones, const struct wow_m2_bone *wow_bones, uint32_t nb);
void gx_m2_bones_initialize(struct gx_m2_bones *bones);
void gx_m2_bones_render_points(struct gx_m2_bones *bones, const struct gx_m2_instance **instances, uint32_t instances_nb);
void gx_m2_bones_render_lines(struct gx_m2_bones *bones, const struct gx_m2_instance **instances, uint32_t instances_nb);

#endif
