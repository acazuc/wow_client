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
#include <math.h>

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
	ribbons->initialized = false;
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
		struct wow_m2_material *material = &parent->parent->materials[ribbon->material_indices[0]];
		enum world_rasterizer_state rasterizer_state = WORLD_RASTERIZER_UNCULLED;
		enum world_depth_stencil_state depth_stencil_state;
		enum world_blend_state blend_state;
		switch (material->blend_mode)
		{
			case 0: /* opaque */
				blend_state = WORLD_BLEND_OPAQUE;
				emitter->alpha_test = 0 / 255.;
				emitter->fog_override = false;
				break;
			case 1: /* alpha key */
				blend_state = WORLD_BLEND_OPAQUE;
				emitter->alpha_test = 224 / 255.;
				emitter->fog_override = false;
				break;
			case 2: /* alpha */
				blend_state = WORLD_BLEND_ALPHA;
				emitter->alpha_test = 1 / 255.;
				emitter->fog_override = false;
				break;
			case 3: /* no alpha add */
				blend_state = WORLD_BLEND_NO_ALPHA_ADD;
				emitter->alpha_test = 1 / 255.;
				emitter->fog_override = true;
				VEC3_SETV(emitter->fog_color, 0);
				break;
			case 4: /* add */
				blend_state = WORLD_BLEND_ADD;
				emitter->alpha_test = 1 / 255.;
				emitter->fog_override = true;
				VEC3_SETV(emitter->fog_color, 0);
				break;
			case 5: /* mod */
				blend_state = WORLD_BLEND_MOD;
				emitter->alpha_test = 1 / 255.;
				emitter->fog_override = true;
				VEC3_SETV(emitter->fog_color, 1);
				break;
			case 6: /* mod2x */
				blend_state = WORLD_BLEND_MOD2X;
				emitter->alpha_test = 1 / 255.;
				emitter->fog_override = true;
				VEC3_SETV(emitter->fog_color, .5);
				break;
			default:
				blend_state = WORLD_BLEND_ALPHA;
				emitter->alpha_test = 1 / 255.;
				emitter->fog_override = false;
				LOG_WARN("unsupported blend mode: %u", material->blend_mode);
				break;
		}
		switch ((!(material->flags & WOW_M2_MATERIAL_FLAGS_DEPTH_WRITE)) * 2 + (!(material->flags & WOW_M2_MATERIAL_FLAGS_DEPTH_TEST)) * 1)
		{
			case 0:
				depth_stencil_state = WORLD_DEPTH_STENCIL_NO_W;
				break;
			case 1:
				depth_stencil_state = WORLD_DEPTH_STENCIL_R_W;
				break;
			case 2:
				depth_stencil_state = WORLD_DEPTH_STENCIL_W_W;
				break;
			case 3:
				depth_stencil_state = WORLD_DEPTH_STENCIL_RW_W;
				break;
		}
		emitter->pipeline_state = &g_wow->graphics->ribbons_pipeline_states[rasterizer_state][depth_stencil_state][blend_state] - &g_wow->graphics->ribbons_pipeline_states[0][0][0];
		emitter->fog_override = false;
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

static void update_ribbons(struct gx_m2_ribbons *ribbons, struct gx_m2_ribbons_emitter *emitter)
{
	float dt = (g_wow->frametime - g_wow->lastframetime) / 1000000000.f;
	for (size_t i = 0; i < emitter->points.size; ++i)
	{
		struct gx_m2_ribbon_point *point = JKS_ARRAY_GET(&emitter->points, i, struct gx_m2_ribbon_point);
		float lifetime = (g_wow->frametime - point->created) / 1000000000.f;
		if (lifetime >= emitter->emitter->edge_lifetime)
		{
			jks_array_erase(&emitter->points, i);
			i--;
			continue;
		}
		point->position.y += dt * (emitter->emitter->gravity * (lifetime - dt * .5f));
	}
	if (!jks_array_resize(&emitter->vertexes, emitter->points.size * 2))
	{
		jks_array_resize(&emitter->points, emitter->points.size - 1);
		LOG_ERROR("failed to grow vertexes array");
		return;
	}
	for (size_t i = 0; i < emitter->points.size; ++i)
	{
		struct gx_m2_ribbon_point *point = JKS_ARRAY_GET(&emitter->points, i, struct gx_m2_ribbon_point);
		struct shader_ribbon_input *vertexes = JKS_ARRAY_GET(&emitter->vertexes, i * 2, struct shader_ribbon_input);
		float lifetime = ((g_wow->frametime - point->created) / 1000000000.f) / emitter->emitter->edge_lifetime;
		VEC3_MULV(vertexes[0].position, point->dir, point->size.y);
		VEC3_ADD(vertexes[0].position, point->position, vertexes[0].position);
		VEC3_MULV(vertexes[1].position, point->dir, point->size.x);
		VEC3_SUB(vertexes[1].position, point->position, vertexes[1].position);
		vertexes[0].position.w = 1;
		vertexes[1].position.w = 1;
		vertexes[0].color = point->color;
		vertexes[1].color = point->color;
		VEC2_SET(vertexes[0].uv, lifetime, 0);
		VEC2_SET(vertexes[1].uv, lifetime, 1);
	}
}

static void create_point(struct gx_m2_ribbons *ribbons, struct gx_m2_ribbons_emitter *emitter)
{
	struct gx_m2_ribbon_point *point = jks_array_grow(&emitter->points, 1);
	if (!point)
	{
		LOG_ERROR("failed to grow points array");
		return;
	}
	point->created = g_wow->frametime;
	struct vec4f position;
	VEC3_CPY(position, emitter->emitter->position);
	position.w = 1;
	if (emitter->emitter->bone_index != (uint16_t)-1 && emitter->emitter->bone_index < ribbons->parent->parent->bones_nb)
	{
		struct mat4f *bone_mat = JKS_ARRAY_GET(&ribbons->parent->render_frames[g_wow->cull_frame_id].bone_mats, emitter->emitter->bone_index, struct mat4f);
		struct vec4f tmp;
		MAT4_VEC4_MUL(tmp, *bone_mat, position);
		position = tmp;
	}
	MAT4_VEC4_MUL(point->position, ribbons->parent->m, position);
	VEC3_SET(point->dir, 0, 1, 0);
	int16_t alpha;
	m2_instance_get_track_value_int16(ribbons->parent, &emitter->emitter->alpha, &alpha);
	point->color.w = alpha / (float)0x7FFF;
	m2_instance_get_track_value_vec3f(ribbons->parent, &emitter->emitter->color, (struct vec3f*)&point->color.x);
	m2_instance_get_track_value_float(ribbons->parent, &emitter->emitter->height_above, &point->size.x);
	m2_instance_get_track_value_float(ribbons->parent, &emitter->emitter->height_below, &point->size.y);
}

void gx_m2_ribbons_update(struct gx_m2_ribbons *ribbons)
{
	for (size_t i = 0; i < ribbons->emitters.size; ++i)
	{
		struct gx_m2_ribbons_emitter *emitter = JKS_ARRAY_GET(&ribbons->emitters, i, struct gx_m2_ribbons_emitter);
		if (emitter->emitter->edges_per_second > 0)
		{
			uint64_t interval = 1000000000.f / emitter->emitter->edges_per_second;
			size_t j = 0;
			while (g_wow->frametime - emitter->last_spawned > interval)
			{
				if (j == 1 || emitter->points.size >= MAX_RIBBONS)
				{
					/* skip if too much particles must be created (avoiding freeze) */
					emitter->last_spawned = g_wow->frametime;
					break;
				}
				create_point(ribbons, emitter);
				emitter->last_spawned += interval;
				++j;
			}
		}
		update_ribbons(ribbons, emitter);
	}
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
	gfx_draw(g_wow->device, emitter->points.size * 2, 0);
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
			gfx_create_buffer(g_wow->device, &emitter->vertexes_buffers[j], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_ribbon_input) * MAX_RIBBONS * 2, GFX_BUFFER_STREAM);
			gfx_create_buffer(g_wow->device, &emitter->uniform_buffers[j], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_ribbon_model_block), GFX_BUFFER_STREAM);
			gfx_create_attributes_state(g_wow->device, &emitter->attributes_states[j], binds, sizeof(binds) / sizeof(*binds), NULL, 0);
		}
	}
	ribbons->initialized = true;
}

void gx_m2_ribbons_render(struct gx_m2_ribbons *ribbons)
{
	if (!ribbons->initialized)
		initialize(ribbons);
	for (size_t i = 0; i < ribbons->emitters.size; ++i)
		render_emitter(JKS_ARRAY_GET(&ribbons->emitters, i, struct gx_m2_ribbons_emitter));
}
