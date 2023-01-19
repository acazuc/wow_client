#ifndef GX_AABB_H
#define GX_AABB_H

#include "shaders.h"

#include <jks/aabb.h>

#include <gfx/objects.h>

#define GX_AABB_INIT  (1 << 0)
#define GX_AABB_DIRTY (1 << 1)

struct gx_aabb
{
	struct shader_aabb_mesh_block mesh_block;
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t positions_buffer;
	gfx_buffer_t indices_buffer;
	struct vec4f color;
	struct aabb aabb;
	uint32_t indices_nb;
	float line_width;
	uint8_t flags;
};

void gx_aabb_init(struct gx_aabb *aabb, struct vec4f color, float line_width);
void gx_aabb_destroy(struct gx_aabb *aabb);
void gx_aabb_update(struct gx_aabb *aabb, const struct mat4f *mvp);
void gx_aabb_render(struct gx_aabb *aabb);

#endif
