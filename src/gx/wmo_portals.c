#include "gx/wmo_portals.h"

#include "graphics.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <gfx/device.h>

#include <libwow/wmo.h>

#include <string.h>

struct gx_wmo_portals_init_data
{
	uint32_t points;
	struct shader_basic_input *vertexes;
	uint16_t *indices;
};

static void clear_init_data(struct gx_wmo_portals_init_data *init_data)
{
	if (!init_data)
		return;
	mem_free(MEM_GX, init_data->vertexes);
	mem_free(MEM_GX, init_data->indices);
	mem_free(MEM_GX, init_data);
}

void gx_wmo_portals_init(struct gx_wmo_portals *portals)
{
	portals->init_data = NULL;
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
}

bool gx_wmo_portals_load(struct gx_wmo_portals *portals, const struct wow_mopt_data *mopt, uint32_t mopt_nb, const struct wow_vec3f *mopv, uint32_t mopv_nb)
{
	if (!mopt_nb)
	{
		portals->indices_nb = 0;
		return true;
	}
	portals->init_data = mem_zalloc(MEM_GX, sizeof(*portals->init_data));
	if (!portals->init_data)
	{
		LOG_ERROR("allocation failed");
		return false;
	}
	portals->init_data->points = mopv_nb;
	portals->init_data->vertexes = mem_malloc(MEM_GX, sizeof(*portals->init_data->vertexes) * mopv_nb);
	if (!portals->init_data->vertexes)
	{
		LOG_ERROR("allocation failed");
		goto err;
	}
	portals->indices_nb = 0;
	for (uint32_t i = 0; i < mopt_nb; ++i)
		portals->indices_nb += (mopt[i].count - 2) * 3;
	portals->init_data->indices = mem_malloc(MEM_GX, sizeof(*portals->init_data->indices) * portals->indices_nb);
	if (!portals->init_data->indices)
	{
		LOG_ERROR("allocation failed");
		goto err;
	}
	uint32_t indices_pos = 0;
	for (uint32_t i = 0; i < mopt_nb; ++i)
	{
		const wow_mopt_data_t *data = &mopt[i];
		uint32_t base = data->start_vertex;
		for (int32_t j = 0; j < data->count - 2; ++j)
		{
			portals->init_data->indices[indices_pos++] = base;
			portals->init_data->indices[indices_pos++] = base + j + 1;
			portals->init_data->indices[indices_pos++] = base + j + 2;
		}
	}
	for (uint32_t i = 0; i < mopv_nb; ++i)
	{
		VEC3_CPY(portals->init_data->vertexes[i].position, mopv[i]);
#if 0
		VEC3_SETV(portals->init_data->vertexes[i].color, i / (float)mopv_nb);
		portals->init_data->vertexes[i].color.w = .3;
#else
		switch (i % 3)
		{
			case 0:
				VEC4_SET(portals->init_data->vertexes[i].color, 1, 0, 0, .3);
				break;
			case 1:
				VEC4_SET(portals->init_data->vertexes[i].color, 0, 1, 0, .3);
				break;
			case 2:
				VEC4_SET(portals->init_data->vertexes[i].color, 0, 0, 1, .3);
				break;
		}
#endif
	}
	return true;

err:
	clear_init_data(portals->init_data);
	portals->init_data = NULL;
	return false;
}

void gx_wmo_portals_initialize(struct gx_wmo_portals *portals)
{
	if (!portals->indices_nb)
		return;
	gfx_create_buffer(g_wow->device, &portals->vertexes_buffer, GFX_BUFFER_VERTEXES, portals->init_data->vertexes, portals->init_data->points * sizeof(*portals->init_data->vertexes), GFX_BUFFER_IMMUTABLE);
	gfx_create_buffer(g_wow->device, &portals->indices_buffer, GFX_BUFFER_INDICES, portals->init_data->indices, portals->indices_nb * sizeof(*portals->init_data->indices), GFX_BUFFER_IMMUTABLE);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_create_buffer(g_wow->device, &portals->uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_basic_model_block), GFX_BUFFER_STREAM);
	clear_init_data(portals->init_data);
	portals->init_data = NULL;
	gfx_attribute_bind_t binds[] =
	{
		{&portals->vertexes_buffer, sizeof(struct shader_basic_input), offsetof(struct shader_basic_input, position)},
		{&portals->vertexes_buffer, sizeof(struct shader_basic_input), offsetof(struct shader_basic_input, color)},
	};
	gfx_create_attributes_state(g_wow->device, &portals->attributes_state, binds, sizeof(binds) / sizeof(*binds), &portals->indices_buffer, GFX_INDEX_UINT16);
}

void gx_wmo_portals_update(struct gx_wmo_portals *portals, const struct mat4f *mvp)
{
	if (!portals->indices_nb)
		return;
	portals->model_block.mvp = *mvp;
}

void gx_wmo_portals_render(struct gx_wmo_portals *portals)
{
	if (!portals->indices_nb)
		return;
	gfx_bind_attributes_state(g_wow->device, &portals->attributes_state, &g_wow->graphics->wmo_portals_input_layout);
	gfx_set_buffer_data(&portals->uniform_buffers[g_wow->draw_frame_id], &portals->model_block, sizeof(portals->model_block), 0);
	gfx_bind_constant(g_wow->device, 1, &portals->uniform_buffers[g_wow->draw_frame_id], sizeof(portals->model_block), 0);
	gfx_draw_indexed(g_wow->device, portals->indices_nb, 0);
}
