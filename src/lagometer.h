#ifndef LAGOMETER_H
#define LAGOMETER_H

#include <jks/mat4.h>
#include <jks/vec4.h>
#include <jks/vec2.h>

#include <gfx/objects.h>

struct shader_gui_input;

struct lagometer
{
	gfx_depth_stencil_state_t depth_stencil_state;
	gfx_rasterizer_state_t rasterizer_state;
	gfx_pipeline_state_t pipeline_state;
	struct shader_gui_input *vertexes_data;
	gfx_input_layout_t input_layout;
	gfx_blend_state_t blend_state;
	gfx_texture_t white_pixel;
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t vertexes_buffer;
	uint32_t pos;
	struct mat4f mat;
};

struct lagometer *lagometer_new(void);
void lagometer_delete(struct lagometer *lagometer);
void lagometer_draw(struct lagometer *lagometer);

#endif
