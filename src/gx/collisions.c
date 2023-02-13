#include "gx/collisions.h"

#include "map/map.h"

#include "graphics.h"
#include "memory.h"
#include "camera.h"
#include "wow.h"
#include "log.h"

#include <gfx/device.h>

#include <math.h>

void gx_collisions_init(struct gx_collisions *collisions)
{
	collisions->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	collisions->triangles_uniform_buffer = GFX_BUFFER_INIT();
	collisions->lines_uniform_buffer = GFX_BUFFER_INIT();
	collisions->positions_buffer = GFX_BUFFER_INIT();
	collisions->triangles_nb = 0;
	collisions->initialized = false;
}

void gx_collisions_destroy(struct gx_collisions *collisions)
{
	gfx_delete_attributes_state(g_wow->device, &collisions->attributes_state);
	gfx_delete_buffer(g_wow->device, &collisions->lines_uniform_buffer);
	gfx_delete_buffer(g_wow->device, &collisions->triangles_uniform_buffer);
	gfx_delete_buffer(g_wow->device, &collisions->positions_buffer);
}

void gx_collisions_update(struct gx_collisions *collisions, struct collision_triangle *triangles, size_t triangles_nb)
{
	if (collisions->attributes_state.handle.ptr)
		gfx_delete_attributes_state(g_wow->device, &collisions->attributes_state);
	if (collisions->positions_buffer.handle.ptr)
		gfx_delete_buffer(g_wow->device, &collisions->positions_buffer);
	struct vec3f *points = mem_malloc(MEM_GX, sizeof(*points) * triangles_nb * 3);
	if (!points)
	{
		LOG_ERROR("points allocation failed");
		return;
	}
	for (size_t i = 0; i < triangles_nb; ++i)
	{
		struct vec3f *dst = &points[i * 3];
		dst[0] = triangles[i].points[0];
		dst[1] = triangles[i].points[1];
		dst[2] = triangles[i].points[2];
	}
	gfx_create_buffer(g_wow->device, &collisions->positions_buffer, GFX_BUFFER_VERTEXES, points, sizeof(*points) * triangles_nb * 3, GFX_BUFFER_IMMUTABLE);
	mem_free(MEM_GX, points);
	const struct gfx_attribute_bind binds[] =
	{
		{&collisions->positions_buffer, sizeof(struct vec3f), 0},
	};
	gfx_create_attributes_state(g_wow->device, &collisions->attributes_state, binds, sizeof(binds) / sizeof(*binds), NULL, GFX_INDEX_UINT16);
	collisions->triangles_nb = triangles_nb;
}

static void initialize(struct gx_collisions *collisions)
{
	gfx_create_buffer(g_wow->device, &collisions->triangles_uniform_buffer, GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_aabb_mesh_block), GFX_BUFFER_STREAM);
	gfx_create_buffer(g_wow->device, &collisions->lines_uniform_buffer, GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_aabb_mesh_block), GFX_BUFFER_STREAM);
	collisions->initialized = true;
}

void gx_collisions_render(struct gx_collisions *collisions, bool triangles)
{
	if (!collisions->attributes_state.handle.ptr)
		return;
	if (!collisions->triangles_nb)
		return;
	if (!collisions->initialized)
		initialize(collisions);
	gfx_bind_attributes_state(g_wow->device, &collisions->attributes_state, &g_wow->graphics->collisions_input_layout);
	struct shader_aabb_mesh_block mesh_block;
	if (triangles)
	{
		float f = sin(g_wow->frametime / 50000000.);
		VEC4_SET(mesh_block.color, 1, .1 + .1 * f, .1 + .1 * f, .1);
	}
	else
	{
		VEC4_SET(mesh_block.color, 1, .2, .2, .5);
	}
	mesh_block.mvp = g_wow->view_camera->vp;
	gfx_buffer_t *uniform_buffer = triangles ? &collisions->triangles_uniform_buffer : &collisions->lines_uniform_buffer;
	gfx_set_buffer_data(uniform_buffer, &mesh_block, sizeof(mesh_block), 0);
	gfx_bind_constant(g_wow->device, 0, uniform_buffer, sizeof(mesh_block), 0);
	gfx_set_line_width(g_wow->device, 1);
	gfx_draw(g_wow->device, collisions->triangles_nb * 3, 0);
}
