#include "lagometer.h"

#include "shaders.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <gfx/window.h>
#include <gfx/device.h>

#define WIDTH 500

static const struct gfx_input_layout_bind g_binds[] =
{
	{GFX_ATTR_R32G32_FLOAT      , sizeof(struct shader_gui_input), offsetof(struct shader_gui_input, position)},
	{GFX_ATTR_R32G32B32A32_FLOAT, sizeof(struct shader_gui_input), offsetof(struct shader_gui_input, color)},
};

struct lagometer *lagometer_new(void)
{
	struct lagometer *lagometer = mem_zalloc(MEM_GENERIC, sizeof(*lagometer));
	if (!lagometer)
		return NULL;
	lagometer->pos = 0;
	lagometer->vertexes_data = mem_zalloc(MEM_GENERIC, sizeof(*lagometer->vertexes_data) * WIDTH * 8);
	if (!lagometer->vertexes_data)
	{
		LOG_ERROR("failed to alloc lagometer vertexes");
		goto err;
	}
	lagometer->pipeline_state = GFX_PIPELINE_STATE_INIT();
	lagometer->vertexes_buffer = GFX_BUFFER_INIT();
	lagometer->input_layout = GFX_INPUT_LAYOUT_INIT();
	gfx_create_buffer(g_wow->device, &lagometer->vertexes_buffer, GFX_BUFFER_VERTEXES, NULL, sizeof(struct shader_gui_input) * WIDTH * 8, GFX_BUFFER_STREAM);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		lagometer->uniform_buffers[i] = GFX_BUFFER_INIT();
		gfx_create_buffer(g_wow->device, &lagometer->uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_gui_model_block), GFX_BUFFER_STREAM);
	}
	const struct gfx_attribute_bind binds[] =
	{
		{&lagometer->vertexes_buffer, sizeof(struct shader_gui_input), offsetof(struct shader_gui_input, position)},
		{&lagometer->vertexes_buffer, sizeof(struct shader_gui_input), offsetof(struct shader_gui_input, color)},
	};
	gfx_create_attributes_state(g_wow->device, &lagometer->attributes_state, binds, sizeof(binds) / sizeof(*binds), NULL, 0);
	gfx_create_input_layout(g_wow->device, &lagometer->input_layout, g_binds, sizeof(g_binds) / sizeof(*g_binds), &g_wow->shaders->gui);
	lagometer->blend_state = GFX_BLEND_STATE_INIT();
	lagometer->depth_stencil_state = GFX_DEPTH_STENCIL_STATE_INIT();
	lagometer->rasterizer_state = GFX_RASTERIZER_STATE_INIT();
	gfx_create_depth_stencil_state(g_wow->device, &lagometer->depth_stencil_state, false, false, GFX_CMP_ALWAYS, false, 0, GFX_CMP_NEVER, 0, 0, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP);
	gfx_create_rasterizer_state(g_wow->device, &lagometer->rasterizer_state, GFX_FILL_SOLID, GFX_CULL_NONE, GFX_FRONT_CCW, true);
	gfx_create_blend_state(g_wow->device, &lagometer->blend_state, true, GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE_MINUS_SRC_ALPHA, GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE_MINUS_SRC_ALPHA, GFX_EQUATION_ADD, GFX_EQUATION_ADD, GFX_COLOR_MASK_ALL);
	gfx_create_pipeline_state(g_wow->device, &lagometer->pipeline_state, &g_wow->shaders->gui, &lagometer->rasterizer_state, &lagometer->depth_stencil_state, &lagometer->blend_state, &lagometer->input_layout, GFX_PRIMITIVE_LINES);
	return lagometer;

err:
	mem_free(MEM_GENERIC, lagometer->vertexes_data);
	mem_free(MEM_GENERIC, lagometer);
	return NULL;
}

void lagometer_delete(struct lagometer *lagometer)
{
	if (!lagometer)
		return;
	gfx_delete_blend_state(g_wow->device, &lagometer->blend_state);
	gfx_delete_depth_stencil_state(g_wow->device, &lagometer->depth_stencil_state);
	gfx_delete_rasterizer_state(g_wow->device, &lagometer->rasterizer_state);
	gfx_delete_input_layout(g_wow->device, &lagometer->input_layout);
	gfx_delete_attributes_state(g_wow->device, &lagometer->attributes_state);
	gfx_delete_pipeline_state(g_wow->device, &lagometer->pipeline_state);
	gfx_delete_buffer(g_wow->device, &lagometer->vertexes_buffer);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_delete_buffer(g_wow->device, &lagometer->uniform_buffers[i]);
	mem_free(MEM_GENERIC, lagometer->vertexes_data);
	mem_free(MEM_GENERIC, lagometer);
}

void lagometer_draw(struct lagometer *lagometer)
{
	MAT4_ORTHO(float, lagometer->mat, 0, (float)g_wow->render_width, (float)g_wow->render_height, 0, -2, 2);
	float alpha = .5;
	for (size_t i = 0; i < WIDTH * 8; ++i)
		lagometer->vertexes_data[i].position.x--;
	float divisor = 100000;
	float y = g_wow->render_height;
	VEC2_SET(lagometer->vertexes_data[lagometer->pos * 8 + 0].position, WIDTH - 1, y);
	y -= g_wow->last_frame_cull_duration / divisor;
	VEC2_SET(lagometer->vertexes_data[lagometer->pos * 8 + 1].position, WIDTH - 1, y);
	VEC2_SET(lagometer->vertexes_data[lagometer->pos * 8 + 2].position, WIDTH - 1, y);
	y -= g_wow->last_frame_draw_duration / divisor;
	VEC2_SET(lagometer->vertexes_data[lagometer->pos * 8 + 3].position, WIDTH - 1, y);
	VEC2_SET(lagometer->vertexes_data[lagometer->pos * 8 + 4].position, WIDTH - 1, y);
	y -= g_wow->last_frame_update_duration / divisor;
	VEC2_SET(lagometer->vertexes_data[lagometer->pos * 8 + 5].position, WIDTH - 1, y);
	VEC2_SET(lagometer->vertexes_data[lagometer->pos * 8 + 6].position, WIDTH - 1, y);
	y -= g_wow->last_frame_misc_duration / divisor;
	VEC2_SET(lagometer->vertexes_data[lagometer->pos * 8 + 7].position, WIDTH - 1, y);
	VEC4_SET(lagometer->vertexes_data[lagometer->pos * 8 + 0].color, 1, 0, 1, alpha);
	VEC4_SET(lagometer->vertexes_data[lagometer->pos * 8 + 1].color, 1, 0, 1, alpha);
	uint64_t duration = g_wow->last_frame_cull_duration + g_wow->last_frame_draw_duration;
	if (duration > 16666666)
	{
		VEC4_SET(lagometer->vertexes_data[lagometer->pos * 8 + 2].color, 0, 0, 1, alpha);
		VEC4_SET(lagometer->vertexes_data[lagometer->pos * 8 + 3].color, 0, 0, 1, alpha);
	}
	else
	{
		if (duration < 16666666 / 2)
		{
			float tmp = duration / 100000.f / 167.f * 2;
			if (tmp < 0)
				tmp = 0;
			if (tmp > 1)
				tmp = 1;
			VEC4_SET(lagometer->vertexes_data[lagometer->pos * 8 + 2].color, tmp, 1, 0, alpha);
			VEC4_SET(lagometer->vertexes_data[lagometer->pos * 8 + 3].color, tmp, 1, 0, alpha);
		}
		else
		{
			float tmp = (duration - 16666666 / 2) / 100000.f / 167.f * 2;
			if (tmp < 0)
				tmp = 0;
			if (tmp > 1)
				tmp = 1;
			VEC4_SET(lagometer->vertexes_data[lagometer->pos * 8 + 2].color, 1, 1 - tmp, 0, alpha);
			VEC4_SET(lagometer->vertexes_data[lagometer->pos * 8 + 3].color, 1, 1 - tmp, 0, alpha);
		}
	}
	VEC4_SET(lagometer->vertexes_data[lagometer->pos * 8 + 4].color, 1, 1, 1, alpha);
	VEC4_SET(lagometer->vertexes_data[lagometer->pos * 8 + 5].color, 1, 1, 1, alpha);
	VEC4_SET(lagometer->vertexes_data[lagometer->pos * 8 + 6].color, 0, 1, 1, alpha);
	VEC4_SET(lagometer->vertexes_data[lagometer->pos * 8 + 7].color, 0, 1, 1, alpha);
	lagometer->pos = (lagometer->pos + 1) % WIDTH;
	gfx_set_buffer_data(&lagometer->vertexes_buffer, lagometer->vertexes_data, sizeof(*lagometer->vertexes_data) * WIDTH * 8, 0);
	struct shader_gui_model_block model_block;
	model_block.mvp = lagometer->mat;
	gfx_set_buffer_data(&lagometer->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
	gfx_bind_attributes_state(g_wow->device, &lagometer->attributes_state, &lagometer->input_layout);
	gfx_bind_pipeline_state(g_wow->device, &lagometer->pipeline_state);
	gfx_bind_constant(g_wow->device, 1, &lagometer->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	gfx_set_line_width(g_wow->device, 1);
	gfx_draw(g_wow->device, WIDTH * 8, 0);
}
