#include "gx/wmo_portals.h"
#include "gx/frame.h"
#include "gx/wmo.h"

#include "graphics.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <gfx/device.h>

#include <libwow/wmo.h>

#include <string.h>
#include <assert.h>
#include <math.h>

MEMORY_DECL(GX);

struct gx_wmo_portal
{
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	struct vec4f plane;
	uint16_t indices_offset;
	uint16_t indices_nb;
};

struct gx_wmo_portals_init_data
{
	struct shader_wmo_portals_input *vertexes;
	uint16_t *indices;
	uint32_t vertexes_nb;
	uint32_t indices_nb;
};

static void clear_init_data(struct gx_wmo_portals_init_data *init_data)
{
	if (!init_data)
		return;
	mem_free(MEM_GX, init_data->vertexes);
	mem_free(MEM_GX, init_data->indices);
	mem_free(MEM_GX, init_data);
}

static void portal_dtr(void *ptr)
{
	struct gx_wmo_portal *portal = ptr;
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_delete_buffer(g_wow->device, &portal->uniform_buffers[i]);
}

void gx_wmo_portals_init(struct gx_wmo_portals *portals)
{
	portals->init_data = NULL;
	jks_array_init(&portals->portals, sizeof(struct gx_wmo_portal), portal_dtr, &jks_array_memory_fn_GX);
	portals->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		portals->uniform_buffers[i] = GFX_BUFFER_INIT();
	portals->vertexes_buffer = GFX_BUFFER_INIT();
	portals->indices_buffer = GFX_BUFFER_INIT();
}

void gx_wmo_portals_destroy(struct gx_wmo_portals *portals)
{
	clear_init_data(portals->init_data);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_delete_buffer(g_wow->device, &portals->uniform_buffers[i]);
	gfx_delete_buffer(g_wow->device, &portals->vertexes_buffer);
	gfx_delete_buffer(g_wow->device, &portals->indices_buffer);
	gfx_delete_attributes_state(g_wow->device, &portals->attributes_state);
	jks_array_destroy(&portals->portals);
}

bool gx_wmo_portals_load(struct gx_wmo_portals *portals, const struct wow_mopt_data *mopt, uint32_t mopt_nb, const struct wow_vec3f *mopv, uint32_t mopv_nb)
{
	if (!mopt_nb)
		return true;
	portals->init_data = mem_zalloc(MEM_GX, sizeof(*portals->init_data));
	if (!portals->init_data)
	{
		LOG_ERROR("allocation failed");
		return false;
	}
	struct gx_wmo_portal *portal = jks_array_grow(&portals->portals, mopt_nb);
	if (!portals)
	{
		LOG_ERROR("failed to resize wmo portals");
		goto err;
	}
	portals->init_data->vertexes_nb = 0;
	portals->init_data->indices_nb = 0;
	for (uint32_t i = 0; i < mopt_nb; ++i)
	{
		for (size_t j = 0; j < RENDER_FRAMES_COUNT; ++j)
			portal[i].uniform_buffers[j] = GFX_BUFFER_INIT();
		portal[i].indices_offset = portals->init_data->indices_nb;
		portal[i].indices_nb = (mopt[i].count - 2) * 3;
		VEC4_SET(portal[i].plane, mopt[i].normal.x, mopt[i].normal.y, mopt[i].normal.z, mopt[i].distance);
		portals->init_data->vertexes_nb += mopt[i].count;
		portals->init_data->indices_nb += (mopt[i].count - 2) * 3;
	}
	portals->init_data->vertexes = mem_malloc(MEM_GX, sizeof(*portals->init_data->vertexes) * portals->init_data->vertexes_nb);
	if (!portals->init_data->vertexes)
	{
		LOG_ERROR("allocation failed");
		goto err;
	}
	portals->init_data->indices = mem_malloc(MEM_GX, sizeof(*portals->init_data->indices) * portals->init_data->indices_nb);
	if (!portals->init_data->indices)
	{
		LOG_ERROR("allocation failed");
		goto err;
	}
	uint32_t vertexes_pos = 0;
	uint32_t indices_pos = 0;
	for (uint32_t i = 0; i < mopt_nb; ++i)
	{
		const struct wow_mopt_data *data = &mopt[i];
		uint32_t base = data->start_vertex;
		uint32_t tmp = vertexes_pos;
		for (uint32_t j = 0; j < data->count; ++j)
		{
			struct shader_wmo_portals_input *vertex = &portals->init_data->vertexes[vertexes_pos++];
			assert(base + j < mopv_nb);
			VEC3_CPY(vertex->position, mopv[base + j]);
			switch (j % 6)
			{
				case 0:
					VEC4_SET(vertex->color, 1, 0, 0, .3);
					break;
				case 1:
					VEC4_SET(vertex->color, 0, 1, 0, .3);
					break;
				case 2:
					VEC4_SET(vertex->color, 0, 0, 1, .3);
					break;
				case 3:
					VEC4_SET(vertex->color, 1, 1, 0, .3);
					break;
				case 4:
					VEC4_SET(vertex->color, 0, 1, 1, .3);
					break;
				case 5:
					VEC4_SET(vertex->color, 1, 0, 1, .3);
					break;
			}
		}
		for (int32_t j = 0; j < data->count - 2; ++j)
		{
			portals->init_data->indices[indices_pos++] = tmp;
			portals->init_data->indices[indices_pos++] = tmp + j + 1;
			portals->init_data->indices[indices_pos++] = tmp + j + 2;
		}
	}
	return true;

err:
	clear_init_data(portals->init_data);
	portals->init_data = NULL;
	jks_array_resize(&portals->portals, 0);
	jks_array_shrink(&portals->portals);
	return false;
}

void gx_wmo_portals_initialize(struct gx_wmo_portals *portals)
{
	if (!portals->portals.size)
		return;
	gfx_create_buffer(g_wow->device, &portals->vertexes_buffer, GFX_BUFFER_VERTEXES, portals->init_data->vertexes, portals->init_data->vertexes_nb * sizeof(*portals->init_data->vertexes), GFX_BUFFER_IMMUTABLE);
	gfx_create_buffer(g_wow->device, &portals->indices_buffer, GFX_BUFFER_INDICES, portals->init_data->indices, portals->init_data->indices_nb * sizeof(*portals->init_data->indices), GFX_BUFFER_IMMUTABLE);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_create_buffer(g_wow->device, &portals->uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_wmo_portals_model_block), GFX_BUFFER_STREAM);
	clear_init_data(portals->init_data);
	portals->init_data = NULL;
	gfx_attribute_bind_t binds[] =
	{
		{&portals->vertexes_buffer, sizeof(struct shader_wmo_portals_input), offsetof(struct shader_wmo_portals_input, position)},
		{&portals->vertexes_buffer, sizeof(struct shader_wmo_portals_input), offsetof(struct shader_wmo_portals_input, color)},
	};
	gfx_create_attributes_state(g_wow->device, &portals->attributes_state, binds, sizeof(binds) / sizeof(*binds), &portals->indices_buffer, GFX_INDEX_UINT16);
	for (size_t i = 0; i < portals->portals.size; ++i)
	{
		struct gx_wmo_portal *portal = JKS_ARRAY_GET(&portals->portals, i, struct gx_wmo_portal);
		for (size_t j = 0; j < RENDER_FRAMES_COUNT; ++j)
			gfx_create_buffer(g_wow->device, &portal->uniform_buffers[j], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_wmo_portals_mesh_block), GFX_BUFFER_STREAM);
	}
}

void gx_wmo_portals_render(struct gx_wmo_portals *portals, struct gx_wmo_instance *instance)
{
	if (!portals->portals.size)
		return;
	struct shader_wmo_portals_model_block model_block;
	model_block.mvp = instance->render_frames[g_wow->draw_frame_id].mvp;
	gfx_set_buffer_data(&portals->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
	gfx_bind_constant(g_wow->device, 1, &portals->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	gfx_bind_attributes_state(g_wow->device, &portals->attributes_state, &g_wow->graphics->wmo_portals_input_layout);
	struct vec4f rpos;
	struct vec4f tmp;
	VEC3_CPY(tmp, g_wow->cull_frame->view_pos);
	tmp.w = 1;
	MAT4_VEC4_MUL(rpos, instance->m_inv, tmp);
	for (size_t i = 0; i < portals->portals.size; ++i)
	{
		struct gx_wmo_portal *portal = JKS_ARRAY_GET(&portals->portals, i, struct gx_wmo_portal);
		struct shader_wmo_portals_mesh_block mesh_block;
		float v = fabsf(VEC3_DOT(rpos, portal->plane) + portal->plane.w) / 50.f;
		if (v > 1)
			v = 1;
		if (v < 0)
			v = 0;
		VEC4_SET(mesh_block.color, v, v, v, 1);
		gfx_set_buffer_data(&portal->uniform_buffers[g_wow->draw_frame_id], &mesh_block, sizeof(mesh_block), 0);
		gfx_bind_constant(g_wow->device, 0, &portal->uniform_buffers[g_wow->draw_frame_id], sizeof(mesh_block), 0);
		gfx_draw_indexed(g_wow->device, portal->indices_nb, portal->indices_offset);
	}
}
