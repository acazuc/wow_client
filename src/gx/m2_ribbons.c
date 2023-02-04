#include "gx/m2_ribbons.h"
#include "gx/m2.h"

#include "memory.h"
#include "log.h"
#include "wow.h"

#include <gfx/device.h>

MEMORY_DECL(GX);

static void destroy_emitter(void *ptr)
{
	struct gx_m2_ribbons_emitter *emitter = ptr;
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		gfx_delete_attributes_state(g_wow->device, &emitter->attributes_states[i]);
		gfx_delete_buffer(g_wow->device, &emitter->vertexes_buffers[i]);
		gfx_delete_buffer(g_wow->device, &emitter->uniform_buffers[i]);
	}
	jks_array_destroy(&emitter->vertexes);
	jks_array_destroy(&emitter->points);
}

struct gx_m2_ribbons *gx_m2_ribbons_new(struct gx_m2_instance *parent)
{
	struct gx_m2_ribbons *ribbons = mem_malloc(MEM_GX, sizeof(*ribbons));
	if (!ribbons)
	{
		LOG_ERROR("malloc failed");
		return NULL;
	}
	ribbons->parent = parent;
	jks_array_init(&ribbons->emitters, sizeof(struct gx_m2_ribbons_emitter), destroy_emitter, &jks_array_memory_fn_GX);
	if (!jks_array_reserve(&ribbons->emitters, parent->parent->ribbons_nb))
	{
		LOG_ERROR("failed tt reserve ribbon emitters");
		goto err;
	}
	for (size_t i = 0; i < parent->parent->ribbons_nb; ++i)
	{
		struct gx_m2_ribbons_emitter *emitter = jks_array_grow(&ribbons->emitters, 1);
		if (!emitter)
		{
			LOG_ERROR("failed to grow emitter array");
			goto err;
		}
		struct wow_m2_ribbon *ribbon = &parent->parent->ribbons[i];
		for (size_t j = 0; j < RENDER_FRAMES_COUNT; ++j)
		{
			emitter->attributes_states[j] = GFX_ATTRIBUTES_STATE_INIT();
			emitter->vertexes_buffers[j] = GFX_BUFFER_INIT();
			emitter->uniform_buffers[j] = GFX_BUFFER_INIT();
		}
	}
	return ribbons;

err:
	gx_m2_ribbons_delete(ribbons);
	return NULL;
}

void gx_m2_ribbons_delete(struct gx_m2_ribbons *ribbons)
{
	if (!ribbons)
		return;
	jks_array_destroy(&ribbons->emitters);
	mem_free(MEM_GX, ribbons);
}

void gx_m2_ribbons_update(struct gx_m2_ribbons *ribbons)
{
}

void gx_m2_ribbons_render(struct gx_m2_ribbons *ribbons)
{
}
