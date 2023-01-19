#ifndef GX_COLLISIONS_H
#define GX_COLLISIONS_H

#include "shaders.h"

#include <gfx/objects.h>

#include <stddef.h>

struct collision_triangle;

struct gx_collisions
{
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t triangles_uniform_buffer;
	gfx_buffer_t lines_uniform_buffer;
	gfx_buffer_t positions_buffer;
	uint32_t triangles_nb;
	bool initialized;
};

void gx_collisions_init(struct gx_collisions *collisions);
void gx_collisions_destroy(struct gx_collisions *collisions);
void gx_collisions_update(struct gx_collisions *collisions, struct collision_triangle *triangles, size_t triangles_nb);
void gx_collisions_render(struct gx_collisions *collisions, bool triangles);

#endif
