#include "gx/m2_ribbons.h"
#include "gx/skybox.h"
#include "gx/frame.h"
#include "gx/m2.h"

#include "map/map.h"

#include "graphics.h"
#include "shaders.h"
#include "memory.h"
#include "cache.h"
#include "log.h"
#include "wow.h"
#include "blp.h"

#include <jks/mat4.h>
#include <jks/vec3.h>

#include <gfx/device.h>

#include <stdio.h>

MEMORY_DECL(GX);

static void destroy_emitter(void *ptr)
{
	struct gx_m2_ribbons_emitter *emitter = ptr;
	if (emitter->texture)
		cache_unref_by_ref_blp(g_wow->cache, emitter->texture);
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
		for (size_t j = 0; j < RENDER_FRAMES_COUNT; ++j)
		{
			emitter->attributes_states[j] = GFX_ATTRIBUTES_STATE_INIT();
			emitter->vertexes_buffers[j] = GFX_BUFFER_INIT();
			emitter->uniform_buffers[j] = GFX_BUFFER_INIT();
		}
		struct wow_m2_ribbon *ribbon = &parent->parent->ribbons[i];
		struct wow_m2_texture *texture = &parent->parent->textures[ribbon->texture_indices[0]];
		if (!texture->type)
		{
			char path[256];
			snprintf(path, sizeof(path), "%s", texture->filename);
			normalize_blp_filename(path, sizeof(path));
			if (!cache_ref_by_key_blp(g_wow->cache, path, &emitter->texture))
			{
				LOG_ERROR("failed to get texture %s", path);
				emitter->texture = NULL;
			}
			blp_texture_ask_load(emitter->texture);
		}
		else
		{
			LOG_ERROR("unsupported texture type for ribbon: %d", (int)texture->type);
			emitter->texture = NULL;
		}
		emitter->emitter = ribbon;
		emitter->last_spawned = g_wow->frametime;
		jks_array_init(&emitter->vertexes, sizeof(struct shader_ribbon_input), NULL, &jks_array_memory_fn_GX);
		jks_array_init(&emitter->points, sizeof(struct gx_m2_ribbon_point), NULL, &jks_array_memory_fn_GX);
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

static void initialize(struct gx_m2_ribbons *ribbons)
{
	for (size_t i = 0; i < ribbons->emitters.size; ++i)
	{
		struct gx_m2_ribbons_emitter *emitter = JKS_ARRAY_GET(&ribbons->emitters, i, struct gx_m2_ribbons_emitter);
		for (size_t j = 0; j < RENDER_FRAMES_COUNT; ++j)
		{
			gfx_attribute_bind_t binds[] =
			{
				{&emitter->vertexes_buffers[j], sizeof(struct shader_ribbon_input), offsetof(struct shader_ribbon_input, position)},
				{&emitter->vertexes_buffers[j], sizeof(struct shader_ribbon_input), offsetof(struct shader_ribbon_input, color)},
				{&emitter->vertexes_buffers[j], sizeof(struct shader_ribbon_input), offsetof(struct shader_ribbon_input, uv)},
			};
			gfx_create_buffer(g_wow->device, &emitter->vertexes_buffers[j], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_ribbon_input) * MAX_RIBBONS * 4, GFX_BUFFER_STREAM);
			gfx_create_buffer(g_wow->device, &emitter->uniform_buffers[j], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_ribbon_model_block), GFX_BUFFER_STREAM);
			gfx_create_attributes_state(g_wow->device, &emitter->attributes_states[j], binds, sizeof(binds) / sizeof(*binds), &g_wow->map->ribbons_indices_buffer, GFX_INDEX_UINT16);
		}
	}
	ribbons->initialized = true;
}

void gx_m2_ribbons_update(struct gx_m2_ribbons *ribbons)
{
}

static void render_emitter(struct gx_m2_ribbons_emitter *emitter)
{
	if (emitter->points.size < 2)
		return;
	gfx_set_buffer_data(&emitter->vertexes_buffers[g_wow->draw_frame_id], emitter->vertexes.data, sizeof(struct shader_ribbon_input) * emitter->vertexes.size, 0);
	gfx_bind_attributes_state(g_wow->device, &emitter->attributes_states[g_wow->draw_frame_id], &g_wow->graphics->ribbons_input_layout);
	gfx_bind_pipeline_state(g_wow->device, &((gfx_pipeline_state_t*)g_wow->graphics->ribbons_pipeline_states)[emitter->pipeline_state]);
	struct shader_ribbon_model_block model_block;
	model_block.alpha_test = emitter->alpha_test;
	model_block.mvp = g_wow->draw_frame->view_vp;
	model_block.mv = g_wow->draw_frame->view_v;
	if (emitter->fog_override)
		VEC3_CPY(model_block.fog_color, emitter->fog_color);
	else
		VEC3_CPY(model_block.fog_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_FOG]);
	gfx_set_buffer_data(&emitter->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
	gfx_bind_constant(g_wow->device, 1, &emitter->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	blp_texture_bind(emitter->texture, 0);
	gfx_draw_indexed(g_wow->device, (emitter->points.size - 1) * 6, 0);
}

void gx_m2_ribbons_render(struct gx_m2_ribbons *ribbons)
{
	if (!ribbons->initialized)
		initialize(ribbons);
	for (size_t i = 0; i < ribbons->emitters.size; ++i)
		render_emitter(JKS_ARRAY_GET(&ribbons->emitters, i, struct gx_m2_ribbons_emitter));
}
