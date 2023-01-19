#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include <gfx/objects.h>
#include <stdint.h>

struct render_target;
struct render_pass;

struct render_pass_vtable
{
	void (*ctr)(struct render_pass *render_pass);
	void (*dtr)(struct render_pass *render_pass);
	void (*process)(struct render_pass *render_pass, struct render_target *src, struct render_target *dst, uint32_t buffers);
	void (*resize)(struct render_pass *render_pass, uint32_t width, uint32_t height);
};

struct render_pass
{
	const struct render_pass_vtable *vtable;
	uint32_t width;
	uint32_t height;
	bool dirty_size;
	bool enabled;
};

extern const struct render_pass_vtable render_pass_vtable;

void render_pass_delete(struct render_pass *render_pass);
void render_pass_process(struct render_pass *render_pass, struct render_target *src, struct render_target *dst, uint32_t buffers);
void render_pass_resize(struct render_pass *render_pass, uint32_t width, uint32_t height);

struct render_pass *ssao_render_pass_new(void);
struct render_pass *bloom_render_pass_new(void);
struct render_pass *cel_render_pass_new(void);
struct render_pass *fxaa_render_pass_new(void);
struct render_pass *glow_render_pass_new(void);
struct render_pass *sharpen_render_pass_new(void);
struct render_pass *chromaber_render_pass_new(void);
struct render_pass *sobel_render_pass_new(void);
struct render_pass *fsaa_render_pass_new(void);

#endif
