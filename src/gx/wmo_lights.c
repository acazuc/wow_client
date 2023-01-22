#include "gx/wmo_lights.h"

#include "graphics.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <gfx/device.h>

#include <libwow/wmo.h>

struct gx_wmo_lights_init_data
{
	struct shader_basic_input *vertexes;
};

static void clear_init_data(struct gx_wmo_lights_init_data *init_data)
{
	if (!init_data)
		return;
	mem_free(MEM_GX, init_data->vertexes);
	mem_free(MEM_GX, init_data);
}

void gx_wmo_lights_init(struct gx_wmo_lights *lights)
{
	lights->init_data = NULL;
	lights->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		lights->uniform_buffers[i] = GFX_BUFFER_INIT();
	lights->vertexes_buffer = GFX_BUFFER_INIT();
}

void gx_wmo_lights_destroy(struct gx_wmo_lights *lights)
{
	clear_init_data(lights->init_data);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_delete_buffer(g_wow->device, &lights->uniform_buffers[i]);
	gfx_delete_buffer(g_wow->device, &lights->vertexes_buffer);
	gfx_delete_attributes_state(g_wow->device, &lights->attributes_state);
}

bool gx_wmo_lights_load(struct gx_wmo_lights *lights, const struct wow_molt_data *molt, uint32_t nb)
{
	lights->lights_nb = nb;
	if (!nb)
		return true;
	lights->init_data = mem_zalloc(MEM_GX, sizeof(*lights->init_data));
	if (!lights->init_data)
	{
		LOG_ERROR("allocation failed");
		return false;
	}
	lights->init_data->vertexes = mem_malloc(MEM_GX, sizeof(*lights->init_data->vertexes) * lights->lights_nb);
	if (!lights->init_data->vertexes)
	{
		LOG_ERROR("allocation failed");
		goto err;
	}
	for (uint32_t i = 0; i < nb; ++i)
	{
		const struct wow_molt_data *data = &molt[i];
		VEC3_SET(lights->init_data->vertexes[i].position, data->position.x, data->position.y, data->position.z);
		VEC4_SET(lights->init_data->vertexes[i].color, data->intensity, data->intensity, data->intensity, 1);
	}
	return true;

err:
	clear_init_data(lights->init_data);
	lights->init_data = NULL;
	return false;
}

void gx_wmo_lights_initialize(struct gx_wmo_lights *lights)
{
	if (!lights->lights_nb)
		return;
	gfx_create_buffer(g_wow->device, &lights->vertexes_buffer, GFX_BUFFER_VERTEXES, lights->init_data->vertexes, lights->lights_nb * sizeof(*lights->init_data->vertexes), GFX_BUFFER_IMMUTABLE);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_create_buffer(g_wow->device, &lights->uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_basic_model_block), GFX_BUFFER_STREAM);
	clear_init_data(lights->init_data);
	lights->init_data = NULL;
	gfx_attribute_bind_t binds[] =
	{
		{&lights->vertexes_buffer, sizeof(struct shader_basic_input), offsetof(struct shader_basic_input, position)},
		{&lights->vertexes_buffer, sizeof(struct shader_basic_input), offsetof(struct shader_basic_input, color)},
	};
	gfx_create_attributes_state(g_wow->device, &lights->attributes_state, binds, sizeof(binds) / sizeof(*binds), NULL, 0);
}

void gx_wmo_lights_update(struct gx_wmo_lights *lights, const struct mat4f *mvp)
{
	if (!lights->lights_nb)
		return;
	lights->model_block.mvp = *mvp;
}

void gx_wmo_lights_render(struct gx_wmo_lights *lights)
{
	if (!lights->lights_nb)
		return;
	gfx_set_point_size(g_wow->device, 5);
	gfx_bind_attributes_state(g_wow->device, &lights->attributes_state, &g_wow->graphics->wmo_lights_input_layout);
	gfx_set_buffer_data(&lights->uniform_buffers[g_wow->draw_frame_id], &lights->model_block, sizeof(lights->model_block), 0);
	gfx_bind_constant(g_wow->device, 1, &lights->uniform_buffers[g_wow->draw_frame_id], sizeof(lights->model_block), 0);
	gfx_draw(g_wow->device, lights->lights_nb, 0);
}
