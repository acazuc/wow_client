#include "gx/m2_bones.h"
#include "gx/m2.h"

#include "graphics.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <jks/vec4.h>
#include <jks/vec3.h>

#include <gfx/device.h>

struct gx_m2_bones_init_data
{
	struct shader_m2_bones_input *points_vertexes;
	struct shader_m2_bones_input *lines_vertexes;
};

static void clear_init_data(struct gx_m2_bones_init_data *init_data)
{
	if (!init_data)
		return;
	mem_free(MEM_GX, init_data->points_vertexes);
	mem_free(MEM_GX, init_data->lines_vertexes);
	mem_free(MEM_GX, init_data);
}

void gx_m2_bones_init(struct gx_m2_bones *bones)
{
	bones->init_data = NULL;
	bones->points_attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	bones->lines_attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	bones->points_vertexes_buffer = GFX_BUFFER_INIT();
	bones->lines_vertexes_buffer = GFX_BUFFER_INIT();
}

void gx_m2_bones_destroy(struct gx_m2_bones *bones)
{
	clear_init_data(bones->init_data);
	gfx_delete_buffer(g_wow->device, &bones->points_vertexes_buffer);
	gfx_delete_buffer(g_wow->device, &bones->lines_vertexes_buffer);
	gfx_delete_attributes_state(g_wow->device, &bones->points_attributes_state);
	gfx_delete_attributes_state(g_wow->device, &bones->lines_attributes_state);
}

bool gx_m2_bones_load(struct gx_m2_bones *bones, const struct wow_m2_bone *wow_bones, uint32_t nb)
{
	uint32_t lines_pos;
	if (!nb)
	{
		bones->points_indices_nb = 0;
		bones->lines_indices_nb = 0;
		return true;
	}
	bones->init_data = mem_zalloc(MEM_GX, sizeof(*bones->init_data));
	if (!bones->init_data)
		return false;
	bones->points_indices_nb = nb;
	bones->lines_indices_nb = 0;
	for (size_t i = 0; i < nb; ++i)
	{
		if (wow_bones[i].parent_bone != -1)
			bones->lines_indices_nb++;
	}
	bones->lines_indices_nb *= 2;
	bones->init_data->points_vertexes = mem_malloc(MEM_GX, sizeof(*bones->init_data->points_vertexes) * bones->points_indices_nb);
	if (!bones->init_data->points_vertexes)
	{
		LOG_ERROR("allocation failed");
		goto err;
	}
	bones->init_data->lines_vertexes = mem_malloc(MEM_GX, sizeof(*bones->init_data->lines_vertexes) * bones->lines_indices_nb);
	if (!bones->init_data->lines_vertexes)
	{
		LOG_ERROR("allocation failed");
		goto err;
	}
	lines_pos = 0;
	for (size_t i = 0; i < nb; ++i)
	{
		const struct wow_m2_bone *bone = &wow_bones[i];
		bones->init_data->points_vertexes[i].bone = i;
		VEC3_CPY(bones->init_data->points_vertexes[i].position, bone->pivot);
		struct vec3f color;
		VEC3_SET(color, 1, 1, 0);
		VEC4_SET(bones->init_data->points_vertexes[i].color, color.x, color.y, color.z, 1);
		if (bone->parent_bone != -1)
		{
			const struct wow_m2_bone *parent_bone = &wow_bones[bone->parent_bone];
			bones->init_data->lines_vertexes[lines_pos + 0].bone = i;
			bones->init_data->lines_vertexes[lines_pos + 1].bone = bone->parent_bone;
			VEC3_CPY(bones->init_data->lines_vertexes[lines_pos + 0].position, bone->pivot);
			VEC3_CPY(bones->init_data->lines_vertexes[lines_pos + 1].position, parent_bone->pivot);
			VEC4_SET(bones->init_data->lines_vertexes[lines_pos + 0].color, 0, 1, 1, 1);
			VEC4_SET(bones->init_data->lines_vertexes[lines_pos + 1].color, 1, 0, 1, 1);
			lines_pos += 2;
		}
	}
	return true;

err:
	clear_init_data(bones->init_data);
	bones->init_data = NULL;
	return false;
}

void gx_m2_bones_initialize(struct gx_m2_bones *bones)
{
	if (!bones->init_data)
		return;
	if (bones->points_indices_nb)
	{
		gfx_create_buffer(g_wow->device, &bones->points_vertexes_buffer, GFX_BUFFER_VERTEXES, bones->init_data->points_vertexes, bones->points_indices_nb * sizeof(*bones->init_data->points_vertexes), GFX_BUFFER_IMMUTABLE);
		const struct gfx_attribute_bind binds[] =
		{
			{&bones->points_vertexes_buffer, sizeof(struct shader_m2_bones_input), offsetof(struct shader_m2_bones_input, position)},
			{&bones->points_vertexes_buffer, sizeof(struct shader_m2_bones_input), offsetof(struct shader_m2_bones_input, color)},
			{&bones->points_vertexes_buffer, sizeof(struct shader_m2_bones_input), offsetof(struct shader_m2_bones_input, bone)},
		};
		gfx_create_attributes_state(g_wow->device, &bones->points_attributes_state, binds, sizeof(binds) / sizeof(*binds), NULL, 0);
	}
	if (bones->lines_indices_nb)
	{
		gfx_create_buffer(g_wow->device, &bones->lines_vertexes_buffer, GFX_BUFFER_VERTEXES, bones->init_data->lines_vertexes, bones->lines_indices_nb * sizeof(*bones->init_data->lines_vertexes), GFX_BUFFER_IMMUTABLE);
		const struct gfx_attribute_bind binds[] =
		{
			{&bones->lines_vertexes_buffer, sizeof(struct shader_m2_bones_input), offsetof(struct shader_m2_bones_input, position)},
			{&bones->lines_vertexes_buffer, sizeof(struct shader_m2_bones_input), offsetof(struct shader_m2_bones_input, color)},
			{&bones->lines_vertexes_buffer, sizeof(struct shader_m2_bones_input), offsetof(struct shader_m2_bones_input, bone)},
		};
		gfx_create_attributes_state(g_wow->device, &bones->lines_attributes_state, binds, sizeof(binds) / sizeof(*binds), NULL, 0);
	}
	clear_init_data(bones->init_data);
	bones->init_data = NULL;
}

void gx_m2_bones_render_points(struct gx_m2_bones *bones, const struct gx_m2_instance **instances, size_t nb)
{
	if (!bones->points_indices_nb)
		return;
	if (nb == 0 || (nb == 1 && !instances[0]->uniform_buffers[g_wow->draw_frame_id].handle.u64))
		return;
	gfx_set_point_size(g_wow->device, 5);
	gfx_bind_attributes_state(g_wow->device, &bones->points_attributes_state, &g_wow->graphics->m2_bones_points_input_layout);
	for (size_t i = 0; i < nb; ++i)
	{
		const struct gx_m2_instance *instance = instances[i];
		if (!instance->uniform_buffers[g_wow->draw_frame_id].handle.u64)
			continue;
		gfx_bind_constant(g_wow->device, 1, &instance->uniform_buffers[g_wow->draw_frame_id], sizeof(struct shader_m2_model_block), 0);
		gfx_draw(g_wow->device, bones->points_indices_nb, 0);
	}
}

void gx_m2_bones_render_lines(struct gx_m2_bones *bones, const struct gx_m2_instance **instances, size_t nb)
{
	if (!bones->lines_indices_nb)
		return;
	if (nb == 0 || (nb == 1 && !instances[0]->uniform_buffers[g_wow->draw_frame_id].handle.u64))
		return;
	gfx_set_line_width(g_wow->device, 1);
	gfx_bind_attributes_state(g_wow->device, &bones->lines_attributes_state, &g_wow->graphics->m2_bones_lines_input_layout);
	for (size_t i = 0; i < nb; ++i)
	{
		const struct gx_m2_instance *instance = instances[i];
		if (!instance->uniform_buffers[g_wow->draw_frame_id].handle.u64)
			continue;
		gfx_bind_constant(g_wow->device, 1, &instance->uniform_buffers[g_wow->draw_frame_id], sizeof(struct shader_m2_model_block), 0);
		gfx_draw(g_wow->device, bones->lines_indices_nb, 0);
	}
}
