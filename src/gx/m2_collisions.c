#include "gx/m2_collisions.h"
#include "gx/m2.h"

#include "graphics.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <gfx/device.h>

#include <jks/vec3.h>

#include <inttypes.h>
#include <string.h>
#include <math.h>

struct gx_m2_collisions_init_data
{
	struct shader_m2_collisions_input *vertexes;
	uint32_t vertexes_nb;
	uint16_t *indices;
};

static void clear_init_data(struct gx_m2_collisions_init_data *init_data)
{
	if (!init_data)
		return;
	mem_free(MEM_GX, init_data->vertexes);
	mem_free(MEM_GX, init_data->indices);
}

void gx_m2_collisions_init(struct gx_m2_collisions *collisions)
{
	collisions->init_data = NULL;
	collisions->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	collisions->vertexes_buffer = GFX_BUFFER_INIT();
	collisions->indices_buffer = GFX_BUFFER_INIT();
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		collisions->triangles_uniform_buffers[i] = GFX_BUFFER_INIT();
		collisions->lines_uniform_buffers[i] = GFX_BUFFER_INIT();
	}
}

void gx_m2_collisions_destroy(struct gx_m2_collisions *collisions)
{
	clear_init_data(collisions->init_data);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		gfx_delete_buffer(g_wow->device, &collisions->triangles_uniform_buffers[i]);
		gfx_delete_buffer(g_wow->device, &collisions->lines_uniform_buffers[i]);
	}
	gfx_delete_buffer(g_wow->device, &collisions->indices_buffer);
	gfx_delete_buffer(g_wow->device, &collisions->vertexes_buffer);
	gfx_delete_attributes_state(g_wow->device, &collisions->attributes_state);
}

bool gx_m2_collisions_load(struct gx_m2_collisions *collisions, struct gx_m2 *m2)
{
	collisions->triangles_nb = m2->collision_triangles_nb / 3;
	if (!collisions->triangles_nb)
		return true;
	collisions->init_data = mem_zalloc(MEM_GX, sizeof(*collisions->init_data));
	if (!collisions->init_data)
	{
		LOG_ERROR("allocation failed");
		goto err;
	}
	collisions->init_data->vertexes_nb = m2->collision_vertexes_nb;
	collisions->init_data->vertexes = mem_malloc(MEM_GX, sizeof(*collisions->init_data->vertexes) * m2->collision_vertexes_nb);
	if (!collisions->init_data->vertexes)
	{
		LOG_ERROR("allocation failed");
		goto err;
	}
	for (size_t i = 0; i < m2->collision_vertexes_nb; ++i)
	{
		VEC3_CPY(collisions->init_data->vertexes[i].position, m2->collision_vertexes[i]);
	}
	collisions->init_data->indices = mem_malloc(MEM_GX, sizeof(*collisions->init_data->indices) * m2->collision_triangles_nb);
	if (!collisions->init_data->indices)
	{
		LOG_ERROR("allocation failed");
		goto err;
	}
	memcpy(collisions->init_data->indices, m2->collision_triangles, sizeof(*collisions->init_data->indices) * m2->collision_triangles_nb);
	return true;
err:
	clear_init_data(collisions->init_data);
	collisions->init_data = NULL;
	return false;
}

void gx_m2_collisions_initialize(struct gx_m2_collisions *collisions)
{
	if (!collisions->triangles_nb)
		return;
	gfx_create_buffer(g_wow->device, &collisions->vertexes_buffer, GFX_BUFFER_VERTEXES, collisions->init_data->vertexes, collisions->init_data->vertexes_nb * sizeof(*collisions->init_data->vertexes), GFX_BUFFER_IMMUTABLE);
	gfx_create_buffer(g_wow->device, &collisions->indices_buffer, GFX_BUFFER_INDICES, collisions->init_data->indices, collisions->triangles_nb * sizeof(*collisions->init_data->indices) * 3, GFX_BUFFER_IMMUTABLE);
	const struct gfx_attribute_bind binds[] =
	{
		{&collisions->vertexes_buffer},
	};
	gfx_create_attributes_state(g_wow->device, &collisions->attributes_state, binds, sizeof(binds) / sizeof(*binds), &collisions->indices_buffer, GFX_INDEX_UINT16);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		gfx_create_buffer(g_wow->device, &collisions->triangles_uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_m2_collisions_mesh_block), GFX_BUFFER_STREAM);
		gfx_create_buffer(g_wow->device, &collisions->lines_uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_m2_collisions_mesh_block), GFX_BUFFER_STREAM);
	}
	clear_init_data(collisions->init_data);
	collisions->init_data = NULL;
}

void gx_m2_collisions_render(struct gx_m2_collisions *collisions, const struct gx_m2_instance **instances, size_t instances_nb, bool triangles)
{
	if (!instances_nb)
		return;
	if (!collisions->triangles_nb)
		return;
	gfx_bind_attributes_state(g_wow->device, &collisions->attributes_state, &g_wow->graphics->m2_collisions_input_layout);
	struct shader_m2_collisions_mesh_block mesh_block;
	if (triangles)
		VEC4_SET(mesh_block.color, .5, .5, 1, .25);
	else
		VEC4_SET(mesh_block.color, .5, .5, 1, .5);
	gfx_buffer_t *uniform_buffer = triangles ? &collisions->triangles_uniform_buffers[g_wow->draw_frame_id] : &collisions->lines_uniform_buffers[g_wow->draw_frame_id];
	gfx_set_buffer_data(uniform_buffer, &mesh_block, sizeof(mesh_block), 0);
	gfx_bind_constant(g_wow->device, 0, uniform_buffer, sizeof(mesh_block), 0);
	gfx_set_line_width(g_wow->device, 1);
	for (size_t i = 0; i < instances_nb; ++i)
	{
		const struct gx_m2_instance *instance = instances[i];
		if (!instance->uniform_buffers[g_wow->draw_frame_id].handle.u64)
			continue;
		gfx_bind_constant(g_wow->device, 1, &instance->uniform_buffers[g_wow->draw_frame_id], sizeof(struct shader_m2_model_block), 0);
		gfx_draw_indexed(g_wow->device, collisions->triangles_nb * 3, 0);
	}
}
