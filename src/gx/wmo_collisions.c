#include "gx/wmo_collisions.h"
#include "gx/wmo.h"

#include "graphics.h"
#include "shaders.h"
#include "memory.h"
#include "wow.h"
#include "log.h"

#include <libwow/wmo_group.h>

#include <gfx/device.h>

struct gx_wmo_collisions_init_data
{
	struct vec3f *vertexes;
};

static void clear_init_data(struct gx_wmo_collisions_init_data *init_data)
{
	if (!init_data)
		return;
	mem_free(MEM_GX, init_data->vertexes);
	mem_free(MEM_GX, init_data);
}

void gx_wmo_collisions_init(struct gx_wmo_collisions *collisions)
{
	collisions->init_data = NULL;
	collisions->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		collisions->triangles_uniform_buffers[i] = GFX_BUFFER_INIT();
		collisions->lines_uniform_buffers[i] = GFX_BUFFER_INIT();
	}
	collisions->vertexes_buffer = GFX_BUFFER_INIT();
}

void gx_wmo_collisions_destroy(struct gx_wmo_collisions *collisions)
{
	clear_init_data(collisions->init_data);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		gfx_delete_buffer(g_wow->device, &collisions->triangles_uniform_buffers[i]);
		gfx_delete_buffer(g_wow->device, &collisions->lines_uniform_buffers[i]);
	}
	gfx_delete_buffer(g_wow->device, &collisions->vertexes_buffer);
	gfx_delete_attributes_state(g_wow->device, &collisions->attributes_state);
}

bool gx_wmo_collisions_load(struct gx_wmo_collisions *collisions, const uint16_t *mobr, uint32_t mobr_nb, const uint16_t *movi, const struct wow_vec3f *movt, const struct wow_mopy_data *mopy)
{
	if (!mobr_nb)
		return true;
	uint8_t *set = mem_zalloc(MEM_GX, (mobr_nb + 7) / 8);
	if (!set)
	{
		LOG_ERROR("allocation failed");
		return false;
	}
	collisions->init_data = mem_zalloc(MEM_GX, sizeof(*collisions->init_data));
	if (!collisions->init_data)
	{
		LOG_ERROR("allocation failed");
		mem_free(MEM_GX, set);
		return false;
	}
	collisions->init_data->vertexes = mem_malloc(MEM_GX, sizeof(*collisions->init_data->vertexes) * mobr_nb * 3);
	if (!collisions->init_data->vertexes)
	{
		LOG_ERROR("allocation failed");
		goto err;
	}
	collisions->indices_nb = 0;
	for (uint32_t i = 0; i < mobr_nb; ++i)
	{
		struct vec3f *vertex = &collisions->init_data->vertexes[collisions->indices_nb];
		uint32_t triangle_id = mobr[i];
		if (set[triangle_id / 8] & (1 << (triangle_id % 8)))
			continue;
		uint8_t flags = mopy[triangle_id].flags;
		if (!((flags & WOW_MOPY_FLAGS_COLLISION) || ((flags & WOW_MOPY_FLAGS_RENDER) && !(flags & WOW_MOPY_FLAGS_DETAIL))) && mopy[triangle_id].material_id != 0xFF)
			continue;
		set[triangle_id / 8] |= (1 << (triangle_id % 8));
		collisions->indices_nb += 3;
		VEC3_CPY(vertex[0], movt[movi[3 * triangle_id + 0]]);
		VEC3_CPY(vertex[1], movt[movi[3 * triangle_id + 1]]);
		VEC3_CPY(vertex[2], movt[movi[3 * triangle_id + 2]]);
	}
	mem_free(MEM_GX, set);
	return true;

err:
	clear_init_data(collisions->init_data);
	collisions->init_data = NULL;
	mem_free(MEM_GX, set);
	return false;
}

void gx_wmo_collisions_initialize(struct gx_wmo_collisions *collisions)
{
	if (!collisions->indices_nb)
		return;
	gfx_create_buffer(g_wow->device, &collisions->vertexes_buffer, GFX_BUFFER_VERTEXES, collisions->init_data->vertexes, collisions->indices_nb * sizeof(*collisions->init_data->vertexes), GFX_BUFFER_IMMUTABLE);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		gfx_create_buffer(g_wow->device, &collisions->triangles_uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_wmo_collisions_mesh_block), GFX_BUFFER_STREAM);
		gfx_create_buffer(g_wow->device, &collisions->lines_uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_wmo_collisions_mesh_block), GFX_BUFFER_STREAM);
	}
	clear_init_data(collisions->init_data);
	collisions->init_data = NULL;
	const struct gfx_attribute_bind binds[] =
	{
		{&collisions->vertexes_buffer, sizeof(struct vec3f), 0},
	};
	gfx_create_attributes_state(g_wow->device, &collisions->attributes_state, binds, sizeof(binds) / sizeof(*binds), NULL, 0);
}

void gx_wmo_collisions_render(struct gx_wmo_collisions *collisions, const struct gx_wmo_instance **instances, size_t instances_nb, size_t group_idx, bool triangles)
{
	if (!collisions->attributes_state.handle.ptr)
		return;
	if (!collisions->indices_nb)
		return;
	struct shader_wmo_collisions_mesh_block mesh_block;
	if (triangles)
		VEC4_SET(mesh_block.color, .2, 1, .2, .1);
	else
		VEC4_SET(mesh_block.color, .2, 1, .2, .4);
	gfx_buffer_t *uniform_buffer = triangles ? &collisions->triangles_uniform_buffers[g_wow->draw_frame_id] : &collisions->lines_uniform_buffers[g_wow->draw_frame_id];
	gfx_bind_attributes_state(g_wow->device, &collisions->attributes_state, &g_wow->graphics->wmo_collisions_input_layout);
	gfx_set_buffer_data(uniform_buffer, &mesh_block, sizeof(mesh_block), 0);
	gfx_bind_constant(g_wow->device, 0, uniform_buffer, sizeof(mesh_block), 0);
	gfx_set_line_width(g_wow->device, 1);
	for (size_t i = 0; i < instances_nb; ++i)
	{
		struct gx_wmo_group_instance *group_instance = jks_array_get(&instances[i]->groups, group_idx);
		if (group_instance->render_frames[g_wow->draw_frame_id].culled)
			continue;
		gfx_bind_constant(g_wow->device, 1, &instances[i]->uniform_buffers[g_wow->draw_frame_id], sizeof(struct shader_wmo_model_block), 0);
		gfx_draw(g_wow->device, collisions->indices_nb, 0);
	}
}
