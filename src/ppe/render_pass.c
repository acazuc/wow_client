#include "render_pass.h"

#include "memory.h"
#include "wow.h"

static void ctr(struct render_pass *render_pass)
{
	render_pass->enabled = false;
	render_pass->width = 0;
	render_pass->height = 0;
	render_pass->dirty_size = false;
}

static void dtr(struct render_pass *render_pass)
{
	(void)render_pass;
}

static void process(struct render_pass *render_pass, struct render_target *src, struct render_target *dst, uint32_t buffers)
{
	(void)src;
	(void)dst;
	(void)buffers;
	if (render_pass->dirty_size)
	{
		render_pass->dirty_size = false;
		render_pass->vtable->resize(render_pass, g_wow->render_width * g_wow->fsaa, g_wow->render_height * g_wow->fsaa);
	}
}

static void resize(struct render_pass *render_pass, uint32_t width, uint32_t height)
{
	render_pass->width = width;
	render_pass->height = height;
}

const struct render_pass_vtable render_pass_vtable =
{
	.ctr     = ctr,
	.dtr     = dtr,
	.process = process,
	.resize  = resize,
};

void render_pass_process(struct render_pass *render_pass, struct render_target *src, struct render_target *dst, uint32_t buffers)
{
	render_pass->vtable->process(render_pass, src, dst, buffers);
}

void render_pass_resize(struct render_pass *render_pass, uint32_t width, uint32_t height)
{
	render_pass->vtable->resize(render_pass, width, height);
}

void render_pass_delete(struct render_pass *render_pass)
{
	if (!render_pass)
		return;
	render_pass->vtable->dtr(render_pass);
	mem_free(MEM_PPE, render_pass);
}
