#include "render_pass.h"

#include "ppe/render_target.h"

#include "gx/skybox.h"

#include "map/map.h"

#include "shaders.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <gfx/device.h>

struct filter_render_pass
{
	struct render_pass render_pass;
	const gfx_shader_state_t *shader_state;
	gfx_depth_stencil_state_t depth_stencil_state;
	gfx_attributes_state_t attributes_state;
	gfx_rasterizer_state_t rasterizer_state;
	gfx_pipeline_state_t pipeline_state;
	gfx_input_layout_t input_layout;
	gfx_blend_state_t blend_state;
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t positions_buffer;
	gfx_buffer_t indices_buffer;
	gfx_buffer_t uv_buffer;
};

static const struct render_pass_vtable filter_render_pass_vtable;

static const struct gfx_input_layout_bind g_binds[] =
{
	{GFX_ATTR_R32G32_FLOAT, sizeof(struct vec2f), 0},
	{GFX_ATTR_R32G32_FLOAT, sizeof(struct vec2f), 0},
};

static void create_uniform_buffers(struct filter_render_pass *filter_render_pass, size_t size)
{
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_create_buffer(g_wow->device, &filter_render_pass->uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, size, GFX_BUFFER_STREAM);
}

static void ctr(struct render_pass *render_pass)
{
	render_pass_vtable.ctr(render_pass);
	struct filter_render_pass *filter = (struct filter_render_pass*)render_pass;
	filter->depth_stencil_state = GFX_DEPTH_STENCIL_STATE_INIT();
	filter->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	filter->rasterizer_state = GFX_RASTERIZER_STATE_INIT();
	filter->pipeline_state = GFX_PIPELINE_STATE_INIT();
	filter->input_layout = GFX_INPUT_LAYOUT_INIT();
	filter->blend_state = GFX_BLEND_STATE_INIT();
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		filter->uniform_buffers[i] = GFX_BUFFER_INIT();
	filter->positions_buffer = GFX_BUFFER_INIT();
	filter->indices_buffer = GFX_BUFFER_INIT();
	filter->uv_buffer = GFX_BUFFER_INIT();
	struct vec2f positions[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
	struct vec2f uvs[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
	uint16_t indices[6] = {0, 1, 3, 3, 1, 2};
	gfx_create_buffer(g_wow->device, &filter->positions_buffer, GFX_BUFFER_VERTEXES, positions, sizeof(positions), GFX_BUFFER_IMMUTABLE);
	gfx_create_buffer(g_wow->device, &filter->indices_buffer, GFX_BUFFER_INDICES, indices, sizeof(indices), GFX_BUFFER_IMMUTABLE);
	gfx_create_buffer(g_wow->device, &filter->uv_buffer, GFX_BUFFER_VERTEXES, uvs, sizeof(uvs), GFX_BUFFER_IMMUTABLE);
	gfx_create_input_layout(g_wow->device, &filter->input_layout, g_binds, sizeof(g_binds) / sizeof(*g_binds), filter->shader_state);
	const struct gfx_attribute_bind binds[] =
	{
		{&filter->positions_buffer},
		{&filter->uv_buffer},
	};
	gfx_create_attributes_state(g_wow->device, &filter->attributes_state, binds, sizeof(binds) / sizeof(*binds), &filter->indices_buffer, GFX_INDEX_UINT16);
	gfx_create_rasterizer_state(g_wow->device, &filter->rasterizer_state, GFX_FILL_SOLID, GFX_CULL_NONE, GFX_FRONT_CCW, false);
	gfx_create_depth_stencil_state(g_wow->device, &filter->depth_stencil_state, false, false, GFX_CMP_ALWAYS, false, 0, GFX_CMP_ALWAYS, 0, 0, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP);
	gfx_create_blend_state(g_wow->device, &filter->blend_state, true, GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE_MINUS_SRC_ALPHA, GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE_MINUS_SRC_ALPHA, GFX_EQUATION_ADD, GFX_EQUATION_ADD, GFX_COLOR_MASK_ALL);
	gfx_create_pipeline_state(g_wow->device,
		&filter->pipeline_state,
		filter->shader_state,
		&filter->rasterizer_state,
		&filter->depth_stencil_state,
		&filter->blend_state,
		&filter->input_layout,
		GFX_PRIMITIVE_TRIANGLES);
}

static void dtr(struct render_pass *render_pass)
{
	struct filter_render_pass *filter = (struct filter_render_pass*)render_pass;
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_delete_buffer(g_wow->device, &filter->uniform_buffers[i]);
	gfx_delete_buffer(g_wow->device, &filter->positions_buffer);
	gfx_delete_buffer(g_wow->device, &filter->indices_buffer);
	gfx_delete_buffer(g_wow->device, &filter->uv_buffer);
	gfx_delete_attributes_state(g_wow->device, &filter->attributes_state);
	gfx_delete_pipeline_state(g_wow->device, &filter->pipeline_state);
	gfx_delete_input_layout(g_wow->device, &filter->input_layout);
	gfx_delete_rasterizer_state(g_wow->device, &filter->rasterizer_state);
	gfx_delete_depth_stencil_state(g_wow->device, &filter->depth_stencil_state);
	gfx_delete_blend_state(g_wow->device, &filter->blend_state);
	render_pass_vtable.dtr(render_pass);
}

static void process(struct render_pass *render_pass, struct render_target *src, struct render_target *dst, uint32_t buffers)
{
	render_pass_vtable.process(render_pass, src, dst, buffers);
	struct filter_render_pass *filter = (struct filter_render_pass*)render_pass;
	gfx_render_target_t *render_target;
	uint32_t width;
	uint32_t height;
	if (dst)
	{
		render_target = &dst->render_target;
		render_target_bind(dst, buffers);
		width = dst->width;
		height = dst->height;
	}
	else
	{
		render_target = NULL;
		gfx_bind_render_target(g_wow->device, NULL);
		width = g_wow->render_width;
		height = g_wow->render_height;
	}
	gfx_bind_pipeline_state(g_wow->device, &filter->pipeline_state);
	gfx_set_viewport(g_wow->device, 0, 0, width, height);
	gfx_clear_color(g_wow->device, render_target, GFX_RENDERTARGET_ATTACHMENT_COLOR0, (struct vec4f){0, 0, 0, 1});
	gfx_bind_attributes_state(g_wow->device, &filter->attributes_state, &filter->input_layout);
	const gfx_texture_t *textures[] =
	{
		&src->color_texture,
		&src->normal_texture,
		&src->position_texture,
	};
	gfx_bind_samplers(g_wow->device, 0, 3, textures);
	gfx_draw_indexed(g_wow->device, 6, 0);
}

static void resize(struct render_pass *render_pass, uint32_t width, uint32_t height)
{
	render_pass_vtable.resize(render_pass, width, height);
}

static const struct render_pass_vtable filter_render_pass_vtable =
{
	.ctr     = ctr,
	.dtr     = dtr,
	.process = process,
	.resize  = resize,
};

static struct render_pass *filter_render_pass_new(const struct render_pass_vtable *vtable)
{
	struct render_pass *render_pass = mem_malloc(MEM_PPE, sizeof(struct filter_render_pass));
	if (!render_pass)
		return NULL;
	render_pass->vtable = vtable;
	render_pass->vtable->ctr(render_pass);
	return render_pass;
}

static void cel_ctr(struct render_pass *render_pass)
{
	struct filter_render_pass *filter_render_pass = (struct filter_render_pass*)render_pass;
	filter_render_pass->shader_state = &g_wow->shaders->cel;
	filter_render_pass_vtable.ctr(render_pass);
	create_uniform_buffers(filter_render_pass, sizeof(struct shader_cel_model_block));
}

static void cel_process(struct render_pass *render_pass, struct render_target *src, struct render_target *dst, uint32_t buffers)
{
	struct filter_render_pass *filter_render_pass = (struct filter_render_pass*)render_pass;
	struct shader_cel_model_block model_block;
	MAT4_ORTHO(float, model_block.mvp, 0, 1, 0, 1, -2, 2);
	model_block.cel = 1 / 5.;
	gfx_set_buffer_data(&filter_render_pass->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
	gfx_bind_constant(g_wow->device, 1, &filter_render_pass->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	filter_render_pass_vtable.process(render_pass, src, dst, buffers);
}

static const struct render_pass_vtable cel_render_pass_vtable =
{
	.ctr     = cel_ctr,
	.dtr     = dtr,
	.process = cel_process,
	.resize  = resize,
};

struct render_pass *cel_render_pass_new(void)
{
	return filter_render_pass_new(&cel_render_pass_vtable);
}

static void fxaa_ctr(struct render_pass *render_pass)
{
	struct filter_render_pass *filter_render_pass = (struct filter_render_pass*)render_pass;
	filter_render_pass->shader_state = &g_wow->shaders->fxaa;
	filter_render_pass_vtable.ctr(render_pass);
	create_uniform_buffers(filter_render_pass, sizeof(struct shader_fxaa_model_block));
}

static void fxaa_process(struct render_pass *render_pass, struct render_target *src, struct render_target *dst, uint32_t buffers)
{
	struct filter_render_pass *filter_render_pass = (struct filter_render_pass*)render_pass;
	struct shader_fxaa_model_block model_block;
	MAT4_ORTHO(float, model_block.mvp, 0, 1, 0, 1, -2, 2);
	VEC2_SET(model_block.screen_size, g_wow->render_width, g_wow->render_height);
	gfx_set_buffer_data(&filter_render_pass->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
	gfx_bind_constant(g_wow->device, 1, &filter_render_pass->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	filter_render_pass_vtable.process(render_pass, src, dst, buffers);
}

static const struct render_pass_vtable fxaa_render_pass_vtable =
{
	.ctr     = fxaa_ctr,
	.dtr     = dtr,
	.process = fxaa_process,
	.resize  = resize,
};

struct render_pass *fxaa_render_pass_new(void)
{
	return filter_render_pass_new(&fxaa_render_pass_vtable);
}

static void glow_ctr(struct render_pass *render_pass)
{
	struct filter_render_pass *filter_render_pass = (struct filter_render_pass*)render_pass;
	filter_render_pass->shader_state = &g_wow->shaders->glow;
	filter_render_pass_vtable.ctr(render_pass);
	create_uniform_buffers(filter_render_pass, sizeof(struct shader_glow_model_block));
}

static void glow_process(struct render_pass *render_pass, struct render_target *src, struct render_target *dst, uint32_t buffers)
{
	struct filter_render_pass *filter_render_pass = (struct filter_render_pass*)render_pass;
	struct shader_glow_model_block model_block;
	MAT4_ORTHO(float, model_block.mvp, 0, 1, 0, 1, -2, 2);
	model_block.factor = g_wow->map->gx_skybox->glow * .75;
	gfx_set_buffer_data(&filter_render_pass->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
	gfx_bind_constant(g_wow->device, 1, &filter_render_pass->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	filter_render_pass_vtable.process(render_pass, src, dst, buffers);
}

static const struct render_pass_vtable glow_render_pass_vtable =
{
	.ctr     = glow_ctr,
	.dtr     = dtr,
	.process = glow_process,
	.resize  = resize,
};

struct render_pass *glow_render_pass_new(void)
{
	return filter_render_pass_new(&glow_render_pass_vtable);
}

static void sharpen_ctr(struct render_pass *render_pass)
{
	struct filter_render_pass *filter_render_pass = (struct filter_render_pass*)render_pass;
	filter_render_pass->shader_state = &g_wow->shaders->sharpen;
	filter_render_pass_vtable.ctr(render_pass);
	create_uniform_buffers(filter_render_pass, sizeof(struct shader_sharpen_model_block));
}

static void sharpen_process(struct render_pass *render_pass, struct render_target *src, struct render_target *dst, uint32_t buffers)
{
	struct filter_render_pass *filter_render_pass = (struct filter_render_pass*)render_pass;
	struct shader_sharpen_model_block model_block;
	MAT4_ORTHO(float, model_block.mvp, 0, 1, 0, 1, -2, 2);
	model_block.power = .5;
	gfx_set_buffer_data(&filter_render_pass->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
	gfx_bind_constant(g_wow->device, 1, &filter_render_pass->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	filter_render_pass_vtable.process(render_pass, src, dst, buffers);
}

static const struct render_pass_vtable sharpen_render_pass_vtable =
{
	.ctr     = sharpen_ctr,
	.dtr     = dtr,
	.process = sharpen_process,
	.resize  = resize,
};

struct render_pass *sharpen_render_pass_new(void)
{
	return filter_render_pass_new(&sharpen_render_pass_vtable);
}

static void chromaber_ctr(struct render_pass *render_pass)
{
	struct filter_render_pass *filter_render_pass = (struct filter_render_pass*)render_pass;
	filter_render_pass->shader_state = &g_wow->shaders->chromaber;
	filter_render_pass_vtable.ctr(render_pass);
	create_uniform_buffers(filter_render_pass, sizeof(struct shader_chromaber_model_block));
}

static void chromaber_process(struct render_pass *render_pass, struct render_target *src, struct render_target *dst, uint32_t buffers)
{
	struct filter_render_pass *filter_render_pass = (struct filter_render_pass*)render_pass;
	struct shader_chromaber_model_block model_block;
	MAT4_ORTHO(float, model_block.mvp, 0, 1, 0, 1, -2, 2);
	VEC2_SET(model_block.screen_size, g_wow->render_width, g_wow->render_height);
	uint64_t interval = 60000000000. / 150;
	uint64_t t = g_wow->frametime % interval;
	model_block.size = 50 * (1 - (t / (float)interval));
	model_block.power = 1;
	gfx_set_buffer_data(&filter_render_pass->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
	gfx_bind_constant(g_wow->device, 1, &filter_render_pass->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	filter_render_pass_vtable.process(render_pass, src, dst, buffers);
}

static const struct render_pass_vtable chromaber_render_pass_vtable =
{
	.ctr     = chromaber_ctr,
	.dtr     = dtr,
	.process = chromaber_process,
	.resize  = resize,
};

struct render_pass *chromaber_render_pass_new(void)
{
	return filter_render_pass_new(&chromaber_render_pass_vtable);
}

static void sobel_ctr(struct render_pass *render_pass)
{
	struct filter_render_pass *filter_render_pass = (struct filter_render_pass*)render_pass;
	filter_render_pass->shader_state = &g_wow->shaders->sobel;
	filter_render_pass_vtable.ctr(render_pass);
	create_uniform_buffers(filter_render_pass, sizeof(struct shader_sobel_model_block));
}

static void sobel_process(struct render_pass *render_pass, struct render_target *src, struct render_target *dst, uint32_t buffers)
{
	struct filter_render_pass *filter_render_pass = (struct filter_render_pass*)render_pass;
	struct shader_sobel_model_block model_block;
	MAT4_ORTHO(float, model_block.mvp, 0, 1, 0, 1, -2, 2);
	gfx_set_buffer_data(&filter_render_pass->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
	gfx_bind_constant(g_wow->device, 1, &filter_render_pass->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	filter_render_pass_vtable.process(render_pass, src, dst, buffers);
}

static const struct render_pass_vtable sobel_render_pass_vtable =
{
	.ctr     = sobel_ctr,
	.dtr     = dtr,
	.process = sobel_process,
	.resize  = resize,
};

struct render_pass *sobel_render_pass_new(void)
{
	return filter_render_pass_new(&sobel_render_pass_vtable);
}

static void fsaa_ctr(struct render_pass *render_pass)
{
	struct filter_render_pass *filter_render_pass = (struct filter_render_pass*)render_pass;
	filter_render_pass->shader_state = &g_wow->shaders->fsaa;
	filter_render_pass_vtable.ctr(render_pass);
	create_uniform_buffers(filter_render_pass, sizeof(struct shader_fsaa_model_block));
}

static void fsaa_process(struct render_pass *render_pass, struct render_target *src, struct render_target *dst, uint32_t buffers)
{
	struct filter_render_pass *filter_render_pass = (struct filter_render_pass*)render_pass;
	struct shader_fsaa_model_block model_block;
	MAT4_ORTHO(float, model_block.mvp, 0, 1, 0, 1, -2, 2);
	gfx_set_buffer_data(&filter_render_pass->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
	gfx_bind_constant(g_wow->device, 1, &filter_render_pass->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	filter_render_pass_vtable.process(render_pass, src, dst, buffers);
}

static const struct render_pass_vtable fsaa_render_pass_vtable =
{
	.ctr     = fsaa_ctr,
	.dtr     = dtr,
	.process = fsaa_process,
	.resize  = resize,
};

struct render_pass *fsaa_render_pass_new(void)
{
	return filter_render_pass_new(&fsaa_render_pass_vtable);
}
