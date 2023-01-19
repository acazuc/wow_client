#ifndef RENDER_TARGET_H
#define RENDER_TARGET_H

#include <gfx/objects.h>

#define RENDER_TARGET_DEPTH_BUFFER_BIT    0x01
#define RENDER_TARGET_STENCIL_BUFFER_BIT  0x02
#define RENDER_TARGET_COLOR_BUFFER_BIT    0x04
#define RENDER_TARGET_NORMAL_BUFFER_BIT   0x08
#define RENDER_TARGET_POSITION_BUFFER_BIT 0x10

struct render_target;

struct render_target
{
	gfx_render_target_t render_target;
	gfx_texture_t depth_stencil_texture;
	gfx_texture_t position_texture;
	gfx_texture_t normal_texture;
	gfx_texture_t color_texture;
	uint32_t height;
	uint32_t width;
	uint8_t samples;
	bool dirty_size;
	bool enabled;
};

struct render_target *render_target_new(uint8_t samples);
void render_target_delete(struct render_target *render_target);
void render_target_resize(struct render_target *render_target, uint32_t width, uint32_t height);
void render_target_bind(struct render_target *render_target, uint32_t buffers);
void render_target_clear(struct render_target *render_target, uint32_t buffers);
void render_target_resolve(struct render_target *src, struct render_target *dst, uint32_t buffers);
void render_target_set_enabled(struct render_target *render_target, bool enabled);

#endif
