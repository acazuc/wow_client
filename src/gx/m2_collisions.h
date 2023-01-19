#ifndef GX_M2_COLLISIONS_H
#define GX_M2_COLLISIONS_H

#include <gfx/objects.h>

struct gx_m2_collisions_init_data;
struct gx_m2_instance;
struct gx_m2;

struct gx_m2_collisions
{
	struct gx_m2_collisions_init_data *init_data;
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t triangles_uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t lines_uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t vertexes_buffer;
	gfx_buffer_t indices_buffer;
	uint32_t triangles_nb;
};

void gx_m2_collisions_init(struct gx_m2_collisions *collisions);
void gx_m2_collisions_destroy(struct gx_m2_collisions *collisions);
bool gx_m2_collisions_load(struct gx_m2_collisions *collisions, struct gx_m2 *m2);
void gx_m2_collisions_initialize(struct gx_m2_collisions *collisions);
void gx_m2_collisions_render(struct gx_m2_collisions *collisions, const struct gx_m2_instance **instances, uint32_t instances_nb, bool triangles);

#endif
