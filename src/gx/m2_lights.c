#include "gx/m2_lights.h"
#include "gx/m2.h"

#include "graphics.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <jks/vec3.h>

#include <gfx/device.h>

#include <libwow/m2.h>

struct gx_m2_lights_init_data
{
	struct shader_m2_lights_input *vertexes;
};

static void clear_init_data(struct gx_m2_lights_init_data *init_data)
{
	if (!init_data)
		return;
	mem_free(MEM_GX, init_data->vertexes);
	mem_free(MEM_GX, init_data);
}

void gx_m2_lights_init(struct gx_m2_lights *lights)
{
	lights->init_data = NULL;
	lights->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	lights->vertexes_buffer = GFX_BUFFER_INIT();
}

void gx_m2_lights_destroy(struct gx_m2_lights *lights)
{
	clear_init_data(lights->init_data);
	gfx_delete_buffer(g_wow->device, &lights->vertexes_buffer);
	gfx_delete_attributes_state(g_wow->device, &lights->attributes_state);
}

bool gx_m2_lights_load(struct gx_m2_lights *lights, const struct wow_m2_light *wow_lights, uint32_t nb)
{
	lights->lights_nb = nb;
	if (!lights->lights_nb)
		return true;
	lights->init_data = mem_zalloc(MEM_GX, sizeof(*lights->init_data));
	if (!lights->init_data)
	{
		LOG_ERROR("allocation failed");
		goto err;
	}
	lights->init_data->vertexes = mem_malloc(MEM_GX, sizeof(*lights->init_data->vertexes) * lights->lights_nb);
	if (!lights->init_data->vertexes)
	{
		LOG_ERROR("allocation failed");
		goto err;
	}
	for (uint32_t i = 0; i < nb; ++i)
	{
		const struct wow_m2_light *light = &wow_lights[i];
		VEC3_SET(lights->init_data->vertexes[i].position, light->position.x, light->position.y, light->position.z);
		VEC4_SET(lights->init_data->vertexes[i].color, 1, 0, 1, 1);
		lights->init_data->vertexes[i].bone = light->bone;
	}
	return true;

err:
	clear_init_data(lights->init_data);
	lights->init_data = NULL;
	return false;
}

void gx_m2_lights_initialize(struct gx_m2_lights *lights)
{
	if (!lights->lights_nb)
		return;
	gfx_create_buffer(g_wow->device, &lights->vertexes_buffer, GFX_BUFFER_VERTEXES, lights->init_data->vertexes, lights->lights_nb * sizeof(*lights->init_data->vertexes), GFX_BUFFER_IMMUTABLE);
	gfx_attribute_bind_t binds[] =
	{
		{&lights->vertexes_buffer, sizeof(struct shader_m2_lights_input), offsetof(struct shader_m2_lights_input, position)},
		{&lights->vertexes_buffer, sizeof(struct shader_m2_lights_input), offsetof(struct shader_m2_lights_input, color)},
		{&lights->vertexes_buffer, sizeof(struct shader_m2_lights_input), offsetof(struct shader_m2_lights_input, bone)},
	};
	gfx_create_attributes_state(g_wow->device, &lights->attributes_state, binds, sizeof(binds) / sizeof(*binds), NULL, 0);
	clear_init_data(lights->init_data);
	lights->init_data = NULL;
}

void gx_m2_lights_render(struct gx_m2_lights *lights, const struct gx_m2_instance **instances, uint32_t nb)
{
	if (!lights->lights_nb)
		return;
	if (nb == 0 || (nb == 1 && !instances[0]->uniform_buffers[g_wow->draw_frame_id].handle.u64))
		return;
	gfx_set_point_size(g_wow->device, 5);
	gfx_bind_attributes_state(g_wow->device, &lights->attributes_state, &g_wow->graphics->m2_lights_input_layout);
	for (uint32_t i = 0; i < nb; ++i)
	{
		const struct gx_m2_instance *instance = instances[i];
		if (!instance->uniform_buffers[g_wow->draw_frame_id].handle.u64)
			continue;
		gfx_bind_constant(g_wow->device, 1, &instance->uniform_buffers[g_wow->draw_frame_id], sizeof(struct shader_m2_model_block), 0);
		gfx_draw(g_wow->device, lights->lights_nb, 0);
	}
}
