#include "render_target.h"

#include "memory.h"
#include "log.h"
#include "wow.h"

#include <gfx/device.h>

struct render_target *render_target_new(uint8_t samples)
{
	struct render_target *render_target = mem_malloc(MEM_PPE, sizeof(*render_target));
	if (!render_target)
		return NULL;
	render_target->height = 0;
	render_target->width = 0;
	render_target->samples = samples;
	render_target->dirty_size = false;
	render_target->enabled = false;
	render_target->render_target = GFX_RENDER_TARGET_INIT();
	render_target->depth_stencil_texture = GFX_TEXTURE_INIT();
	render_target->position_texture = GFX_TEXTURE_INIT();
	render_target->normal_texture = GFX_TEXTURE_INIT();
	render_target->color_texture = GFX_TEXTURE_INIT();
	render_target_resize(render_target, 1, 1);
	return render_target;
}

void render_target_delete(struct render_target *render_target)
{
	if (!render_target)
		return;
	gfx_delete_render_target(g_wow->device, &render_target->render_target);
	gfx_delete_texture(g_wow->device, &render_target->depth_stencil_texture);
	gfx_delete_texture(g_wow->device, &render_target->position_texture);
	gfx_delete_texture(g_wow->device, &render_target->normal_texture);
	gfx_delete_texture(g_wow->device, &render_target->color_texture);
	mem_free(MEM_PPE, render_target);
}

void render_target_resize(struct render_target *render_target, uint32_t width, uint32_t height)
{
	uint32_t lod = render_target->samples;
	enum gfx_texture_type type;
	if (lod != 0)
	{
		type = GFX_TEXTURE_2D_MS;
	}
	else
	{
		type = GFX_TEXTURE_2D;
		lod = 1;
	}
	render_target->width = width;
	render_target->height = height;
	gfx_delete_render_target(g_wow->device, &render_target->render_target);
	gfx_create_render_target(g_wow->device, &render_target->render_target);
	/* depth stencil */
	gfx_delete_texture(g_wow->device, &render_target->depth_stencil_texture);
	gfx_create_texture(g_wow->device, &render_target->depth_stencil_texture, type, GFX_DEPTH24_STENCIL8, lod, width, height, 0);
	if (type == GFX_TEXTURE_2D)
	{
		gfx_set_texture_filtering(&render_target->depth_stencil_texture, GFX_FILTERING_LINEAR, GFX_FILTERING_LINEAR, GFX_FILTERING_NONE);
		gfx_set_texture_addressing(&render_target->depth_stencil_texture, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP);
	}
	gfx_set_render_target_texture(&render_target->render_target, GFX_RENDERTARGET_ATTACHMENT_DEPTH_STENCIL, &render_target->depth_stencil_texture);
	/* position */
	gfx_delete_texture(g_wow->device, &render_target->position_texture);
	gfx_create_texture(g_wow->device, &render_target->position_texture, type, GFX_RGBA16F, lod, width, height, 0);
	if (type == GFX_TEXTURE_2D)
	{
		gfx_set_texture_filtering(&render_target->position_texture, GFX_FILTERING_LINEAR, GFX_FILTERING_LINEAR, GFX_FILTERING_NONE);
		gfx_set_texture_addressing(&render_target->position_texture, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP);
	}
	gfx_set_render_target_texture(&render_target->render_target, GFX_RENDERTARGET_ATTACHMENT_COLOR2, &render_target->position_texture);
	/* normal */
	gfx_delete_texture(g_wow->device, &render_target->normal_texture);
	gfx_create_texture(g_wow->device, &render_target->normal_texture, type, GFX_RGBA16F, lod, width, height, 0);
	if (type == GFX_TEXTURE_2D)
	{
		gfx_set_texture_filtering(&render_target->normal_texture, GFX_FILTERING_LINEAR, GFX_FILTERING_LINEAR, GFX_FILTERING_NONE);
		gfx_set_texture_addressing(&render_target->normal_texture, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP);
	}
	gfx_set_render_target_texture(&render_target->render_target, GFX_RENDERTARGET_ATTACHMENT_COLOR1, &render_target->normal_texture);
	/* color */
	gfx_delete_texture(g_wow->device, &render_target->color_texture);
	gfx_create_texture(g_wow->device, &render_target->color_texture, type, GFX_B8G8R8A8, lod, width, height, 0);
	if (type == GFX_TEXTURE_2D)
	{
		gfx_set_texture_filtering(&render_target->color_texture, GFX_FILTERING_LINEAR, GFX_FILTERING_LINEAR, GFX_FILTERING_NONE);
		gfx_set_texture_addressing(&render_target->color_texture, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP);
	}
	gfx_set_render_target_texture(&render_target->render_target, GFX_RENDERTARGET_ATTACHMENT_COLOR0, &render_target->color_texture);
}

void render_target_bind(struct render_target *render_target, uint32_t buffers)
{
	if (render_target->dirty_size)
	{
		render_target_resize(render_target, g_wow->render_width * g_wow->fsaa, g_wow->render_height * g_wow->fsaa);
		render_target->dirty_size = false;
	}
	gfx_bind_render_target(g_wow->device, &render_target->render_target);
	{
		uint32_t draw_buffers[3];
		draw_buffers[0] = (buffers & RENDER_TARGET_COLOR_BUFFER_BIT) ? GFX_RENDERTARGET_ATTACHMENT_COLOR0 : GFX_RENDERTARGET_ATTACHMENT_NONE;
		draw_buffers[1] = (buffers & RENDER_TARGET_NORMAL_BUFFER_BIT) ? GFX_RENDERTARGET_ATTACHMENT_COLOR1 : GFX_RENDERTARGET_ATTACHMENT_NONE;
		draw_buffers[2] = (buffers & RENDER_TARGET_POSITION_BUFFER_BIT) ? GFX_RENDERTARGET_ATTACHMENT_COLOR2 : GFX_RENDERTARGET_ATTACHMENT_NONE;
		gfx_set_render_target_draw_buffers(&render_target->render_target, draw_buffers, 3);
	}
	gfx_set_viewport(g_wow->device, 0, 0, render_target->width, render_target->height);
}

void render_target_clear(struct render_target *render_target, uint32_t buffers)
{
	if (buffers & RENDER_TARGET_COLOR_BUFFER_BIT)
		gfx_clear_color(g_wow->device, &render_target->render_target, GFX_RENDERTARGET_ATTACHMENT_COLOR0, (struct vec4f){0, 0, 0, 1});
	if (buffers & RENDER_TARGET_NORMAL_BUFFER_BIT)
		gfx_clear_color(g_wow->device, &render_target->render_target, GFX_RENDERTARGET_ATTACHMENT_COLOR1, (struct vec4f){0, 0, 0, 1});
	if (buffers & RENDER_TARGET_POSITION_BUFFER_BIT)
		gfx_clear_color(g_wow->device, &render_target->render_target, GFX_RENDERTARGET_ATTACHMENT_COLOR2, (struct vec4f){0, 0, 0, 1});
	gfx_clear_depth_stencil(g_wow->device, &render_target->render_target, 1, 0);
}

void render_target_resolve(struct render_target *src, struct render_target *dst, uint32_t buffers)
{
	if (dst)
	{
		bool depth_stencil = false;
		if (dst->dirty_size)
		{
			render_target_resize(dst, g_wow->render_width * g_wow->fsaa, g_wow->render_height * g_wow->fsaa);
			dst->dirty_size = false;
		}
		for (size_t i = 0; i < 3; ++i)
		{
			if (!(buffers & (1 << (2 + i)))) /* offset of depth + stencil */
				continue;
			uint32_t buffers_bits = GFX_BUFFER_COLOR_BIT;
			if (!depth_stencil)
			{
				depth_stencil = true;
				if (buffers & RENDER_TARGET_DEPTH_BUFFER_BIT)
					buffers_bits |= GFX_BUFFER_DEPTH_BIT;
				if (buffers & RENDER_TARGET_STENCIL_BUFFER_BIT)
					buffers_bits |= GFX_BUFFER_STENCIL_BIT;
			}
			gfx_resolve_render_target(&src->render_target, &dst->render_target, buffers_bits, i, i);
		}
		if (!depth_stencil)
		{
			uint32_t buffers_bits = 0;
			if (buffers & RENDER_TARGET_DEPTH_BUFFER_BIT)
				buffers_bits |= GFX_BUFFER_DEPTH_BIT;
			if (buffers & RENDER_TARGET_STENCIL_BUFFER_BIT)
				buffers_bits |= GFX_BUFFER_STENCIL_BIT;
			if (buffers_bits)
				gfx_resolve_render_target(&src->render_target, &dst->render_target, buffers_bits, 0, 0);
		}
	}
	else
	{
		uint32_t buffers_bits = 0;
		if (buffers & RENDER_TARGET_DEPTH_BUFFER_BIT)
			buffers_bits |= GFX_BUFFER_DEPTH_BIT;
		if (buffers & RENDER_TARGET_STENCIL_BUFFER_BIT)
			buffers_bits |= GFX_BUFFER_STENCIL_BIT;
		if (buffers & RENDER_TARGET_COLOR_BUFFER_BIT)
			buffers_bits |= GFX_BUFFER_COLOR_BIT;
		if (buffers_bits)
			gfx_resolve_render_target(&src->render_target, NULL, buffers_bits, 0, 0);
	}
}

void render_target_set_enabled(struct render_target *render_target, bool enabled)
{
	render_target->enabled = enabled;
	if (!render_target->enabled)
		render_target_resize(render_target, 1, 1);
	else
		render_target->dirty_size = true;
}
