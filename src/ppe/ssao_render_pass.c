#include "render_pass.h"

#include "ppe/render_target.h"

#include "shaders.h"
#include "camera.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <gfx/device.h>

struct ssao_render_pass
{
	struct render_pass render_pass;
	gfx_depth_stencil_state_t depth_stencil_state;
	gfx_attributes_state_t attributes_state;
	gfx_rasterizer_state_t rasterizer_state;
	gfx_pipeline_state_t denoiser_pipeline_state;
	gfx_pipeline_state_t pipeline_state;
	gfx_render_target_t render_target;
	gfx_input_layout_t input_layout;
	gfx_blend_state_t blend_state;
	gfx_texture_t texture;
	gfx_buffer_t denoiser_uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t positions_buffer;
	gfx_buffer_t indices_buffer;
	gfx_buffer_t uv_buffer;
};

static const gfx_input_layout_bind_t g_binds[] =
{
	{GFX_ATTR_R32G32_FLOAT, sizeof(struct vec2f), 0},
	{GFX_ATTR_R32G32_FLOAT, sizeof(struct vec2f), 0},
};

static void ctr(struct render_pass *render_pass)
{
	render_pass_vtable.ctr(render_pass);
	struct ssao_render_pass *ssao = (struct ssao_render_pass*)render_pass;
	ssao->depth_stencil_state = GFX_DEPTH_STENCIL_STATE_INIT();
	ssao->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	ssao->rasterizer_state = GFX_RASTERIZER_STATE_INIT();
	ssao->denoiser_pipeline_state = GFX_PIPELINE_STATE_INIT();
	ssao->pipeline_state = GFX_PIPELINE_STATE_INIT();
	ssao->render_target = GFX_RENDER_TARGET_INIT();
	ssao->input_layout = GFX_INPUT_LAYOUT_INIT();
	ssao->blend_state = GFX_BLEND_STATE_INIT();
	ssao->positions_buffer = GFX_BUFFER_INIT();
	ssao->indices_buffer = GFX_BUFFER_INIT();
	ssao->uv_buffer = GFX_BUFFER_INIT();
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		ssao->uniform_buffers[i] = GFX_BUFFER_INIT();
		ssao->denoiser_uniform_buffers[i] = GFX_BUFFER_INIT();
	}
	ssao->texture = GFX_TEXTURE_INIT();

	gfx_create_render_target(g_wow->device, &ssao->render_target);
	render_pass->vtable->resize(render_pass, 1, 1);
	uint32_t draw_buffer = GFX_RENDERTARGET_ATTACHMENT_COLOR0;
	gfx_set_render_target_draw_buffers(&ssao->render_target, &draw_buffer, 1);
	gfx_bind_render_target(g_wow->device, NULL);

	struct vec2f positions[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
	struct vec2f uvs[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
	uint16_t indices[6] = {0, 1, 3, 3, 1, 2};
	gfx_create_buffer(g_wow->device, &ssao->positions_buffer, GFX_BUFFER_VERTEXES, positions, sizeof(positions), GFX_BUFFER_IMMUTABLE);
	gfx_create_buffer(g_wow->device, &ssao->indices_buffer, GFX_BUFFER_INDICES, indices, sizeof(indices), GFX_BUFFER_IMMUTABLE);
	gfx_create_buffer(g_wow->device, &ssao->uv_buffer, GFX_BUFFER_VERTEXES, uvs, sizeof(uvs), GFX_BUFFER_IMMUTABLE);
	gfx_create_input_layout(g_wow->device, &ssao->input_layout, g_binds, sizeof(g_binds) / sizeof(*g_binds), &g_wow->shaders->ssao);
	gfx_attribute_bind_t binds[] =
	{
		{&ssao->positions_buffer, sizeof(struct vec2f), 0},
		{&ssao->uv_buffer       , sizeof(struct vec2f), 0},
	};
	gfx_create_attributes_state(g_wow->device, &ssao->attributes_state, binds, sizeof(binds) / sizeof(*binds), &ssao->indices_buffer, GFX_INDEX_UINT16);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		gfx_create_buffer(g_wow->device, &ssao->uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_ssao_model_block), GFX_BUFFER_STREAM);
		gfx_create_buffer(g_wow->device, &ssao->denoiser_uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_ssao_denoiser_model_block), GFX_BUFFER_STREAM);
	}
	gfx_create_rasterizer_state(g_wow->device, &ssao->rasterizer_state, GFX_FILL_SOLID, GFX_CULL_NONE, GFX_FRONT_CCW, false);
	gfx_create_depth_stencil_state(g_wow->device, &ssao->depth_stencil_state, false, false, GFX_CMP_ALWAYS, false, 0, GFX_CMP_ALWAYS, 0, 0, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP);
	gfx_create_blend_state(g_wow->device, &ssao->blend_state, true, GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE_MINUS_SRC_ALPHA, GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE_MINUS_SRC_ALPHA, GFX_EQUATION_ADD, GFX_EQUATION_ADD, GFX_COLOR_MASK_ALL);
	gfx_create_pipeline_state(g_wow->device,
		&ssao->pipeline_state,
		&g_wow->shaders->ssao,
		&ssao->rasterizer_state,
		&ssao->depth_stencil_state,
		&ssao->blend_state,
		&ssao->input_layout,
		GFX_PRIMITIVE_TRIANGLES);
	gfx_create_pipeline_state(g_wow->device,
		&ssao->denoiser_pipeline_state,
		&g_wow->shaders->ssao_denoiser,
		&ssao->rasterizer_state,
		&ssao->depth_stencil_state,
		&ssao->blend_state,
		&ssao->input_layout,
		GFX_PRIMITIVE_TRIANGLES);
}

static void dtr(struct render_pass *render_pass)
{
	struct ssao_render_pass *ssao = (struct ssao_render_pass*)render_pass;
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		gfx_delete_buffer(g_wow->device, &ssao->denoiser_uniform_buffers[i]);
		gfx_delete_buffer(g_wow->device, &ssao->uniform_buffers[i]);
	}
	gfx_delete_buffer(g_wow->device, &ssao->positions_buffer);
	gfx_delete_buffer(g_wow->device, &ssao->indices_buffer);
	gfx_delete_buffer(g_wow->device, &ssao->uv_buffer);
	gfx_delete_pipeline_state(g_wow->device, &ssao->pipeline_state);
	gfx_delete_pipeline_state(g_wow->device, &ssao->denoiser_pipeline_state);
	gfx_delete_attributes_state(g_wow->device, &ssao->attributes_state);
	gfx_delete_input_layout(g_wow->device, &ssao->input_layout);
	gfx_delete_render_target(g_wow->device, &ssao->render_target);
	gfx_delete_texture(g_wow->device, &ssao->texture);
	gfx_delete_rasterizer_state(g_wow->device, &ssao->rasterizer_state);
	gfx_delete_depth_stencil_state(g_wow->device, &ssao->depth_stencil_state);
	gfx_delete_blend_state(g_wow->device, &ssao->blend_state);
	render_pass_vtable.dtr(render_pass);
}

static void resize(struct render_pass *render_pass, uint32_t width, uint32_t height)
{
	render_pass_vtable.resize(render_pass, width, height);
	struct ssao_render_pass *ssao = (struct ssao_render_pass*)render_pass;
	gfx_delete_texture(g_wow->device, &ssao->texture);
	gfx_create_texture(g_wow->device, &ssao->texture, GFX_TEXTURE_2D, GFX_B8G8R8A8, 1, width, height, 0);
	gfx_set_texture_filtering(&ssao->texture, GFX_FILTERING_LINEAR, GFX_FILTERING_LINEAR, GFX_FILTERING_NONE);
	gfx_set_texture_addressing(&ssao->texture, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP);
	gfx_set_render_target_texture(&ssao->render_target, GFX_RENDERTARGET_ATTACHMENT_COLOR0, &ssao->texture);
}

static void render_ssao(struct render_pass *render_pass, struct render_target *src)
{
	struct ssao_render_pass *ssao = (struct ssao_render_pass*)render_pass;
	gfx_bind_render_target(g_wow->device, &ssao->render_target);
	gfx_bind_pipeline_state(g_wow->device, &ssao->pipeline_state);
	gfx_set_viewport(g_wow->device, 0, 0, render_pass->width, render_pass->height);
	gfx_clear_color(g_wow->device, &ssao->render_target, GFX_RENDERTARGET_ATTACHMENT_COLOR0, (struct vec4f){0, 0, 0, 1});
	{
		struct shader_ssao_model_block model_block;
		MAT4_ORTHO(float, model_block.mvp, 0, 1, 0, 1, -2, 2);
		model_block.p = g_wow->view_camera->p;
		VEC2_SET(model_block.window_size, render_pass->width, render_pass->height);
		model_block.radius = 10;
		gfx_set_buffer_data(&ssao->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
		gfx_bind_constant(g_wow->device, 1, &ssao->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	}
	gfx_bind_attributes_state(g_wow->device, &ssao->attributes_state, &ssao->input_layout);
	const gfx_texture_t *textures[] =
	{
		&src->position_texture,
		&src->normal_texture,
	};
	gfx_bind_samplers(g_wow->device, 0, 2, textures);
	gfx_draw_indexed(g_wow->device, 6, 0);
}

static void merge_ssao(struct render_pass *render_pass, struct render_target *src, struct render_target *dst, uint32_t buffers)
{
	struct ssao_render_pass *ssao = (struct ssao_render_pass*)render_pass;
	gfx_render_target_t *render_target;
	if (dst)
	{
		render_target = &dst->render_target;
		render_target_bind(dst, buffers);
	}
	else
	{
		render_target = NULL;
		gfx_bind_render_target(g_wow->device, NULL);
	}
	gfx_bind_pipeline_state(g_wow->device, &ssao->denoiser_pipeline_state);
	gfx_set_viewport(g_wow->device, 0, 0, dst ? dst->width : g_wow->render_width, dst ? dst->height : g_wow->render_height);
	gfx_clear_color(g_wow->device, render_target, GFX_RENDERTARGET_ATTACHMENT_COLOR0, (struct vec4f){0, 0, 0, 1});
	{
		struct shader_ssao_denoiser_model_block model_block;
		MAT4_ORTHO(float, model_block.mvp, 0, 1, 0, 1, -2, 2);
		gfx_set_buffer_data(&ssao->denoiser_uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
		gfx_bind_constant(g_wow->device, 1, &ssao->denoiser_uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	}
	gfx_bind_attributes_state(g_wow->device, &ssao->attributes_state, &ssao->input_layout);
	const gfx_texture_t *textures[] =
	{
		&src->color_texture,
		&src->normal_texture,
		&src->position_texture,
		&ssao->texture,
	};
	gfx_bind_samplers(g_wow->device, 0, 4, textures);
	gfx_draw_indexed(g_wow->device, 6, 0);
}

static void process(struct render_pass *render_pass, struct render_target *src, struct render_target *dst, uint32_t buffers)
{
	render_pass_vtable.process(render_pass, src, dst, buffers);
	render_ssao(render_pass, src);
	merge_ssao(render_pass, src, dst, buffers);
}

static const struct render_pass_vtable ssao_render_pass_vtable =
{
	.ctr     = ctr,
	.dtr     = dtr,
	.process = process,
	.resize  = resize,
};

struct render_pass *ssao_render_pass_new(void)
{
	struct render_pass *render_pass = mem_malloc(MEM_PPE, sizeof(struct ssao_render_pass));
	if (!render_pass)
		return NULL;
	render_pass->vtable = &ssao_render_pass_vtable;
	render_pass->vtable->ctr(render_pass);
	return render_pass;
}
