#include "render_pass.h"

#include "ppe/render_target.h"

#include "shaders.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <gfx/device.h>

static const float scale_factor = .25;
static const float threshold = .5;
static const float factor = .25;

struct bloom_render_pass
{
	struct render_pass render_pass;
	gfx_depth_stencil_state_t depth_stencil_state;
	gfx_rasterizer_state_t rasterizer_state;
	gfx_pipeline_state_t merge_pipeline_state;
	gfx_pipeline_state_t bloom_pipeline_state;
	gfx_pipeline_state_t blur_pipeline_state;
	gfx_blend_state_t blend_state;
	gfx_render_target_t bloom_render_target;
	gfx_render_target_t vblur_render_target;
	gfx_render_target_t hblur_render_target;
	gfx_attributes_state_t attributes_state;
	gfx_input_layout_t input_layout;
	gfx_texture_t texture1;
	gfx_texture_t texture2;
	gfx_buffer_t merge_uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t vblur_uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t hblur_uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t bloom_uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t positions_buffer;
	gfx_buffer_t indices_buffer;
	gfx_buffer_t uv_buffer;
};

static const struct gfx_input_layout_bind g_binds[] =
{
	{GFX_ATTR_R32G32_FLOAT, sizeof(struct vec2f), 0},
	{GFX_ATTR_R32G32_FLOAT, sizeof(struct vec2f), 0},
};

static void ctr(struct render_pass *render_pass)
{
	render_pass_vtable.ctr(render_pass);
	struct bloom_render_pass *bloom = (struct bloom_render_pass*)render_pass;
	bloom->depth_stencil_state = GFX_DEPTH_STENCIL_STATE_INIT();
	bloom->bloom_render_target = GFX_RENDER_TARGET_INIT();
	bloom->vblur_render_target = GFX_RENDER_TARGET_INIT();
	bloom->hblur_render_target = GFX_RENDER_TARGET_INIT();
	bloom->rasterizer_state = GFX_RASTERIZER_STATE_INIT();
	bloom->merge_pipeline_state = GFX_PIPELINE_STATE_INIT();
	bloom->bloom_pipeline_state = GFX_PIPELINE_STATE_INIT();
	bloom->blur_pipeline_state = GFX_PIPELINE_STATE_INIT();
	bloom->blend_state = GFX_BLEND_STATE_INIT();
	bloom->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	bloom->input_layout = GFX_INPUT_LAYOUT_INIT();
	bloom->positions_buffer = GFX_BUFFER_INIT();
	bloom->indices_buffer = GFX_BUFFER_INIT();
	bloom->uv_buffer = GFX_BUFFER_INIT();
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		bloom->bloom_uniform_buffers[i] = GFX_BUFFER_INIT();
		bloom->vblur_uniform_buffers[i] = GFX_BUFFER_INIT();
		bloom->hblur_uniform_buffers[i] = GFX_BUFFER_INIT();
		bloom->merge_uniform_buffers[i] = GFX_BUFFER_INIT();
	}
	bloom->texture1 = GFX_TEXTURE_INIT();
	bloom->texture2 = GFX_TEXTURE_INIT();
	gfx_create_render_target(g_wow->device, &bloom->bloom_render_target);
	gfx_create_render_target(g_wow->device, &bloom->vblur_render_target);
	gfx_create_render_target(g_wow->device, &bloom->hblur_render_target);
	render_pass->vtable->resize(render_pass, 1, 1);
	uint32_t draw_buffer = GFX_RENDERTARGET_ATTACHMENT_COLOR0;
	gfx_set_render_target_draw_buffers(&bloom->bloom_render_target, &draw_buffer, 1);
	gfx_set_render_target_draw_buffers(&bloom->vblur_render_target, &draw_buffer, 1);
	gfx_set_render_target_draw_buffers(&bloom->hblur_render_target, &draw_buffer, 1);
	gfx_bind_render_target(g_wow->device, NULL);
	struct vec2f positions[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
	struct vec2f uvs[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
	uint16_t indices[6] = {0, 1, 3, 3, 1, 2};
	gfx_create_buffer(g_wow->device, &bloom->positions_buffer, GFX_BUFFER_VERTEXES, positions, sizeof(positions), GFX_BUFFER_IMMUTABLE);
	gfx_create_buffer(g_wow->device, &bloom->indices_buffer, GFX_BUFFER_INDICES, indices, sizeof(indices), GFX_BUFFER_IMMUTABLE);
	gfx_create_buffer(g_wow->device, &bloom->uv_buffer, GFX_BUFFER_VERTEXES, uvs, sizeof(uvs), GFX_BUFFER_IMMUTABLE);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		gfx_create_buffer(g_wow->device, &bloom->bloom_uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_bloom_model_block), GFX_BUFFER_STREAM);
		gfx_create_buffer(g_wow->device, &bloom->vblur_uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_bloom_blur_model_block), GFX_BUFFER_STREAM);
		gfx_create_buffer(g_wow->device, &bloom->hblur_uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_bloom_blur_model_block), GFX_BUFFER_STREAM);
		gfx_create_buffer(g_wow->device, &bloom->merge_uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_bloom_merge_model_block), GFX_BUFFER_STREAM);
	}
	gfx_create_input_layout(g_wow->device, &bloom->input_layout, g_binds, sizeof(g_binds) / sizeof(*g_binds), &g_wow->shaders->bloom);
	const struct gfx_attribute_bind binds[] =
	{
		{&bloom->positions_buffer},
		{&bloom->uv_buffer},
	};
	gfx_create_attributes_state(g_wow->device, &bloom->attributes_state, binds, sizeof(binds) / sizeof(*binds), &bloom->indices_buffer, GFX_INDEX_UINT16);
	gfx_create_rasterizer_state(g_wow->device, &bloom->rasterizer_state, GFX_FILL_SOLID, GFX_CULL_NONE, GFX_FRONT_CCW, false);
	gfx_create_depth_stencil_state(g_wow->device, &bloom->depth_stencil_state, false, false, GFX_CMP_ALWAYS, false, 0, GFX_CMP_ALWAYS, 0, 0, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP);
	gfx_create_blend_state(g_wow->device, &bloom->blend_state, true, GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE_MINUS_SRC_ALPHA, GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE_MINUS_SRC_ALPHA, GFX_EQUATION_ADD, GFX_EQUATION_ADD, GFX_COLOR_MASK_ALL);
	gfx_create_pipeline_state(g_wow->device,
		&bloom->merge_pipeline_state,
		&g_wow->shaders->bloom_merge,
		&bloom->rasterizer_state,
		&bloom->depth_stencil_state,
		&bloom->blend_state,
		&bloom->input_layout,
		GFX_PRIMITIVE_TRIANGLES);
	gfx_create_pipeline_state(g_wow->device,
		&bloom->bloom_pipeline_state,
		&g_wow->shaders->bloom,
		&bloom->rasterizer_state,
		&bloom->depth_stencil_state,
		&bloom->blend_state,
		&bloom->input_layout,
		GFX_PRIMITIVE_TRIANGLES);
	gfx_create_pipeline_state(g_wow->device,
		&bloom->blur_pipeline_state,
		&g_wow->shaders->bloom_blur,
		&bloom->rasterizer_state,
		&bloom->depth_stencil_state,
		&bloom->blend_state,
		&bloom->input_layout,
		GFX_PRIMITIVE_TRIANGLES);
}

static void dtr(struct render_pass *render_pass)
{
	struct bloom_render_pass *bloom = (struct bloom_render_pass*)render_pass;
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		gfx_delete_buffer(g_wow->device, &bloom->merge_uniform_buffers[i]);
		gfx_delete_buffer(g_wow->device, &bloom->vblur_uniform_buffers[i]);
		gfx_delete_buffer(g_wow->device, &bloom->hblur_uniform_buffers[i]);
		gfx_delete_buffer(g_wow->device, &bloom->bloom_uniform_buffers[i]);
	}
	gfx_delete_buffer(g_wow->device, &bloom->positions_buffer);
	gfx_delete_buffer(g_wow->device, &bloom->indices_buffer);
	gfx_delete_buffer(g_wow->device, &bloom->uv_buffer);
	gfx_delete_render_target(g_wow->device, &bloom->bloom_render_target);
	gfx_delete_render_target(g_wow->device, &bloom->hblur_render_target);
	gfx_delete_render_target(g_wow->device, &bloom->vblur_render_target);
	gfx_delete_attributes_state(g_wow->device, &bloom->attributes_state);
	gfx_delete_pipeline_state(g_wow->device, &bloom->merge_pipeline_state);
	gfx_delete_pipeline_state(g_wow->device, &bloom->bloom_pipeline_state);
	gfx_delete_pipeline_state(g_wow->device, &bloom->blur_pipeline_state);
	gfx_delete_input_layout(g_wow->device, &bloom->input_layout);
	gfx_delete_rasterizer_state(g_wow->device, &bloom->rasterizer_state);
	gfx_delete_depth_stencil_state(g_wow->device, &bloom->depth_stencil_state);
	gfx_delete_blend_state(g_wow->device, &bloom->blend_state);
	gfx_delete_texture(g_wow->device, &bloom->texture1);
	gfx_delete_texture(g_wow->device, &bloom->texture2);
	render_pass_vtable.dtr(render_pass);
}

static void resize(struct render_pass *render_pass, uint32_t width, uint32_t height)
{
	render_pass_vtable.resize(render_pass, width, height);
	struct bloom_render_pass *bloom = (struct bloom_render_pass*)render_pass;
	gfx_delete_texture(g_wow->device, &bloom->texture1);
	width *= scale_factor;
	height *= scale_factor;
	if (width < 1)
		width = 1;
	if (height < 1)
		height = 1;
	gfx_create_texture(g_wow->device, &bloom->texture1, GFX_TEXTURE_2D, GFX_R8G8B8A8, 1, width, height, 0);
	gfx_set_texture_filtering(&bloom->texture1, GFX_FILTERING_LINEAR, GFX_FILTERING_LINEAR, GFX_FILTERING_NONE);
	gfx_set_texture_addressing(&bloom->texture1, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP);
	gfx_delete_texture(g_wow->device, &bloom->texture2);
	gfx_create_texture(g_wow->device, &bloom->texture2, GFX_TEXTURE_2D, GFX_R8G8B8A8, 1, width, height, 0);
	gfx_set_texture_filtering(&bloom->texture2, GFX_FILTERING_LINEAR, GFX_FILTERING_LINEAR, GFX_FILTERING_NONE);
	gfx_set_texture_addressing(&bloom->texture2, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP);
	gfx_set_render_target_texture(&bloom->bloom_render_target, GFX_RENDERTARGET_ATTACHMENT_COLOR0, &bloom->texture1);
	gfx_set_render_target_texture(&bloom->vblur_render_target, GFX_RENDERTARGET_ATTACHMENT_COLOR0, &bloom->texture2);
	gfx_set_render_target_texture(&bloom->hblur_render_target, GFX_RENDERTARGET_ATTACHMENT_COLOR0, &bloom->texture1);
}

static void render_bloom(struct render_pass *render_pass, struct render_target *src)
{
	struct bloom_render_pass *bloom = (struct bloom_render_pass*)render_pass;
	gfx_bind_render_target(g_wow->device, &bloom->bloom_render_target);
	gfx_bind_pipeline_state(g_wow->device, &bloom->bloom_pipeline_state);
	gfx_set_viewport(g_wow->device, 0, 0, render_pass->width * scale_factor, render_pass->height * scale_factor);
	gfx_clear_color(g_wow->device, &bloom->bloom_render_target, GFX_RENDERTARGET_ATTACHMENT_COLOR0, (struct vec4f){0, 0, 0, 1});
	{
		struct shader_bloom_model_block model_block;
		MAT4_ORTHO(float, model_block.mvp, 0, 1, 0, 1, -2, 2);
		model_block.threshold = threshold;
		gfx_set_buffer_data(&bloom->bloom_uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
		gfx_bind_constant(g_wow->device, 1, &bloom->bloom_uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	}
	gfx_bind_attributes_state(g_wow->device, &bloom->attributes_state, &bloom->input_layout);
	const gfx_texture_t *textures[] =
	{
		&src->color_texture,
	};
	gfx_bind_samplers(g_wow->device, 0, 1, textures);
	gfx_draw_indexed(g_wow->device, 6, 0);
}

static void render_vblur(struct render_pass *render_pass)
{
	struct bloom_render_pass *bloom = (struct bloom_render_pass*)render_pass;
	gfx_bind_render_target(g_wow->device, &bloom->vblur_render_target);
	gfx_bind_pipeline_state(g_wow->device, &bloom->blur_pipeline_state);
	gfx_set_viewport(g_wow->device, 0, 0, render_pass->width * scale_factor, render_pass->height * scale_factor);
	gfx_clear_color(g_wow->device, &bloom->vblur_render_target, GFX_RENDERTARGET_ATTACHMENT_COLOR0, (struct vec4f){0, 0, 0, 1});
	{
		struct shader_bloom_blur_model_block model_block;
		MAT4_ORTHO(float, model_block.mvp, 0, 1, 0, 1, -2, 2);
		VEC2_SET(model_block.screen_size, g_wow->render_width, g_wow->render_height);
		model_block.horizontal = 0;
		gfx_set_buffer_data(&bloom->vblur_uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
		gfx_bind_constant(g_wow->device, 1, &bloom->vblur_uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	}
	gfx_bind_attributes_state(g_wow->device, &bloom->attributes_state, &bloom->input_layout);
	const gfx_texture_t *textures[] =
	{
		&bloom->texture1,
	};
	gfx_bind_samplers(g_wow->device, 0, 1, textures);
	gfx_draw_indexed(g_wow->device, 6, 0);
}

static void render_hblur(struct render_pass *render_pass)
{
	struct bloom_render_pass *bloom = (struct bloom_render_pass*)render_pass;
	gfx_bind_render_target(g_wow->device, &bloom->hblur_render_target);
	gfx_bind_pipeline_state(g_wow->device, &bloom->blur_pipeline_state);
	gfx_set_viewport(g_wow->device, 0, 0, render_pass->width * scale_factor, render_pass->height * scale_factor);
	gfx_clear_color(g_wow->device, &bloom->hblur_render_target, GFX_RENDERTARGET_ATTACHMENT_COLOR0, (struct vec4f){0, 0, 0, 1});
	{
		struct shader_bloom_blur_model_block model_block;
		MAT4_ORTHO(float, model_block.mvp, 0, 1, 0, 1, -2, 2);
		VEC2_SET(model_block.screen_size, g_wow->render_width, g_wow->render_height);
		model_block.horizontal = 1;
		gfx_set_buffer_data(&bloom->hblur_uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
		gfx_bind_constant(g_wow->device, 1, &bloom->hblur_uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	}
	gfx_bind_attributes_state(g_wow->device, &bloom->attributes_state, &bloom->input_layout);
	const gfx_texture_t *textures[] =
	{
		&bloom->texture2,
	};
	gfx_bind_samplers(g_wow->device, 0, 1, textures);
	gfx_draw_indexed(g_wow->device, 6, 0);
}

static void merge_bloom(struct render_pass *render_pass, struct render_target *src, struct render_target *dst, uint32_t buffers)
{
	struct bloom_render_pass *bloom = (struct bloom_render_pass*)render_pass;
	gfx_render_target_t *render_target;
	if (dst)
	{
		render_target = &dst->render_target;
		render_target_bind(dst, buffers);
	}
	else
	{
		render_target= NULL;
		gfx_bind_render_target(g_wow->device, NULL);
	}
	gfx_bind_pipeline_state(g_wow->device, &bloom->merge_pipeline_state);
	gfx_set_viewport(g_wow->device, 0, 0, dst ? dst->width : g_wow->render_width, dst ? dst->height : g_wow->render_height);
	gfx_clear_color(g_wow->device, render_target, GFX_RENDERTARGET_ATTACHMENT_COLOR0, (struct vec4f){0, 0, 0, 1});
	{
		struct shader_bloom_merge_model_block model_block;
		MAT4_ORTHO(float, model_block.mvp, 0, 1, 0, 1, -2, 2);
		model_block.factor = factor;
		gfx_set_buffer_data(&bloom->merge_uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
		gfx_bind_constant(g_wow->device, 1, &bloom->merge_uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	}
	gfx_bind_attributes_state(g_wow->device, &bloom->attributes_state, &bloom->input_layout);
	const gfx_texture_t *textures[] =
	{
		&src->color_texture,
		&src->normal_texture,
		&src->position_texture,
		&bloom->texture1,
	};
	gfx_bind_samplers(g_wow->device, 0, 4, textures);
	gfx_draw_indexed(g_wow->device, 6, 0);
}

static void process(struct render_pass *render_pass, struct render_target *src, struct render_target *dst, uint32_t buffers)
{
	render_pass_vtable.process(render_pass, src, dst, buffers);
	render_bloom(render_pass, src);
	render_vblur(render_pass);
	render_hblur(render_pass);
	merge_bloom(render_pass, src, dst, buffers);
}

static const struct render_pass_vtable bloom_render_pass_vtable =
{
	.ctr     = ctr,
	.dtr     = dtr,
	.process = process,
	.resize  = resize,
};

struct render_pass *bloom_render_pass_new(void)
{
	struct render_pass *render_pass = mem_malloc(MEM_PPE, sizeof(struct bloom_render_pass));
	if (!render_pass)
		return NULL;
	render_pass->vtable = &bloom_render_pass_vtable;
	render_pass->vtable->ctr(render_pass);
	return render_pass;
}
