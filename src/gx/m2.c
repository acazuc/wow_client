#include "gx/m2_particles.h"
#include "gx/skybox.h"
#include "gx/m2.h"

#include "map/map.h"

#include "performance.h"
#include "graphics.h"
#include "shaders.h"
#include "camera.h"
#include "loader.h"
#include "memory.h"
#include "frame.h"
#include "cache.h"
#include "blp.h"
#include "log.h"
#include "wow.h"

#include <jks/quaternion.h>

#include <gfx/device.h>

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

#define MAX_BONES 148

#define GX_M2_VERTEX_DIFFUSE                   0x1
#define GX_M2_VERTEX_DIFFUSE_ENV_MAP           0x2
#define GX_M2_VERTEX_DIFFUSE_2TEX              0x3
#define GX_M2_VERTEX_DIFFUSE_ENV_MAP0_2TEX     0x4
#define GX_M2_VERTEX_DIFFUSE_ENV_MAP_2TEX      0x5
#define GX_M2_VERTEX_DIFFUSE_DUAL_ENV_MAP_2TEX 0x6

#define GX_M2_FRAGMENT_OPAQUE                 0x1
#define GX_M2_FRAGMENT_DIFFUSE                0x2
#define GX_M2_FRAGMENT_DECAL                  0x3
#define GX_M2_FRAGMENT_ADD                    0x4
#define GX_M2_FRAGMENT_DIFFUSE2X              0x5
#define GX_M2_FRAGMENT_FADE                   0x6
#define GX_M2_FRAGMENT_OPAQUE_OPAQUE          0x7
#define GX_M2_FRAGMENT_OPAQUE_MOD             0x8
#define GX_M2_FRAGMENT_OPAQUE_DECAL           0x9
#define GX_M2_FRAGMENT_OPAQUE_ADD             0xA
#define GX_M2_FRAGMENT_OPAQUE_MOD2X           0xB
#define GX_M2_FRAGMENT_OPAQUE_FADE            0xC
#define GX_M2_FRAGMENT_DIFFUSE_2TEX_OPAQUE    0xD
#define GX_M2_FRAGMENT_DIFFUSE_2TEX           0xE
#define GX_M2_FRAGMENT_DIFFUSE_2TEX_DECAL     0xF
#define GX_M2_FRAGMENT_DIFFUSE_2TEX_ADD       0x10
#define GX_M2_FRAGMENT_DIFFUSE_2TEX_2X        0x11
#define GX_M2_FRAGMENT_DIFFUSE_2TEX_FADE      0x12
#define GX_M2_FRAGMENT_DECAL_OPAQUE           0x13
#define GX_M2_FRAGMENT_DECAL_MOD              0x14
#define GX_M2_FRAGMENT_DECAL_DECAL            0x15
#define GX_M2_FRAGMENT_DECAL_ADD              0x16
#define GX_M2_FRAGMENT_DECAL_MOD2X            0x17
#define GX_M2_FRAGMENT_DECAL_FADE             0x18
#define GX_M2_FRAGMENT_ADD_OPAQUE             0x19
#define GX_M2_FRAGMENT_ADD_MOD                0x1A
#define GX_M2_FRAGMENT_ADD_DECAL              0x1B
#define GX_M2_FRAGMENT_ADD_ADD                0x1C
#define GX_M2_FRAGMENT_ADD_MOD2X              0x1D
#define GX_M2_FRAGMENT_ADD_FADE               0x1E
#define GX_M2_FRAGMENT_DIFFUSE_2TEX_OPAQUE_2X 0x1F
/* #define GX_M2_FRAGMENT_DIFFUSE_2TEX_2X      0x20 */
#define GX_M2_FRAGMENT_DIFFUSE_2TEX_DECAL_2X  0x21
#define GX_M2_FRAGMENT_DIFFUSE_2TEX_ADD_2X    0x22
#define GX_M2_FRAGMENT_DIFFUSE_2TEX_4X        0x23
#define GX_M2_FRAGMENT_DIFFUSE_2TEX_FADE_2X   0x24

MEMORY_DECL(GX);

/*
T interpolateHermite(T &v0, T &v1, T &v2, T &v3, float x)
{
	return v1 + (v1 - v0 + (v0 * 2.f - v1 * 5.f + v2 * 4.f - v3 + (-v0 + v1 * 3.f - v2 * 3.f + v3) * x) * x) * x;
}
*/

static void gx_m2_texture_init(struct gx_m2_texture *texture);
static void gx_m2_texture_destroy(struct gx_m2_texture *texture);
static void gx_m2_texture_load(struct gx_m2_texture *texture, struct wow_m2_file *file, struct wow_m2_batch *batch, uint16_t offset);
static const gfx_texture_t *gx_m2_texture_bind(struct gx_m2_texture *texture, struct gx_m2_instance *instance);

static bool gx_m2_batch_init(struct gx_m2_batch *batch, struct gx_m2_profile *parent, struct wow_m2_file *file, struct wow_m2_skin_profile *profile, struct wow_m2_batch *wow_batch, struct jks_array *indices);
static void gx_m2_batch_destroy(struct gx_m2_batch *batch);
static void gx_m2_batch_initialize(struct gx_m2_batch *batch);
static void gx_m2_batch_load(struct gx_m2_batch *batch, struct wow_m2_file *file, struct wow_m2_batch *wow_batch);
static bool gx_m2_batch_prepare_draw(struct gx_m2_batch *batch, struct gx_m2_render_params *params);
static void gx_m2_batch_render(struct gx_m2_batch *batch, struct gx_m2_instance *instance);

static bool gx_m2_profile_init(struct gx_m2_profile *profile, struct gx_m2 *parent, struct wow_m2_file *file, struct wow_m2_skin_profile *wow_profile, struct jks_array *indices);
static void gx_m2_profile_destroy(struct gx_m2_profile *profile);
static void gx_m2_profile_initialize(struct gx_m2_profile *profile);
static void gx_m2_profile_render(struct gx_m2_profile *profile, struct jks_array *instances, bool transparent);
static void gx_m2_profile_render_instance(struct gx_m2_profile *profile, struct gx_m2_instance *instance, bool transparent, struct gx_m2_render_params *params);

static void update_instance_uniform_buffer(struct gx_m2_instance *profile);

static bool get_track_values(struct gx_m2 *m2, struct wow_m2_track *track, void **v1, void **v2, size_t size, float *a, struct wow_m2_sequence *sequence, uint32_t t)
{
	if (track->timestamps_nb == 0)
		return false;
	if (track->timestamps_nb != track->values_nb)
	{
		LOG_WARN("timestamps_nb != values_nb");
		return false;
	}
	if (track->values_nb == 1)
	{
		*v1 = track->values;
		*v2 = track->values;
		*a = 0;
		return true;
	}
	if (track->global_sequence >= 0 && (uint16_t)track->global_sequence < m2->global_sequences_nb)
	{
		uint32_t max = m2->global_sequences[track->global_sequence];
		if (max == 0)
			t = 0;
		else
			t = (g_wow->frametime / 1000000) % max;
	}
	else if (sequence)
	{
		t = sequence->start + t % (sequence->end - sequence->start);
	}
	t %= track->timestamps[track->timestamps_nb - 1] + 1;
	for (uint32_t i = 0; i < track->timestamps_nb; ++i)
	{
		if (track->timestamps[i] < t)
			continue;
		if (i == 0)
		{
			*a = 0;
			*v1 = track->values;
			*v2 = track->values;
			return true;
		}
		if (track->timestamps[i] == track->timestamps[i - 1])
			*a = 1;
		else
			*a = (t - track->timestamps[i - 1]) / (float)(track->timestamps[i] - track->timestamps[i - 1]);
		*v1 = ((char*)track->values) + (i - 1) * size;
		*v2 = ((char*)track->values) +  i      * size;
		return true;
	}
	LOG_WARN("no track value found");
	return false;
}

bool m2_get_track_value_vec4f(struct gx_m2 *m2, struct wow_m2_track *track, struct vec4f *val, struct wow_m2_sequence *sequence, uint32_t t)
{
	struct vec4f *v1;
	struct vec4f *v2;
	float a;
	if (!get_track_values(m2, track, (void**)&v1, (void**)&v2, sizeof(*v1), &a, sequence, t))
		return false;
	switch (track->interpolation_type)
	{
		default:
		case 0:
			*val = *v1;
			break;
		case 1:
			VEC4_SUB(*val, *v2, *v1);
			VEC4_MULV(*val, *val, a);
			VEC4_ADD(*val, *val, *v1);
			break;
	}
	return true;
}

bool m2_get_track_value_vec3f(struct gx_m2 *m2, struct wow_m2_track *track, struct vec3f *val, struct wow_m2_sequence *sequence, uint32_t t)
{
	struct vec3f *v1;
	struct vec3f *v2;
	float a;
	if (!get_track_values(m2, track, (void**)&v1, (void**)&v2, sizeof(*v1), &a, sequence, t))
		return false;
	switch (track->interpolation_type)
	{
		default:
		case 0:
			*val = *v1;
			break;
		case 1:
			VEC3_SUB(*val, *v2, *v1);
			VEC3_MULV(*val, *val, a);
			VEC3_ADD(*val, *val, *v1);
			break;
	}
	return true;
}

bool m2_get_track_value_float(struct gx_m2 *m2, struct wow_m2_track *track, float *val, struct wow_m2_sequence *sequence, uint32_t t)
{
	float *v1;
	float *v2;
	float a;
	if (!get_track_values(m2, track, (void**)&v1, (void**)&v2, sizeof(*v1), &a, sequence, t))
		return false;
	switch (track->interpolation_type)
	{
		default:
		case 0:
			*val = *v1;
			break;
		case 1:
			*val = *v1 + (*v2 - *v1) * a;
			break;
	}
	return true;
}

bool m2_get_track_value_uint8(struct gx_m2 *m2, struct wow_m2_track *track, uint8_t *val, struct wow_m2_sequence *sequence, uint32_t t)
{
	uint8_t *v1;
	uint8_t *v2;
	float a;
	if (!get_track_values(m2, track, (void**)&v1, (void**)&v2, sizeof(*v1), &a, sequence, t))
		return false;
	switch (track->interpolation_type)
	{
		default:
		case 0:
			*val = *v1;
			break;
		case 1:
			*val = *v1 + (*v2 - *v1) * a;
			break;
	}
	return true;
}

bool m2_get_track_value_int16(struct gx_m2 *m2, struct wow_m2_track *track, int16_t *val, struct wow_m2_sequence *sequence, uint32_t t)
{
	int16_t *v1;
	int16_t *v2;
	float a;
	if (!get_track_values(m2, track, (void**)&v1, (void**)&v2, sizeof(*v1), &a, sequence, t))
		return false;
	switch (track->interpolation_type)
	{
		default:
		case 0:
			*val = *v1;
			break;
		case 1:
			*val = *v1 + (*v2 - *v1) * a;
			break;
	}
	return true;
}

bool m2_get_track_value_quat16(struct gx_m2 *m2, struct wow_m2_track *track, struct vec4f *val, struct wow_m2_sequence *sequence, uint32_t t)
{
	struct vec4s *v1;
	struct vec4s *v2;
	float a;
	if (!get_track_values(m2, track, (void**)&v1, (void**)&v2, sizeof(*v1), &a, sequence, t))
		return false;
	switch (track->interpolation_type)
	{
		default:
		case 0:
			val->x = (v1->x < 0 ? v1->x + 32768 : v1->x - 32767) / 32767.f;
			val->y = (v1->y < 0 ? v1->y + 32768 : v1->y - 32767) / 32767.f;
			val->z = (v1->z < 0 ? v1->z + 32768 : v1->z - 32767) / 32767.f;
			val->w = (v1->w < 0 ? v1->w + 32768 : v1->w - 32767) / 32767.f;
			break;
		case 1:
		{
			struct vec4f tmp1;
			tmp1.x = (v1->x < 0 ? v1->x + 32768 : v1->x - 32767) / 32767.f;
			tmp1.y = (v1->y < 0 ? v1->y + 32768 : v1->y - 32767) / 32767.f;
			tmp1.z = (v1->z < 0 ? v1->z + 32768 : v1->z - 32767) / 32767.f;
			tmp1.w = (v1->w < 0 ? v1->w + 32768 : v1->w - 32767) / 32767.f;
			struct vec4f tmp2;
			tmp2.x = (v2->x < 0 ? v2->x + 32768 : v2->x - 32767) / 32767.f;
			tmp2.y = (v2->y < 0 ? v2->y + 32768 : v2->y - 32767) / 32767.f;
			tmp2.z = (v2->z < 0 ? v2->z + 32768 : v2->z - 32767) / 32767.f;
			tmp2.w = (v2->w < 0 ? v2->w + 32768 : v2->w - 32767) / 32767.f;
			VEC4_SUB(*val, tmp2, tmp1);
			VEC4_MULV(*val, *val, a);
			VEC4_ADD(*val, *val, tmp1);
			break;
		}
	}
	return true;
}

bool m2_instance_get_track_value_vec4f(struct gx_m2_instance *instance, struct wow_m2_track *track, struct vec4f *val)
{
	struct vec4f v1;
	if (!m2_get_track_value_vec4f(instance->parent, track, &v1, instance->sequence, instance->sequence_time))
		return false;
	if (g_wow->frametime - instance->sequence_started >= (instance->sequence->blend_time * 1000000)
	 || !instance->prev_sequence || instance->prev_sequence->end <= instance->prev_sequence->start)
	{
		*val = v1;
		return true;
	}
	struct vec4f v2;
	if (!m2_get_track_value_vec4f(instance->parent, track, &v2, instance->prev_sequence, instance->prev_sequence_time))
	{
		*val = v1;
		return true;
	}
	float pct = (g_wow->frametime - instance->sequence_started) / (instance->sequence->blend_time * 1000000.);
	VEC4_MULV(v1, v1, pct);
	VEC4_MULV(v2, v2, 1 - pct);
	VEC4_ADD(*val, v1, v2);
	return true;
}

bool m2_instance_get_track_value_vec3f(struct gx_m2_instance *instance, struct wow_m2_track *track, struct vec3f *val)
{
	struct vec3f v1;
	if (!m2_get_track_value_vec3f(instance->parent, track, &v1, instance->sequence, instance->sequence_time))
		return false;
	if (g_wow->frametime - instance->sequence_started >= (instance->sequence->blend_time * 1000000)
	 || !instance->prev_sequence || instance->prev_sequence->end <= instance->prev_sequence->start)
	{
		*val = v1;
		return true;
	}
	struct vec3f v2;
	if (!m2_get_track_value_vec3f(instance->parent, track, &v2, instance->prev_sequence, instance->prev_sequence_time))
	{
		*val = v1;
		return true;
	}
	float pct = (g_wow->frametime - instance->sequence_started) / (instance->sequence->blend_time * 1000000.);
	VEC3_MULV(v1, v1, pct);
	VEC3_MULV(v2, v2, 1 - pct);
	VEC3_ADD(*val, v1, v2);
	return true;
}

bool m2_instance_get_track_value_float(struct gx_m2_instance *instance, struct wow_m2_track *track, float *val)
{
	float v1;
	if (!m2_get_track_value_float(instance->parent, track, &v1, instance->sequence, instance->sequence_time))
		return false;
	if (g_wow->frametime - instance->sequence_started >= (instance->sequence->blend_time * 1000000)
	 || !instance->prev_sequence || instance->prev_sequence->end <= instance->prev_sequence->start)
	{
		*val = v1;
		return true;
	}
	float v2;
	if (!m2_get_track_value_float(instance->parent, track, &v2, instance->prev_sequence, instance->prev_sequence_time))
	{
		*val = v1;
		return true;
	}
	float pct = (g_wow->frametime - instance->sequence_started) / (instance->sequence->blend_time * 1000000.);
	*val = v1 * pct + v2 * (1 - pct);
	return true;
}

bool m2_instance_get_track_value_uint8(struct gx_m2_instance *instance, struct wow_m2_track *track, uint8_t *val)
{
	uint8_t v1;
	if (!m2_get_track_value_uint8(instance->parent, track, &v1, instance->sequence, instance->sequence_time))
		return false;
	if (g_wow->frametime - instance->sequence_started >= (instance->sequence->blend_time * 1000000)
	 || !instance->prev_sequence || instance->prev_sequence->end <= instance->prev_sequence->start)
	{
		*val = v1;
		return true;
	}
	uint8_t v2;
	if (!m2_get_track_value_uint8(instance->parent, track, &v2, instance->prev_sequence, instance->prev_sequence_time))
	{
		*val = v1;
		return true;
	}
	float pct = (g_wow->frametime - instance->sequence_started) / (instance->sequence->blend_time * 1000000.);
	*val = v1 * pct + v2 * (1 - pct);
	return true;
}

bool m2_instance_get_track_value_int16(struct gx_m2_instance *instance, struct wow_m2_track *track, int16_t *val)
{
	int16_t v1;
	if (!m2_get_track_value_int16(instance->parent, track, &v1, instance->sequence, instance->sequence_time))
		return false;
	if (g_wow->frametime - instance->sequence_started >= (instance->sequence->blend_time * 1000000)
	 || !instance->prev_sequence || instance->prev_sequence->end <= instance->prev_sequence->start)
	{
		*val = v1;
		return true;
	}
	int16_t v2;
	if (!m2_get_track_value_int16(instance->parent, track, &v2, instance->prev_sequence, instance->prev_sequence_time))
	{
		*val = v1;
		return true;
	}
	float pct = (g_wow->frametime - instance->sequence_started) / (instance->sequence->blend_time * 1000000.);
	*val = v1 * pct + v2 * (1 - pct);
	return true;
}

bool m2_instance_get_track_value_quat16(struct gx_m2_instance *instance, struct wow_m2_track *track, struct vec4f *val)
{
	struct vec4f v1;
	if (!m2_get_track_value_quat16(instance->parent, track, &v1, instance->sequence, instance->sequence_time))
		return false;
	if (g_wow->frametime - instance->sequence_started >= (instance->sequence->blend_time * 1000000)
	 || !instance->prev_sequence || instance->prev_sequence->end <= instance->prev_sequence->start)
	{
		*val = v1;
		return true;
	}
	struct vec4f v2;
	if (!m2_get_track_value_quat16(instance->parent, track, &v2, instance->prev_sequence, instance->prev_sequence_time))
	{
		*val = v1;
		return true;
	}
	float pct = (g_wow->frametime - instance->sequence_started) / (instance->sequence->blend_time * 1000000.);
	VEC4_MULV(v1, v1, pct);
	VEC4_MULV(v2, v2, 1 - pct);
	VEC4_ADD(*val, v1, v2);
	return true;
}

static void gx_m2_texture_init(struct gx_m2_texture *texture)
{
	texture->texture = NULL;
	texture->initialized = false;
}

static void gx_m2_texture_destroy(struct gx_m2_texture *texture)
{
	if (texture->texture)
		cache_unref_by_ref_blp(g_wow->cache, texture->texture);
}

static void gx_m2_texture_load(struct gx_m2_texture *texture, struct wow_m2_file *file, struct wow_m2_batch *batch, uint16_t offset)
{
	texture->initialized = true;
	struct wow_m2_texture *wow_texture = &file->textures[file->texture_lookups[batch->texture_combo_index + offset]];
	texture->type = wow_texture->type;
	if (!wow_texture->type)
	{
		if (wow_texture->filename)
		{
			char filename[512];
			snprintf(filename, sizeof(filename), "%s", wow_texture->filename);
			normalize_blp_filename(filename, sizeof(filename));
			if (cache_ref_by_key_blp(g_wow->cache, filename, &texture->texture))
			{
				blp_texture_ask_load(texture->texture);
			}
			else
			{
				LOG_ERROR("failed to load texture %s", filename);
				texture->texture = NULL;
			}
		}
	}
	texture->flags = wow_texture->flags;
	if (batch->texture_transform_combo_index != 0xffff)
	{
		if (batch->texture_transform_combo_index + offset >= file->texture_transforms_lookups_nb)
		{
			LOG_WARN("invalid textureTransformComboIndex: %u / %u", batch->texture_transform_combo_index + offset, file->texture_transforms_lookups_nb);
			goto no_transform;
		}
		uint16_t lookup = file->texture_transforms_lookups[batch->texture_transform_combo_index + offset];
		if (lookup == 0xffff)
			goto no_transform;
		if (lookup >= file->texture_transforms_nb)
		{
			LOG_WARN("invalid textureTransformLookup: %u / %u", lookup, file->texture_transforms_nb);
			goto no_transform;
		}
		texture->transform = lookup;
		texture->has_transform = true;
	}
	else
	{
no_transform:
		texture->has_transform = false;
	}
}

const gfx_texture_t *gx_m2_texture_bind(struct gx_m2_texture *texture, struct gx_m2_instance *instance)
{
	if (!texture->initialized)
		return NULL;
	struct blp_texture *blp_texture;
	if (texture->texture)
	{
		blp_texture = texture->texture;
	}
	else
	{
		switch (texture->type)
		{
			case 1:
				blp_texture = instance->skin_texture;
				break;
			case 2:
				blp_texture = instance->object_texture;
				break;
			case 6:
				blp_texture = instance->hair_texture;
				break;
			case 8:
				blp_texture = instance->skin_extra_texture;
				break;
			case 11:
				blp_texture = instance->monster_textures[0];
				break;
			case 12:
				blp_texture = instance->monster_textures[1];
				break;
			case 13:
				blp_texture = instance->monster_textures[2];
				break;
			default:
				blp_texture = NULL;
				break;
		}
	}
	if (!blp_texture)
	{
		/* LOG_WARN("batch no type: %u", texture->type); */
		return NULL;
	}
	if (!blp_texture->initialized)
		return NULL;
	enum gfx_texture_addressing addressing_s = (texture->flags & WOW_M2_TEXTURE_FLAG_CLAMP_S) ? GFX_TEXTURE_ADDRESSING_REPEAT : GFX_TEXTURE_ADDRESSING_CLAMP;
	enum gfx_texture_addressing addressing_t = (texture->flags & WOW_M2_TEXTURE_FLAG_CLAMP_T) ? GFX_TEXTURE_ADDRESSING_REPEAT : GFX_TEXTURE_ADDRESSING_CLAMP;
	gfx_set_texture_addressing(&blp_texture->texture, addressing_s, addressing_t, GFX_TEXTURE_ADDRESSING_CLAMP);
	return &blp_texture->texture;
}

void gx_m2_texture_update_matrix(struct gx_m2_texture *texture, struct gx_m2_batch *batch, struct mat4f *mat_ref)
{
	if (!texture->initialized || (texture->type == 0 && !texture->texture))
		return;
	struct mat4f mat;
	{
		struct vec3f tmp = {.5, .5, 0};
		struct mat4f tmp_mat;
		MAT4_IDENTITY(tmp_mat);
		MAT4_TRANSLATE(mat, tmp_mat, tmp);
	}
	struct gx_m2 *m2 = batch->parent->parent;
	if (texture->has_transform)
	{
		{
			struct vec3f v;
			if (m2_get_track_value_vec3f(m2, &m2->texture_transforms[texture->transform].translation, &v, NULL, g_wow->frametime / 1000000))
			{
				struct mat4f tmp_mat;
				MAT4_TRANSLATE(tmp_mat, mat, v);
				mat = tmp_mat;
			}
			if (m2_get_track_value_vec3f(m2, &m2->texture_transforms[texture->transform].scaling, &v, NULL, g_wow->frametime / 1000000))
			{
				struct mat4f tmp_mat;
				MAT4_SCALE(tmp_mat, mat, v);
				mat = tmp_mat;
			}
		}
		{
			struct vec4f v;
			if (m2_get_track_value_vec4f(m2, &m2->texture_transforms[texture->transform].rotation, &v, NULL, g_wow->frametime / 1000000))
			{
				struct mat4f quat;
				QUATERNION_TO_MAT4(float, quat, v);
				struct mat4f tmp_mat;
				MAT4_MUL(tmp_mat, mat, quat);
				mat = tmp_mat;
			}
		}
	}
	*mat_ref = mat;
}

static bool gx_m2_batch_init(struct gx_m2_batch *batch, struct gx_m2_profile *parent, struct wow_m2_file *file, struct wow_m2_skin_profile *profile, struct wow_m2_batch *wow_batch, struct jks_array *indices)
{
	batch->parent = parent;
	gx_m2_texture_init(&batch->textures[0]);
	gx_m2_texture_init(&batch->textures[1]);
	struct wow_m2_skin_section *section = &profile->sections[wow_batch->skin_section_index];
	batch->skin_section_id = section->skin_section_id;
	batch->flags = wow_batch->flags;
	gx_m2_batch_load(batch, file, wow_batch);
	batch->indices_offset = indices->size;
	batch->indices_nb = section->index_count;
	uint16_t *data = jks_array_grow(indices, section->index_count);
	if (!data)
	{
		LOG_ERROR("failed to grow indices array");
		return false;
	}
	for (uint32_t i = 0; i < section->index_count; ++i)
		data[i] = profile->vertexes[profile->indices[section->index_start + i]];
	if (wow_batch->texture_count >= 1)
	{
		gx_m2_texture_load(&batch->textures[0], file, wow_batch, 0);
		if (wow_batch->texture_count >= 2)
			gx_m2_texture_load(&batch->textures[1], file, wow_batch, 1);
	}
	struct wow_m2_material *material = &file->materials[wow_batch->material_index];
	batch->material_flags = material->flags;
	enum world_rasterizer_state rasterizer_state;
	enum world_depth_stencil_state depth_stencil_state;
	enum world_blend_state blend_state;
	rasterizer_state = batch->material_flags & WOW_M2_MATERIAL_FLAGS_UNCULLED ? WORLD_RASTERIZER_UNCULLED : WORLD_RASTERIZER_CULLED;
	/* XXX remove this hack */
	if (0 && strstr(batch->parent->parent->filename, "COT_HOURGLASS"))
	{
		if (batch->id == 4)
			material->blend_mode = 4;
	}
	/* COT hourglass:
	 * batch 0 is solid circles
	 * batch 1 is reflection on circles
	 * batch 2 is ornaments
	 * batch 3 is static sand
	 * batch 4 is glass shininess (hourglassgleam.blp): strange thing is that it's a grey-keyed texture (used ofr example as reflection, but in an "add / mix" way), but doing add of this on global looks strange
	 * batch 5 is glass stars reflection (starsportals.blp)
	 * batch 6 is ornaments
	 * batch 7 is flowing sand
	 * batch 8 is flowing sand (more opaque ?)
	 */
	switch (material->blend_mode)
	{
		case 0: /* opaque */
			blend_state = WORLD_BLEND_OPAQUE;
			batch->blending = false;
			batch->alpha_test = 0 / 255.;
			batch->fog_override = false;
			break;
		case 1: /* alpha key */
			blend_state = WORLD_BLEND_OPAQUE;
			batch->blending = false;
			batch->alpha_test = 224 / 255.;
			batch->fog_override = false;
			break;
		case 2: /* alpha */
			blend_state = WORLD_BLEND_ALPHA;
			batch->blending = true;
			batch->alpha_test = 1 / 255.;
			batch->fog_override = false;
			break;
		case 3: /* no alpha add */
			blend_state = WORLD_BLEND_NO_ALPHA_ADD;
			batch->blending = true;
			batch->alpha_test = 1 / 255.;
			batch->fog_override = true;
			VEC3_SETV(batch->fog_color, 0);
			break;
		case 4: /* add */
			blend_state = WORLD_BLEND_ADD;
			batch->blending = true;
			batch->alpha_test = 1 / 255.;
			batch->fog_override = true;
			VEC3_SETV(batch->fog_color, 0);
			break;
		case 5: /* mod */
			blend_state = WORLD_BLEND_MOD;
			batch->blending = true;
			batch->alpha_test = 1 / 255.;
			batch->fog_override = true;
			VEC3_SETV(batch->fog_color, 1);
			batch->flags |= WOW_M2_MATERIAL_FLAGS_UNLIT;
			break;
		case 6: /* mod2x */
			blend_state = WORLD_BLEND_MOD2X;
			batch->blending = true;
			batch->alpha_test = 1 / 255.;
			batch->fog_override = true;
			VEC3_SETV(batch->fog_color, .5);
			batch->flags |= WOW_M2_MATERIAL_FLAGS_UNLIT;
			break;
		default:
			blend_state = WORLD_BLEND_ALPHA;
			batch->blending = true;
			batch->alpha_test = 1 / 255.;
			batch->fog_override = false;
			LOG_WARN("unsupported blend mode: %u", material->blend_mode);
			break;
	}
	switch ((!(batch->material_flags & WOW_M2_MATERIAL_FLAGS_DEPTH_WRITE)) * 2 + (!(batch->material_flags & WOW_M2_MATERIAL_FLAGS_DEPTH_TEST)) * 1)
	{
		case 0:
			if (batch->parent->parent->skybox)
				depth_stencil_state = WORLD_DEPTH_STENCIL_NO_R;
			else if (batch->blending)
				depth_stencil_state = WORLD_DEPTH_STENCIL_NO_NO;
			else
				depth_stencil_state = WORLD_DEPTH_STENCIL_NO_W;
			break;
		case 1:
			if (batch->parent->parent->skybox)
				depth_stencil_state = WORLD_DEPTH_STENCIL_R_R;
			else if (batch->blending)
				depth_stencil_state = WORLD_DEPTH_STENCIL_R_NO;
			else
				depth_stencil_state = WORLD_DEPTH_STENCIL_R_W;
			break;
		case 2:
			if (batch->parent->parent->skybox)
				depth_stencil_state = WORLD_DEPTH_STENCIL_W_R;
			else if (batch->blending)
				depth_stencil_state = WORLD_DEPTH_STENCIL_W_NO;
			else
				depth_stencil_state = WORLD_DEPTH_STENCIL_W_W;
			break;
		case 3:
			if (batch->parent->parent->skybox)
				depth_stencil_state = WORLD_DEPTH_STENCIL_RW_R;
			else if (batch->blending)
				depth_stencil_state = WORLD_DEPTH_STENCIL_RW_NO;
			else
				depth_stencil_state = WORLD_DEPTH_STENCIL_RW_W;
			break;
	}
	batch->pipeline_state = &g_wow->graphics->m2_pipeline_states[rasterizer_state][depth_stencil_state][blend_state] - &g_wow->graphics->m2_pipeline_states[0][0][0];
	if (wow_batch->color_index != 0xffff)
	{
		if (wow_batch->color_index >= file->colors_nb)
		{
			LOG_WARN("invalid color index: %u / %u", wow_batch->color_index, file->colors_nb);
			goto no_color_transform;
		}
		batch->color_transform = wow_batch->color_index;
		batch->has_color_transform = true;
	}
	else
	{
no_color_transform:
		batch->has_color_transform = false;
	}
	if (wow_batch->texture_weight_combo_index != 0xffff)
	{
		if (wow_batch->texture_weight_combo_index >= file->texture_weights_lookups_nb)
		{
			LOG_WARN("invalid texture weight combo index: %u / %u", wow_batch->texture_weight_combo_index, file->texture_weights_lookups_nb);
			goto no_texture_weight;
		}
		uint16_t lookup = file->texture_weights_lookups[wow_batch->texture_weight_combo_index];
		if (lookup == 0xffff)
			goto no_texture_weight;
		if (lookup >= file->texture_weights_nb)
		{
			LOG_WARN("invalid texture weight lookup: %u / %u", lookup, file->texture_weights_nb);
			goto no_texture_weight;
		}
		batch->texture_weight = lookup;
		batch->has_texture_weight = true;
	}
	else
	{
no_texture_weight:
		batch->has_texture_weight = false;
	}
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		batch->uniform_buffers[i] = GFX_BUFFER_INIT();
	return true;
}

static void gx_m2_batch_destroy(struct gx_m2_batch *batch)
{
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_delete_buffer(g_wow->device, &batch->uniform_buffers[i]);
	gx_m2_texture_destroy(&batch->textures[0]);
	gx_m2_texture_destroy(&batch->textures[1]);
}

static void gx_m2_batch_initialize(struct gx_m2_batch *batch)
{
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_create_buffer(g_wow->device, &batch->uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_m2_mesh_block), GFX_BUFFER_STREAM);
}

static bool get_combiners(uint16_t tex_nb, uint32_t *fragment_modes, bool tex1env, bool tex2env, uint16_t *combiners)
{
	if (tex_nb == 1)
	{
		combiners[0] = tex1env ? GX_M2_VERTEX_DIFFUSE_ENV_MAP : GX_M2_VERTEX_DIFFUSE;
		static const uint32_t modes[] = {GX_M2_FRAGMENT_OPAQUE, GX_M2_FRAGMENT_DIFFUSE, GX_M2_FRAGMENT_DECAL, GX_M2_FRAGMENT_ADD, GX_M2_FRAGMENT_DIFFUSE2X, GX_M2_FRAGMENT_FADE};
		if (fragment_modes[0] >= sizeof(modes) / sizeof(*modes))
			return false;
		combiners[1] = modes[fragment_modes[0]];
		return true;
	}
	if (tex1env)
		combiners[0] = tex2env ? GX_M2_VERTEX_DIFFUSE_DUAL_ENV_MAP_2TEX : GX_M2_VERTEX_DIFFUSE_ENV_MAP0_2TEX;
	else
		combiners[0] = tex2env ? GX_M2_VERTEX_DIFFUSE_ENV_MAP_2TEX : GX_M2_VERTEX_DIFFUSE_2TEX;
	static const uint32_t modes[] =
	{
		GX_M2_FRAGMENT_OPAQUE_OPAQUE         , GX_M2_FRAGMENT_OPAQUE_MOD     , GX_M2_FRAGMENT_OPAQUE_DECAL         , GX_M2_FRAGMENT_OPAQUE_ADD         , GX_M2_FRAGMENT_OPAQUE_MOD2X   , GX_M2_FRAGMENT_OPAQUE_FADE,
		GX_M2_FRAGMENT_DIFFUSE_2TEX_OPAQUE   , GX_M2_FRAGMENT_DIFFUSE_2TEX   , GX_M2_FRAGMENT_DIFFUSE_2TEX_DECAL   , GX_M2_FRAGMENT_DIFFUSE_2TEX_ADD   , GX_M2_FRAGMENT_DIFFUSE_2TEX_2X, GX_M2_FRAGMENT_DIFFUSE_2TEX_FADE,
		GX_M2_FRAGMENT_DECAL_OPAQUE          , GX_M2_FRAGMENT_DECAL_MOD      , GX_M2_FRAGMENT_DECAL_DECAL          , GX_M2_FRAGMENT_DECAL_ADD          , GX_M2_FRAGMENT_DECAL_MOD2X    , GX_M2_FRAGMENT_DECAL_FADE,
		GX_M2_FRAGMENT_ADD_OPAQUE            , GX_M2_FRAGMENT_ADD_MOD        , GX_M2_FRAGMENT_ADD_DECAL            , GX_M2_FRAGMENT_ADD_ADD            , GX_M2_FRAGMENT_ADD_MOD2X      , GX_M2_FRAGMENT_ADD_FADE,
		GX_M2_FRAGMENT_DIFFUSE_2TEX_OPAQUE_2X, GX_M2_FRAGMENT_DIFFUSE_2TEX_2X, GX_M2_FRAGMENT_DIFFUSE_2TEX_DECAL_2X, GX_M2_FRAGMENT_DIFFUSE_2TEX_ADD_2X, GX_M2_FRAGMENT_DIFFUSE_2TEX_4X, GX_M2_FRAGMENT_DIFFUSE_2TEX_FADE_2X,
	};
	if (fragment_modes[0] >= 5 || fragment_modes[1] >= 6)
		return false;
	combiners[1] = modes[fragment_modes[0] * 6 + fragment_modes[1]];
	return true;
}

static void gx_m2_batch_load(struct gx_m2_batch *batch, struct wow_m2_file *file, struct wow_m2_batch *wow_batch)
{
	uint32_t fragment_modes[2];
	uint32_t lookups[2];
	bool tex2env;
	bool tex1env;

	if (wow_batch->shader_id < 0)
	{
		switch (-wow_batch->shader_id)
		{
			default:
#if 0
				LOG_WARN("unknown shader combiner: %d", wow_batch->shader_id);
#endif
				/* FALLTHROUGH */
			case 3:
				fragment_modes[0] = 1; /* test for real values */
				break;
		}
#if 0
		LOG_WARN("using combiner combo %u for %u", fragment_modes[0], batch->id);
#endif
	}
	else
	{
		if (wow_batch->texture_count > 0)
		{
			if (file->header.flags & WOW_M2_HEADER_FLAG_USE_TEXTURE_COMBINER_COMBO)
			{
				for (uint16_t i = 0; i < wow_batch->texture_count; ++i)
				{
					assert((unsigned)wow_batch->shader_id + i < file->texture_combiner_combos_nb);
					fragment_modes[i] = file->texture_combiner_combos[wow_batch->shader_id + i];
#if 0
					LOG_WARN("using combiner combo %u for id %u tex %u", fragment_modes[i], batch->id, i);
#endif
				}
			}
			else
			{
				struct wow_m2_material *material = &file->materials[wow_batch->material_index];
				for (uint16_t i = 0; i < wow_batch->texture_count; ++i)
				{
					static const uint32_t static_fragments[] = {0, 1, 1, 1, 1, 5, 5};
					fragment_modes[i] = static_fragments[material->blend_mode];
				}
			}
		}
	}
	assert(wow_batch->texture_coord_combo_index < file->texture_unit_lookups_nb);
	lookups[0] = file->texture_unit_lookups[wow_batch->texture_coord_combo_index];
	if (wow_batch->texture_count > 1)
	{
		assert(wow_batch->texture_coord_combo_index + 1u < file->texture_unit_lookups_nb);
		lookups[1] = file->texture_unit_lookups[wow_batch->texture_coord_combo_index + 1];
	}
	if (wow_batch->texture_count > 1)
		tex2env = lookups[1] > 2;
	else
		tex2env = false;
	tex1env = lookups[0] > 2 ? 1 : 0;
	if (get_combiners(wow_batch->texture_count, fragment_modes, tex1env, tex2env, batch->combiners))
		return;
	if (wow_batch->texture_count > 1)
		tex2env = lookups[1] > 2;
	else
		tex2env = false;
	fragment_modes[0] = 1;
	fragment_modes[1] = 1;
	if (get_combiners(wow_batch->texture_count, fragment_modes, tex1env, tex2env, batch->combiners))
		return;
	LOG_WARN("undefined combiner :(");
	batch->combiners[0] = GX_M2_VERTEX_DIFFUSE;
	batch->combiners[1] = GX_M2_FRAGMENT_DIFFUSE;
}

static bool gx_m2_batch_prepare_draw(struct gx_m2_batch *batch, struct gx_m2_render_params *params)
{
	struct gx_m2 *m2 = batch->parent->parent;
	struct vec4f color = {1, 1, 1, 1};
	if (batch->has_color_transform)
	{
		{
			struct vec3f v;
			if (m2_get_track_value_vec3f(m2, &m2->colors[batch->color_transform].color, &v, NULL, g_wow->frametime / 1000000))
				VEC3_CPY(color, v);
		}
		{
			int16_t v;
			if (m2_get_track_value_int16(m2, &m2->colors[batch->color_transform].alpha, &v, NULL, g_wow->frametime / 1000000))
				color.w *= v / (float)0x7fff;
		}
	}
	if (batch->has_texture_weight)
	{
		int16_t v;
		if (m2_get_track_value_int16(m2, &m2->texture_weights[batch->texture_weight].weight, &v, NULL, g_wow->frametime / 1000000))
			color.w *= v / (float)0x7fff;
	}
	if (color.w == 0)
		return false;
	PERFORMANCE_BEGIN(M2_RENDER_DATA);
	struct shader_m2_mesh_block mesh_block;
	gx_m2_texture_update_matrix(&batch->textures[0], batch, &mesh_block.tex1_matrix);
	gx_m2_texture_update_matrix(&batch->textures[1], batch, &mesh_block.tex2_matrix);
	mesh_block.settings.x = (batch->material_flags & WOW_M2_MATERIAL_FLAGS_UNFOGGED) ? 1 : 0;
	mesh_block.settings.y = (batch->material_flags & WOW_M2_MATERIAL_FLAGS_UNLIT) ? 1 : 0;
	mesh_block.settings.z = m2->bones_nb;
	mesh_block.settings.w = 0;
	mesh_block.combiners.x = batch->combiners[0];
	mesh_block.combiners.y = batch->combiners[1];
	mesh_block.combiners.z = 0;
	mesh_block.combiners.w = 0;
	mesh_block.color = color;
	if (batch->fog_override)
	{
		mesh_block.fog_color = batch->fog_color;
	}
	else
	{
		if (params)
		{
			mesh_block.fog_color = params->fog_color;
		}
		else
		{
			VEC3_CPY(mesh_block.fog_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_FOG]);
		}
	}
	mesh_block.alpha_test = batch->alpha_test * color.w;
	gfx_set_buffer_data(&batch->uniform_buffers[g_wow->draw_frame_id], &mesh_block, sizeof(mesh_block), 0);
	PERFORMANCE_END(M2_RENDER_DATA);
	PERFORMANCE_BEGIN(M2_RENDER_BIND);
	gfx_bind_constant(g_wow->device, 0, &batch->uniform_buffers[g_wow->draw_frame_id], sizeof(struct shader_m2_mesh_block), 0);
	gfx_bind_pipeline_state(g_wow->device, &((gfx_pipeline_state_t*)g_wow->graphics->m2_pipeline_states)[batch->pipeline_state]);
	PERFORMANCE_END(M2_RENDER_BIND);
	return true;
}

static void gx_m2_batch_render(struct gx_m2_batch *batch, struct gx_m2_instance *instance)
{
	PERFORMANCE_BEGIN(M2_RENDER_BIND);
	const gfx_texture_t *textures[2];
	textures[0] = gx_m2_texture_bind(&batch->textures[0], instance);
	textures[1] = gx_m2_texture_bind(&batch->textures[1], instance);
	gfx_bind_samplers(g_wow->device, 0, 2, textures);
	gfx_bind_constant(g_wow->device, 1, &instance->uniform_buffers[g_wow->draw_frame_id], sizeof(struct shader_m2_model_block), 0); /* XXX really there ? */
	PERFORMANCE_END(M2_RENDER_BIND);
	PERFORMANCE_BEGIN(M2_RENDER_DRAW);
	gfx_draw_indexed(g_wow->device, batch->indices_nb, batch->indices_offset);
	PERFORMANCE_END(M2_RENDER_DRAW);
}

static bool gx_m2_profile_init(struct gx_m2_profile *profile, struct gx_m2 *parent, struct wow_m2_file *file, struct wow_m2_skin_profile *wow_profile, struct jks_array *indices)
{
	profile->parent = parent;
	profile->initialized = false;
	jks_array_init(&profile->batches, sizeof(struct gx_m2_batch), (jks_array_destructor_t)gx_m2_batch_destroy, &jks_array_memory_fn_GX);
	jks_array_init(&profile->transparent_batches, sizeof(uint8_t), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&profile->opaque_batches, sizeof(uint8_t), NULL, &jks_array_memory_fn_GX);
	if (!jks_array_reserve(&profile->batches, wow_profile->batches_nb))
		return false;
	for (uint32_t i = 0; i < wow_profile->batches_nb; ++i)
	{
		struct wow_m2_batch *batch = &wow_profile->batches[i];
		struct gx_m2_batch *batch_profile = jks_array_grow(&profile->batches, 1);
		if (!batch_profile)
			return false;
		batch_profile->id = i;
		if (!gx_m2_batch_init(batch_profile, profile, file, wow_profile, batch, indices))
			return false;
		uint8_t pos = i;
		if (batch_profile->blending)
		{
			if (!jks_array_push_back(&profile->transparent_batches, &pos))
				return false;
		}
		else
		{
			if (!jks_array_push_back(&profile->opaque_batches, &pos))
				return false;
		}
	}
	return true;
}

static void gx_m2_profile_destroy(struct gx_m2_profile *profile)
{
	jks_array_destroy(&profile->batches);
	jks_array_destroy(&profile->transparent_batches);
	jks_array_destroy(&profile->opaque_batches);
}

static void gx_m2_profile_initialize(struct gx_m2_profile *profile)
{
	for (size_t i = 0; i < profile->batches.size; ++i)
	{
		struct gx_m2_batch *batch = jks_array_get(&profile->batches, i);
		gx_m2_batch_initialize(batch);
	}
	profile->initialized = true;
}

static bool should_render_profile(struct gx_m2_instance *instance, struct gx_m2_batch *batch)
{
	if (batch->skin_section_id == 0 || instance->bypass_batches) /* XXX: Is it really always the case ? */
		return true;
	for (size_t i = 0; i < instance->enabled_batches.size; ++i)
	{
		if (batch->skin_section_id == *(uint16_t*)jks_array_get(&instance->enabled_batches, i))
			return true;
	}
	return false;
}

#if 0
#define TEST_BATCH 4
#endif

static void gx_m2_profile_render(struct gx_m2_profile *profile, struct jks_array *instances, bool transparent)
{
	if (!profile->initialized)
		return;
	struct jks_array *batches = transparent ? &profile->transparent_batches : &profile->opaque_batches;
	for (size_t i = 0; i < batches->size; ++i)
	{
		uint8_t batch_id = *(uint8_t*)jks_array_get(batches, i);
		bool initialized = false;
		struct gx_m2_batch *batch = (struct gx_m2_batch*)jks_array_get(&profile->batches, batch_id);
#ifdef TEST_BATCH
		if (strstr(profile->parent->filename, "HOURGLASS"))
		{
			if (batch->id != TEST_BATCH)
				continue;
		}
#endif
		for (size_t j = 0; j < instances->size; ++j)
		{
			static bool local_lighting = false;
			struct gx_m2_instance *instance = *(struct gx_m2_instance**)jks_array_get(instances, j);
			if (!should_render_profile(instance, batch))
				continue;
			if (!initialized)
			{
				if (!gx_m2_batch_prepare_draw(batch, NULL))
					break;
				initialized = true;
			}
			if (instance->local_lighting && instance->local_lighting->uniform_buffer.handle.u64)
			{
				local_lighting = true;
				gfx_bind_constant(g_wow->device, 2, &instance->local_lighting->uniform_buffer, sizeof(struct shader_m2_scene_block), 0);
			}
			else if (local_lighting)
			{
				local_lighting = false;
				gfx_bind_constant(g_wow->device, 2, &g_wow->draw_frame->m2_uniform_buffer, sizeof(struct shader_m2_scene_block), 0);
			}
			gx_m2_batch_render(batch, instance);
		}
	}
}

static void gx_m2_profile_render_instance(struct gx_m2_profile *profile, struct gx_m2_instance *instance, bool transparent, struct gx_m2_render_params *params)
{
	if (!profile->initialized)
		return;
	struct jks_array *batches = transparent ? &profile->transparent_batches : &profile->opaque_batches;
	for (size_t i = 0; i < batches->size; ++i)
	{
		uint8_t batch_id = *(uint8_t*)jks_array_get(batches, i);
		struct gx_m2_batch *batch = jks_array_get(&profile->batches, batch_id);
#ifdef TEST_BATCH
		if (strstr(profile->parent->filename, "HOURGLASS"))
		{
			if (batch->id != TEST_BATCH)
				continue;
		}
#endif
		if (!should_render_profile(instance, batch))
			continue;
		if (!gx_m2_batch_prepare_draw(batch, params))
			continue;
		gx_m2_batch_render(batch, instance);
	}
}

struct gx_m2 *gx_m2_new(const char *filename)
{
	struct gx_m2 *m2 = mem_malloc(MEM_GX, sizeof(*m2));
	if (!m2)
		return NULL;
	m2->filename = mem_strdup(MEM_GX, filename);
	if (!m2->filename)
	{
		LOG_ERROR("failed to allocate m2 filename");
		mem_free(MEM_GX, m2);
		return NULL;
	}
	m2->load_asked = false;
	m2->invalid = false;
	m2->loading = false;
	m2->loaded = false;
	m2->skybox = false;
	m2->has_transparent_batches = false;
	m2->has_opaque_batches = false;
	m2->in_render_list = false;
	m2->initialized = false;
	for (size_t i = 0; i < sizeof(m2->render_frames) / sizeof(*m2->render_frames); ++i)
		jks_array_init(&m2->render_frames[i].to_render, sizeof(struct gx_m2_instance*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&m2->instances, sizeof(struct gx_m2_instance*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&m2->profiles, sizeof(struct gx_m2_profile), (jks_array_destructor_t)gx_m2_profile_destroy, &jks_array_memory_fn_GX);
	jks_array_init(&m2->indices, sizeof(uint16_t), NULL, &jks_array_memory_fn_GX);
#ifdef WITH_DEBUG_RENDERING
	gx_m2_collisions_init(&m2->gx_collisions);
	gx_m2_lights_init(&m2->gx_lights);
	gx_m2_bones_init(&m2->gx_bones);
#endif
	m2->playable_animations = NULL;
	m2->texture_transforms = NULL;
	m2->attachment_lookups = NULL;
	m2->collision_triangles = NULL;
	m2->collision_vertexes = NULL;
	m2->collision_normals = NULL;
	m2->key_bone_lookups = NULL;
	m2->sequence_lookups = NULL;
	m2->global_sequences = NULL;
	m2->texture_weights = NULL;
	m2->bone_lookups = NULL;
	m2->attachments = NULL;
	m2->sequences = NULL;
	m2->particles = NULL;
	m2->textures = NULL;
	m2->vertexes = NULL;
	m2->cameras = NULL;
	m2->colors = NULL;
	m2->lights = NULL;
	m2->bones = NULL;
	m2->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	m2->vertexes_buffer = GFX_BUFFER_INIT();
	m2->indices_buffer = GFX_BUFFER_INIT();
	return m2;
}

void gx_m2_delete(struct gx_m2 *m2)
{
	if (!m2)
		return;
	for (size_t i = 0; i < sizeof(m2->render_frames) / sizeof(*m2->render_frames); ++i)
		jks_array_destroy(&m2->render_frames[i].to_render);
	jks_array_destroy(&m2->instances);
	jks_array_destroy(&m2->profiles);
	jks_array_destroy(&m2->indices);
#ifdef WITH_DEBUG_RENDERING
	gx_m2_collisions_destroy(&m2->gx_collisions);
	gx_m2_lights_destroy(&m2->gx_lights);
	gx_m2_bones_destroy(&m2->gx_bones);
#endif
	mem_free(MEM_GX, m2->playable_animations);
	wow_m2_texture_transforms_delete(m2->texture_transforms, m2->texture_transforms_nb);
	wow_m2_texture_weights_delete(m2->texture_weights, m2->texture_weights_nb);
	mem_free(MEM_GX, m2->collision_vertexes);
	mem_free(MEM_GX, m2->collision_normals);
	mem_free(MEM_GX, m2->collision_triangles);
	mem_free(MEM_GX, m2->attachment_lookups);
	mem_free(MEM_GX, m2->key_bone_lookups);
	mem_free(MEM_GX, m2->sequence_lookups);
	mem_free(MEM_GX, m2->global_sequences);
	mem_free(MEM_GX, m2->bone_lookups);
	wow_m2_attachments_delete(m2->attachments, m2->attachments_nb);
	wow_m2_sequences_delete(m2->sequences, m2->sequences_nb);
	wow_m2_particles_delete(m2->particles, m2->particles_nb);
	wow_m2_textures_delete(m2->textures, m2->textures_nb);
	mem_free(MEM_GX, m2->vertexes);
	wow_m2_cameras_delete(m2->cameras, m2->cameras_nb);
	wow_m2_colors_delete(m2->colors, m2->colors_nb);
	wow_m2_lights_delete(m2->lights, m2->lights_nb);
	wow_m2_bones_delete(m2->bones, m2->bones_nb);
	gfx_delete_buffer(g_wow->device, &m2->vertexes_buffer);
	gfx_delete_buffer(g_wow->device, &m2->indices_buffer);
	gfx_delete_attributes_state(g_wow->device, &m2->attributes_state);
	mem_free(MEM_GX, m2->filename);
	mem_free(MEM_GX, m2);
}

bool gx_m2_ask_load(struct gx_m2 *m2)
{
	if (m2->load_asked)
		return true;
	m2->load_asked = true;
	if (!loader_push(g_wow->loader, ASYNC_TASK_M2_LOAD, m2))
		return false;
	cache_ref_by_ref_unmutexed_m2(g_wow->cache, m2);
	return true;
}

bool gx_m2_ask_unload(struct gx_m2 *m2)
{
	return loader_gc_m2(g_wow->loader, m2);
}

int gx_m2_load(struct gx_m2 *m2, struct wow_m2_file *file)
{
	if (m2->loaded)
		return -1;
	struct vec3f p0 = {file->header.aabb0.x, file->header.aabb0.z, -file->header.aabb0.y};
	struct vec3f p1 = {file->header.aabb1.x, file->header.aabb1.z, -file->header.aabb1.y};
	VEC3_MIN(m2->aabb.p0, p0, p1);
	VEC3_MAX(m2->aabb.p1, p0, p1);
	VEC3_SET(p0, file->header.caabb0.x, file->header.caabb0.z, -file->header.caabb0.y);
	VEC3_SET(p1, file->header.caabb1.x, file->header.caabb1.z, -file->header.caabb1.y);
	VEC3_MIN(m2->caabb.p0, p0, p1);
	VEC3_MAX(m2->caabb.p1, p0, p1);
	m2->collision_sphere_radius = file->header.collision_sphere_radius;
	m2->version = file->header.version;
	m2->flags = file->header.flags;
	m2->render_distance = file->header.bounding_sphere_radius;
	m2->playable_animations_nb = file->playable_animations_nb;
	m2->playable_animations = mem_malloc(MEM_GX, sizeof(*m2->playable_animations) * m2->playable_animations_nb);
	if (!m2->playable_animations)
	{
		LOG_ERROR("failed to allocate m2 attachment lookups");
		return 0;
	}
	memcpy(m2->playable_animations, file->playable_animations, sizeof(*m2->playable_animations) * m2->playable_animations_nb);
	m2->texture_transforms_nb = file->texture_transforms_nb;
	m2->texture_transforms = wow_m2_texture_transforms_dup(file->texture_transforms, file->texture_transforms_nb);
	if (m2->texture_transforms_nb && !m2->texture_transforms)
	{
		LOG_ERROR("failed to allocate m2 m2 texture transforms");
		return 0;
	}
	m2->texture_weights_nb = file->texture_weights_nb;
	m2->texture_weights = wow_m2_texture_weights_dup(file->texture_weights, file->texture_weights_nb);
	if (m2->texture_weights_nb && !m2->texture_weights)
	{
		LOG_ERROR("failed to allocate m2 m2 texture weights");
		return 0;
	}
	m2->attachment_lookups_nb = file->attachment_lookups_nb;
	m2->attachment_lookups = mem_malloc(MEM_GX, sizeof(*m2->attachment_lookups) * m2->attachment_lookups_nb);
	if (!m2->attachment_lookups)
	{
		LOG_ERROR("failed to allocate m2 attachment lookups");
		return 0;
	}
	memcpy(m2->attachment_lookups, file->attachment_lookups, sizeof(*m2->attachment_lookups) * m2->attachment_lookups_nb);
	m2->attachments_nb = file->attachments_nb;
	m2->attachments = wow_m2_attachments_dup(file->attachments, file->attachments_nb);
	if (m2->attachments_nb && !m2->attachments)
	{
		LOG_ERROR("failed to allocate m2 attachments");
		return 0;
	}
	m2->collision_triangles_nb = file->collision_triangles_nb;
	m2->collision_triangles = mem_malloc(MEM_GX, sizeof(*m2->collision_triangles) * m2->collision_triangles_nb);
	if (!m2->collision_triangles)
	{
		LOG_ERROR("failed to allocate m2 collision triangles");
		return 0;
	}
	memcpy(m2->collision_triangles, file->collision_triangles, sizeof(*m2->collision_triangles) * m2->collision_triangles_nb);
	m2->collision_vertexes_nb = file->collision_vertexes_nb;
	m2->collision_vertexes = mem_malloc(MEM_GX, sizeof(*m2->collision_vertexes) * m2->collision_vertexes_nb);
	if (!m2->collision_vertexes)
	{
		LOG_ERROR("failed to allocate m2 collision vertexes");
		return 0;
	}
	memcpy(m2->collision_vertexes, file->collision_vertexes, sizeof(*m2->collision_vertexes) * m2->collision_vertexes_nb);
	m2->collision_normals_nb = file->collision_normals_nb;
	m2->collision_normals = mem_malloc(MEM_GX, sizeof(*m2->collision_normals) * m2->collision_normals_nb);
	if (!m2->collision_normals)
	{
		LOG_ERROR("failed to allocate m2 collision normals");
		return 0;
	}
	memcpy(m2->collision_normals, file->collision_normals, sizeof(*m2->collision_normals) * m2->collision_normals_nb);
	m2->sequences_nb = file->sequences_nb;
	m2->sequences = wow_m2_sequences_dup(file->sequences, file->sequences_nb);
	if (m2->sequences_nb && !m2->sequences)
	{
		LOG_ERROR("failed to allocate m2 sequences");
		return 0;
	}
	m2->particles_nb = file->particles_nb;
	m2->particles = wow_m2_particles_dup(file->particles, file->particles_nb);
	if (m2->particles_nb && !m2->particles)
	{
		LOG_ERROR("failed to allocate m2 particles");
		return 0;
	}
	m2->key_bone_lookups_nb = file->key_bone_lookups_nb;
	m2->key_bone_lookups = mem_malloc(MEM_GX, sizeof(*m2->key_bone_lookups) * m2->key_bone_lookups_nb);
	if (m2->key_bone_lookups_nb && !m2->key_bone_lookups)
	{
		LOG_ERROR("failed to allocate m2 key bone lookups");
		return 0;
	}
	memcpy(m2->key_bone_lookups, file->key_bone_lookups, sizeof(*m2->key_bone_lookups) * m2->key_bone_lookups_nb);
	m2->sequence_lookups_nb = file->sequence_lookups_nb;
	m2->sequence_lookups = mem_malloc(MEM_GX, sizeof(*m2->sequence_lookups) * m2->sequence_lookups_nb);
	if (m2->sequence_lookups_nb && !m2->sequence_lookups)
	{
		LOG_ERROR("failed to allocate m2 sequence lookups");
		return 0;
	}
	memcpy(m2->sequence_lookups, file->sequence_lookups, sizeof(*m2->sequence_lookups) * m2->sequence_lookups_nb);
	m2->global_sequences_nb = file->global_sequences_nb;
	m2->global_sequences = mem_malloc(MEM_GX, sizeof(*m2->global_sequences) * m2->global_sequences_nb);
	if (m2->global_sequences_nb && !m2->global_sequences)
	{
		LOG_ERROR("failed to allocate m2 global sequences");
		return 0;
	}
	memcpy(m2->global_sequences, file->global_sequences, sizeof(*m2->global_sequences) * m2->global_sequences_nb);
	m2->bone_lookups_nb = file->bone_lookups_nb;
	m2->bone_lookups = mem_malloc(MEM_GX, sizeof(*m2->bone_lookups) * m2->bone_lookups_nb);
	if (m2->bone_lookups_nb && !m2->bone_lookups)
	{
		LOG_ERROR("failed to allocate m2 bone lookups");
		return 0;
	}
	memcpy(m2->bone_lookups, file->bone_lookups, sizeof(*m2->bone_lookups) * m2->bone_lookups_nb);
	m2->textures_nb = file->textures_nb;
	m2->textures = wow_m2_textures_dup(file->textures, file->textures_nb);
	if (m2->textures_nb && !m2->textures)
	{
		LOG_ERROR("failed to allocate m2 textures");
		return 0;
	}
	m2->vertexes_nb = file->vertexes_nb;
	m2->vertexes = mem_malloc(MEM_GX, sizeof(*m2->vertexes) * m2->vertexes_nb);
	if (m2->vertexes_nb && !m2->vertexes)
	{
		LOG_ERROR("failed to allocate m2 vertexes");
		return 0;
	}
	memcpy(m2->vertexes, file->vertexes, sizeof(*m2->vertexes) * m2->vertexes_nb);
	m2->cameras_nb = file->cameras_nb;
	m2->cameras = wow_m2_cameras_dup(file->cameras, file->cameras_nb);
	if (m2->cameras_nb && !m2->cameras)
	{
		LOG_ERROR("failed to allocate m2 cameras");
		return 0;
	}
	m2->colors_nb = file->colors_nb;
	m2->colors = wow_m2_colors_dup(file->colors, file->colors_nb);
	if (m2->colors_nb && !m2->colors)
	{
		LOG_ERROR("failed to allocate m2 colors");
		return 0;
	}
	m2->lights_nb = file->lights_nb;
	m2->lights = wow_m2_lights_dup(file->lights, file->lights_nb);
	if (m2->lights_nb && !m2->lights)
	{
		LOG_ERROR("failed to allocate m2 lights");
		return 0;
	}
	m2->bones_nb = file->bones_nb;
	m2->bones = wow_m2_bones_dup(file->bones, file->bones_nb);
	if (m2->bones_nb && !m2->bones)
	{
		LOG_ERROR("failed to allocate m2 bones");
		return 0;
	}
	for (uint32_t i = 0; i < m2->particles_nb; ++i)
	{
		struct wow_m2_particle *particle = &m2->particles[i];
		particle->position = (struct wow_vec3f){particle->position.x, particle->position.z, -particle->position.y};
		particle->wind_vector = (struct wow_vec3f){particle->wind_vector.x, particle->wind_vector.z, -particle->wind_vector.y};
	}
	for (uint32_t i = 0; i < m2->attachments_nb; ++i)
	{
		struct wow_m2_attachment *attachment = &m2->attachments[i];
		attachment->position = (struct wow_vec3f){attachment->position.x, attachment->position.z, -attachment->position.y};
	}
	for (uint32_t i = 0; i < m2->lights_nb; ++i)
	{
		struct wow_m2_light *light = &m2->lights[i];
		light->position = (struct wow_vec3f){light->position.x, light->position.z, -light->position.y};
	}
	for (uint32_t i = 0; i < m2->bones_nb; ++i)
	{
		struct wow_m2_bone *bone = &m2->bones[i];
		bone->pivot = (struct wow_vec3f){bone->pivot.x, bone->pivot.z, -bone->pivot.y};
	}
	for (uint32_t i = 0; i < m2->collision_vertexes_nb; ++i)
	{
		struct wow_vec3f *v = &m2->collision_vertexes[i];
		*v = (struct wow_vec3f){v->x, v->z, -v->y};
	}
	for (uint32_t i = 0; i < m2->collision_normals_nb; ++i)
	{
		struct wow_vec3f *v = &m2->collision_normals[i];
		*v = (struct wow_vec3f){v->x, v->z, -v->y};
	}
	m2->has_billboard_bones = false;
	for (uint32_t i = 0; i < m2->bones_nb; ++i)
	{
		struct wow_m2_bone *bone = &m2->bones[i];
		if (bone->flags & WOW_M2_BONE_BILLBOARD)
		{
			m2->has_billboard_bones = true;
			break;
		}
	}
	for (size_t i = file->skin_profiles_nb - 1; i < file->skin_profiles_nb; ++i)
	{
		struct gx_m2_profile *profile = jks_array_grow(&m2->profiles, 1);
		if (!profile)
		{
			LOG_ERROR("failed to grow skin profiles array");
			continue;
		}
		if (!gx_m2_profile_init(profile, m2, file, &file->skin_profiles[i], &m2->indices))
		{
			LOG_ERROR("failed to initialize m2 skin profile");
			jks_array_resize(&m2->profiles, m2->profiles.size - 1);
			continue;
		}
		for (size_t j = 0; j < profile->batches.size; ++j)
		{
			struct gx_m2_batch *batch = jks_array_get(&profile->batches, j);
			if (batch->blending)
				m2->has_transparent_batches = true;
			else
				m2->has_opaque_batches = true;
		}
	}
#ifdef WITH_DEBUG_RENDERING
	if (!gx_m2_collisions_load(&m2->gx_collisions, m2))
		LOG_ERROR("failed to load m2 collisions m2");
	if (!gx_m2_lights_load(&m2->gx_lights, m2->lights, m2->lights_nb))
		LOG_ERROR("failed to load m2 lights m2");
	if (!gx_m2_bones_load(&m2->gx_bones, m2->bones, m2->bones_nb))
		LOG_ERROR("failed to load m2 bones m2");
#endif
	return 1;
}

bool gx_m2_post_load(struct gx_m2 *m2)
{
	m2->loaded = true;
	for (size_t i = 0; i < m2->instances.size; ++i)
	{
		struct gx_m2_instance *instance = *(struct gx_m2_instance**)jks_array_get(&m2->instances, i);
		gx_m2_instance_on_parent_loaded(instance);
	}
	/* XXX: remove init data ? */
	/* while (1)
	{
		switch (gx_m2_initialize(m2))
		{
			case 0:
				break;
			case 1:
				goto end;
			case -1:
				goto end;
		}
	}
	return true;
end:
	return false; */
	return loader_init_m2(g_wow->loader, m2);
}

int gx_m2_initialize(struct gx_m2 *m2)
{
	if (m2->initialized)
		return 1;
	for (size_t i = 0; i < m2->profiles.size; ++i)
	{
		struct gx_m2_profile *profile = jks_array_get(&m2->profiles, i);
		if (!profile->initialized)
		{
			gx_m2_profile_initialize(profile);
			return 0;
		}
	}
	gfx_create_buffer(g_wow->device, &m2->vertexes_buffer, GFX_BUFFER_VERTEXES, m2->vertexes, m2->vertexes_nb * sizeof(*m2->vertexes), GFX_BUFFER_IMMUTABLE);
	mem_free(MEM_GX, m2->vertexes);
	m2->vertexes = NULL;
	gfx_create_buffer(g_wow->device, &m2->indices_buffer, GFX_BUFFER_INDICES, m2->indices.data, m2->indices.size * sizeof(uint16_t), GFX_BUFFER_IMMUTABLE);
	jks_array_resize(&m2->indices, 0);
	jks_array_shrink(&m2->indices);
	gfx_attribute_bind_t binds[] =
	{
		{&m2->vertexes_buffer, sizeof(struct wow_m2_vertex), offsetof(struct wow_m2_vertex, bone_weights)},
		{&m2->vertexes_buffer, sizeof(struct wow_m2_vertex), offsetof(struct wow_m2_vertex, pos)},
		{&m2->vertexes_buffer, sizeof(struct wow_m2_vertex), offsetof(struct wow_m2_vertex, normal)},
		{&m2->vertexes_buffer, sizeof(struct wow_m2_vertex), offsetof(struct wow_m2_vertex, bone_indices)},
		{&m2->vertexes_buffer, sizeof(struct wow_m2_vertex), offsetof(struct wow_m2_vertex, tex_coords[0])},
		{&m2->vertexes_buffer, sizeof(struct wow_m2_vertex), offsetof(struct wow_m2_vertex, tex_coords[1])},
	};
	gfx_create_attributes_state(g_wow->device, &m2->attributes_state, binds, sizeof(binds) / sizeof(*binds), &m2->indices_buffer, GFX_INDEX_UINT16);
#ifdef WITH_DEBUG_RENDERING
	gx_m2_collisions_initialize(&m2->gx_collisions);
	gx_m2_lights_initialize(&m2->gx_lights);
	gx_m2_bones_initialize(&m2->gx_bones);
#endif
	m2->initialized = true;
	return 1;
}

void gx_m2_render(struct gx_m2 *m2, bool transparent)
{
	if (!m2->initialized)
		return;
	PERFORMANCE_BEGIN(M2_RENDER_BIND);
	gfx_bind_attributes_state(g_wow->device, &m2->attributes_state, &g_wow->graphics->m2_input_layout);
	PERFORMANCE_END(M2_RENDER_BIND);
	PERFORMANCE_BEGIN(M2_RENDER_DATA);
	for (size_t i = 0; i < m2->render_frames[g_wow->draw_frame_id].to_render.size; ++i)
	{
		struct gx_m2_instance *instance = *(struct gx_m2_instance**)jks_array_get(&m2->render_frames[g_wow->draw_frame_id].to_render, i);
		update_instance_uniform_buffer(instance);
	}
	PERFORMANCE_END(M2_RENDER_DATA);
	int profile_id = 0;
	struct gx_m2_profile *profile = jks_array_get(&m2->profiles, profile_id);
	gx_m2_profile_render(profile, &m2->render_frames[g_wow->draw_frame_id].to_render, transparent);
}

static void gx_m2_render_instance(struct gx_m2 *m2, struct gx_m2_instance *instance, bool transparent, struct gx_m2_render_params *params)
{
	if (!m2->initialized)
		return;
	PERFORMANCE_BEGIN(M2_RENDER_BIND);
	gfx_bind_attributes_state(g_wow->device, &m2->attributes_state, &g_wow->graphics->m2_input_layout);
	PERFORMANCE_END(M2_RENDER_BIND);
	PERFORMANCE_BEGIN(M2_RENDER_DATA);
	update_instance_uniform_buffer(instance);
	PERFORMANCE_END(M2_RENDER_DATA);
	int profile_id = 0;
	struct gx_m2_profile *profile = jks_array_get(&m2->profiles, profile_id);
	gx_m2_profile_render_instance(profile, instance, transparent, params);
}

#ifdef WITH_DEBUG_RENDERING
void gx_m2_render_aabb(struct gx_m2 *m2)
{
	if (!m2->initialized)
		return;
	for (size_t i = 0; i < m2->render_frames[g_wow->draw_frame_id].to_render.size; ++i)
	{
		struct gx_m2_instance *instance = *(struct gx_m2_instance**)jks_array_get(&m2->render_frames[g_wow->draw_frame_id].to_render, i);
		gx_aabb_update(&instance->gx_aabb, &g_wow->draw_frame->view_vp);
		gx_aabb_render(&instance->gx_aabb);
		gx_aabb_update(&instance->gx_caabb, &g_wow->draw_frame->view_vp);
		gx_aabb_render(&instance->gx_caabb);
	}
}

void gx_m2_render_bones_points(struct gx_m2 *m2)
{
	if (!m2->initialized)
		return;
	gx_m2_bones_render_points(&m2->gx_bones, (const struct gx_m2_instance**)m2->render_frames[g_wow->draw_frame_id].to_render.data, m2->render_frames[g_wow->draw_frame_id].to_render.size);
}

void gx_m2_render_bones_lines(struct gx_m2 *m2)
{
	if (!m2->initialized)
		return;
	gx_m2_bones_render_lines(&m2->gx_bones, (const struct gx_m2_instance**)m2->render_frames[g_wow->draw_frame_id].to_render.data, m2->render_frames[g_wow->draw_frame_id].to_render.size);
}

void gx_m2_render_lights(struct gx_m2 *m2)
{
	if (!m2->initialized)
		return;
	gx_m2_lights_render(&m2->gx_lights, m2->render_frames[g_wow->draw_frame_id].to_render.data, m2->render_frames[g_wow->draw_frame_id].to_render.size);
}

void gx_m2_render_collisions(struct gx_m2 *m2, bool triangles)
{
	if (!m2->initialized)
		return;
	gx_m2_collisions_render(&m2->gx_collisions, m2->render_frames[g_wow->draw_frame_id].to_render.data, m2->render_frames[g_wow->draw_frame_id].to_render.size, triangles);
}
#endif

void gx_m2_add_to_render(struct gx_m2 *m2)
{
	if (m2->in_render_list)
		return;
	m2->in_render_list = true;
	if (m2->has_opaque_batches)
		render_add_m2_opaque(m2);
}

void gx_m2_clear_update(struct gx_m2 *m2)
{
	for (size_t i = 0; i < m2->render_frames[g_wow->cull_frame_id].to_render.size; ++i)
	{
		struct gx_m2_instance *instance = *JKS_ARRAY_GET(&m2->render_frames[g_wow->cull_frame_id].to_render, i, struct gx_m2_instance*);
		gx_m2_instance_clear_update(instance);
	}
	jks_array_resize(&m2->render_frames[g_wow->cull_frame_id].to_render, 0);
}

struct gx_m2_instance *gx_m2_instance_new(struct gx_m2 *parent)
{
	struct gx_m2_instance *instance = mem_zalloc(MEM_GX, sizeof(*instance));
	if (!instance)
		return NULL;
	instance->parent = parent;
	cache_lock_m2(g_wow->cache);
	cache_ref_by_ref_unmutexed_m2(g_wow->cache, parent);
	if (!jks_array_push_back(&instance->parent->instances, &instance))
	{
		cache_unref_by_ref_unmutexed_m2(g_wow->cache, parent);
		cache_unlock_m2(g_wow->cache);
		LOG_ERROR("failed to add m2 instance to parent");
		mem_free(MEM_GX, instance);
		return NULL;
	}
	MAT4_IDENTITY(instance->m_inv);
	MAT4_IDENTITY(instance->m);
	instance->camera = -1;
	instance->sequence_speed = 1;
	instance->render_distance_max = -1;
	instance->scale = 1;
	for (size_t i = 0; i < sizeof(instance->render_frames) / sizeof(*instance->render_frames); ++i)
	{
		jks_array_init(&instance->render_frames[i].bone_mats, sizeof(struct mat4f), NULL, &jks_array_memory_fn_GX);
		instance->render_frames[i].culled = true;
	}
	jks_array_init(&instance->lights, sizeof(struct gx_m2_light), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&instance->enabled_batches, sizeof(uint16_t), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&instance->bone_calc, sizeof(uint8_t), NULL, &jks_array_memory_fn_GX);
	if (instance->parent->loaded)
		gx_m2_instance_on_parent_loaded(instance);
	cache_unlock_m2(g_wow->cache);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		instance->uniform_buffers[i] = GFX_BUFFER_INIT();
#ifdef WITH_DEBUG_RENDERING
	gx_aabb_init(&instance->gx_aabb, (struct vec4f){1, 1, 1, 1}, 1);
	gx_aabb_init(&instance->gx_caabb, (struct vec4f){.5, .5, .5, 1}, 1);
#endif
	return instance;
}

struct gx_m2_instance *gx_m2_instance_new_filename(const char *filename)
{
	struct gx_m2_instance *instance = mem_zalloc(MEM_GX, sizeof(*instance));
	if (!instance)
		return NULL;
	cache_lock_m2(g_wow->cache);
	if (!cache_ref_by_key_unmutexed_m2(g_wow->cache, filename, &instance->parent))
	{
		cache_unlock_m2(g_wow->cache);
		LOG_ERROR("failed to get m2 instance ref: %s", filename);
		mem_free(MEM_GX, instance);
		return NULL;
	}
	if (!jks_array_push_back(&instance->parent->instances, &instance))
	{
		cache_unref_by_ref_unmutexed_m2(g_wow->cache, instance->parent);
		cache_unlock_m2(g_wow->cache);
		LOG_ERROR("failed to add m2 instance to parent");
		mem_free(MEM_GX, instance);
		return NULL;
	}
	MAT4_IDENTITY(instance->m_inv);
	MAT4_IDENTITY(instance->m);
	instance->camera = -1;
	instance->sequence_speed = 1;
	instance->render_distance_max = -1;
	instance->scale = 1;
	for (size_t i = 0; i < sizeof(instance->render_frames) / sizeof(*instance->render_frames); ++i)
	{
		jks_array_init(&instance->render_frames[i].bone_mats, sizeof(struct mat4f), NULL, &jks_array_memory_fn_GX);
		instance->render_frames[i].culled = true;
	}
	jks_array_init(&instance->lights, sizeof(struct gx_m2_light), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&instance->enabled_batches, sizeof(uint16_t), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&instance->bone_calc, sizeof(uint8_t), NULL, &jks_array_memory_fn_GX);
	if (instance->parent->loaded)
		gx_m2_instance_on_parent_loaded(instance);
	cache_unlock_m2(g_wow->cache);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		instance->uniform_buffers[i] = GFX_BUFFER_INIT();
#ifdef WITH_DEBUG_RENDERING
	gx_aabb_init(&instance->gx_aabb, (struct vec4f){1, 1, 1, 1}, 1);
	gx_aabb_init(&instance->gx_caabb, (struct vec4f){.5, .5, .5, 1}, 1);
#endif
	return instance;
}

void gx_m2_instance_delete(struct gx_m2_instance *instance)
{
	if (!instance)
		return;
	{
		cache_lock_m2(g_wow->cache);
		size_t i;
		for (i = 0; i < instance->parent->instances.size; ++i)
		{
			struct gx_m2_instance *other = *(struct gx_m2_instance**)jks_array_get(&instance->parent->instances, i);
			if (instance != other)
				continue;
			goto found;
		}
		cache_unlock_m2(g_wow->cache);
		goto unref;
found:
		jks_array_erase(&instance->parent->instances, i);
		cache_unlock_m2(g_wow->cache);
	}
unref:
	gx_m2_particles_delete(instance->gx_particles);
	cache_unref_by_ref_m2(g_wow->cache, instance->parent);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_delete_buffer(g_wow->device, &instance->uniform_buffers[i]);
	jks_array_destroy(&instance->lights);
	jks_array_destroy(&instance->enabled_batches);
	for (size_t i = 0; i < sizeof(instance->render_frames) / sizeof(*instance->render_frames); ++i)
		jks_array_destroy(&instance->render_frames[i].bone_mats);
	jks_array_destroy(&instance->bone_calc);
#ifdef WITH_DEBUG_RENDERING
	gx_aabb_destroy(&instance->gx_aabb);
	gx_aabb_destroy(&instance->gx_caabb);
#endif
	if (instance->local_lighting)
	{
		gfx_delete_buffer(g_wow->device, &instance->local_lighting->uniform_buffer);
		mem_free(MEM_GX, instance->local_lighting);
	}
	mem_free(MEM_GX, instance);
}

void gx_m2_instance_gc(struct gx_m2_instance *instance)
{
	if (!instance)
		return;
	render_gc_m2(instance);
}

static void update_local_lighting(struct gx_m2_instance *instance)
{
	if (!instance->local_lighting)
		return;
	struct m2_local_lighting *loc = instance->local_lighting;
	if (!loc->uniform_buffer.handle.u64)
		gfx_create_buffer(g_wow->device, &loc->uniform_buffer, GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_m2_scene_block), GFX_BUFFER_STREAM);
	struct shader_m2_scene_block data;
	VEC4_CPY(data.light_direction, loc->light_direction);
	VEC4_CPY(data.specular_color, loc->diffuse_color);
	VEC4_CPY(data.diffuse_color, loc->diffuse_color);
	VEC4_CPY(data.ambient_color, loc->ambient_color);
	data.fog_range.y = g_wow->draw_frame->view_distance * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_END] / 36 / g_wow->map->fog_divisor;
	data.fog_range.x = data.fog_range.y * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_START];
	gfx_set_buffer_data(&loc->uniform_buffer, &data, sizeof(data), 0);
}

static void update_instance_uniform_buffer(struct gx_m2_instance *instance)
{
	size_t bones_off = offsetof(struct shader_m2_model_block, bone_mats);
	size_t lights_off = offsetof(struct shader_m2_model_block, lights);
	if (!instance->uniform_buffers[g_wow->draw_frame_id].handle.u64)
		gfx_create_buffer(g_wow->device, &instance->uniform_buffers[g_wow->draw_frame_id], GFX_BUFFER_UNIFORM, NULL, bones_off + instance->render_frames[g_wow->draw_frame_id].bone_mats.size * sizeof(struct mat4f), GFX_BUFFER_STREAM);
	update_local_lighting(instance);
	struct shader_m2_model_block data;
	data.v = *(struct mat4f*)&g_wow->draw_frame->view_v;
	data.mv = instance->render_frames[g_wow->draw_frame_id].mv;
	data.mvp = instance->render_frames[g_wow->draw_frame_id].mvp;
	if (instance->parent->bones_nb)
	{
		if (instance->parent->bones_nb <= sizeof(data.bone_mats) / sizeof(*data.bone_mats))
		{
			gfx_set_buffer_data(&instance->uniform_buffers[g_wow->draw_frame_id], instance->render_frames[g_wow->draw_frame_id].bone_mats.data, instance->render_frames[g_wow->draw_frame_id].bone_mats.size * sizeof(struct mat4f), bones_off);
		}
		else
		{
			LOG_ERROR("too much bones (%u, %s)", instance->parent->bones_nb, instance->parent->filename);
		}
	}
	if (instance->enable_lights)
	{
		data.light_count.x = instance->parent->lights_nb;
		if (data.light_count.x > 4)
			data.light_count.x = 4;
		for (int i = 0; i < data.light_count.x; ++i)
		{
			struct wow_m2_light *light = &instance->parent->lights[i];
			/* Ambient */
			struct vec3f ambient_rgb;
			if (!m2_instance_get_track_value_vec3f(instance, &light->ambient_color, &ambient_rgb))
				VEC3_SETV(ambient_rgb, 0);
			float ambient_alpha;
			if (!m2_instance_get_track_value_float(instance, &light->ambient_intensity, &ambient_alpha))
				ambient_alpha = 0;
			VEC3_CPY(data.lights[i].ambient, ambient_rgb);
			data.lights[i].ambient.w = ambient_alpha;
			/* Diffuse */
			struct vec3f diffuse_rgb;
			if (!m2_instance_get_track_value_vec3f(instance, &light->diffuse_color, &diffuse_rgb))
				VEC3_SETV(diffuse_rgb, 0);
			float diffuse_alpha;
			if (!m2_instance_get_track_value_float(instance, &light->diffuse_intensity, &diffuse_alpha))
				diffuse_alpha = 0;
			VEC3_CPY(data.lights[i].diffuse, diffuse_rgb);
			data.lights[i].diffuse.w = diffuse_alpha;
			/* Attenuation */
			if (!m2_instance_get_track_value_float(instance, &light->attenuation_start, &data.lights[i].attenuation.x))
				data.lights[i].attenuation.x = 0;
			if (!m2_instance_get_track_value_float(instance, &light->attenuation_end, &data.lights[i].attenuation.y))
				data.lights[i].attenuation.y = 0;
			/* Position */
			VEC3_CPY(data.lights[i].position, light->position);
			data.lights[i].position.w = light->type;
			if (light->bone != -1)
			{
				struct mat4f *bone_mat = JKS_ARRAY_GET(&instance->render_frames[g_wow->draw_frame_id].bone_mats, light->bone, struct mat4f);
				struct vec4f tmp;
				MAT4_VEC4_MUL(tmp, *bone_mat, data.lights[i].position);
				VEC4_CPY(data.lights[i].position, tmp);
			}
			{
				struct vec4f tmp;
				MAT4_VEC4_MUL(tmp, instance->m, data.lights[i].position);
				VEC4_CPY(data.lights[i].position, tmp);
			}
			/* Misc */
			uint8_t enabled;
			if (!m2_instance_get_track_value_uint8(instance, &light->visibility, &enabled))
				enabled = 0;
			data.lights[i].data.x = enabled;
			data.lights[i].data.y = light->type;
			/* if (i != 3)
				data.lights[i].data.x = 0; */
			/* 3: dir purple
			 * 2: left top back bright
			 * 1: top right front clear
			 * 0: green
			 */
			/*
			LOG_INFO("light idx: %d", (int)i);
			LOG_INFO("light attenuation: {x: %f, y: %f}", data.lights[i].attenuation.x, data.lights[i].attenuation.y);
			LOG_INFO("light enable: %d", (int)enabled);
			LOG_INFO("light bone: %d / %d", light->bone, (int)instance->parent->bones_nb);
			LOG_INFO("light type: %d", light->type);
			LOG_INFO("light diffuse: {%f, %f, %f, %f}", data.lights[i].diffuse.x, data.lights[i].diffuse.y, data.lights[i].diffuse.z, data.lights[i].diffuse.w);
			LOG_INFO("light ambient: {%f, %f, %f, %f}", data.lights[i].ambient.x, data.lights[i].ambient.y, data.lights[i].ambient.z, data.lights[i].ambient.w);
			LOG_INFO(" ");
			*/
		}
	}
	else
	{
		data.light_count.x = 0;
	}
	gfx_set_buffer_data(&instance->uniform_buffers[g_wow->draw_frame_id], &data, lights_off + data.light_count.x * sizeof(struct shader_m2_light_block), 0);
}

static void calc_bone_billboard(struct gx_m2_instance *instance, struct wow_m2_bone *bone, struct mat4f *mat)
{
	if (bone->flags & WOW_M2_BONE_SPHERICAL_BILLBOARD)
	{
		struct mat4f tmp;
		struct mat4f t;
		MAT4_IDENTITY(t);
		VEC3_CPY(t.x, instance->m_inv.x);
		VEC3_CPY(t.y, instance->m_inv.y);
		VEC3_CPY(t.z, instance->m_inv.z);
		MAT4_ROTATEZ(float, tmp, t, -g_wow->cull_frame->cull_rot.z);
		MAT4_ROTATEY(float, t, tmp, -g_wow->cull_frame->cull_rot.y);
		MAT4_ROTATEX(float, tmp, t, -g_wow->cull_frame->cull_rot.x);
		MAT4_ROTATEY(float, t, tmp, -M_PI / 2);
		tmp = t;
		float n;
		n = instance->scale * VEC3_NORM(mat->x);
		VEC3_MULV(tmp.x, tmp.x, n);
		n = instance->scale * VEC3_NORM(mat->y);
		VEC3_MULV(tmp.y, tmp.y, n);
		n = instance->scale * VEC3_NORM(mat->z);
		VEC3_MULV(tmp.z, tmp.z, n);
		VEC3_CPY(mat->x, tmp.x);
		VEC3_CPY(mat->y, tmp.y);
		VEC3_CPY(mat->z, tmp.z);
	}
	else if (bone->flags & WOW_M2_BONE_CYLINDRICAL_X_BILLBOARD)
	{
		struct mat4f tmp;
		struct mat4f t;
		MAT4_IDENTITY(t);
		VEC3_CPY(t.x, instance->m_inv.x);
		VEC3_CPY(t.y, instance->m_inv.y);
		VEC3_CPY(t.z, instance->m_inv.z);
		MAT4_ROTATEX(float, tmp, t, -g_wow->cull_frame->cull_rot.x);
		float n;
		n = instance->scale * VEC3_NORM(mat->y);
		VEC3_MULV(tmp.y, tmp.y, n);
		n = instance->scale * VEC3_NORM(mat->z);
		VEC3_MULV(tmp.z, tmp.z, n);
		VEC3_CPY(mat->y, tmp.y);
		VEC3_CPY(mat->z, tmp.z);
	}
	else if (bone->flags & WOW_M2_BONE_CYLINDRICAL_Y_BILLBOARD)
	{
		struct mat4f tmp;
		struct mat4f t;
		MAT4_IDENTITY(t);
		VEC3_CPY(t.x, instance->m_inv.x);
		VEC3_CPY(t.y, instance->m_inv.y);
		VEC3_CPY(t.z, instance->m_inv.z);
		MAT4_ROTATEZ(float, tmp, t, -g_wow->cull_frame->cull_rot.z);
		float n;
		n = instance->scale * VEC3_NORM(mat->x);
		VEC3_MULV(tmp.x, tmp.x, n);
		n = instance->scale * VEC3_NORM(mat->y);
		VEC3_MULV(tmp.y, tmp.y, n);
		VEC3_CPY(mat->x, tmp.x);
		VEC3_CPY(mat->y, tmp.y);
	}
	else if (bone->flags & WOW_M2_BONE_CYLINDRICAL_Z_BILLBOARD)
	{
		struct mat4f tmp;
		struct mat4f t;
		MAT4_IDENTITY(t);
		VEC3_CPY(t.x, instance->m_inv.x);
		VEC3_CPY(t.y, instance->m_inv.y);
		VEC3_CPY(t.z, instance->m_inv.z);
		MAT4_ROTATEY(float, tmp, t, -g_wow->cull_frame->cull_rot.y - M_PI / 2);
		float n;
		n = instance->scale * VEC3_NORM(mat->x);
		VEC3_MULV(tmp.x, tmp.x, n);
		n = instance->scale * VEC3_NORM(mat->z);
		VEC3_MULV(tmp.z, tmp.z, n);
		VEC3_CPY(mat->x, tmp.x);
		VEC3_CPY(mat->z, tmp.z);
	}
}

static void calc_bone_sequence(struct gx_m2_instance *instance, struct mat4f *mat, struct wow_m2_bone *bone)
{
	{
		struct mat4f tmp_mat;
		MAT4_TRANSLATE(tmp_mat, *mat, bone->pivot);
		*mat = tmp_mat;
	}
	{
		struct vec3f v;
		if (m2_instance_get_track_value_vec3f(instance, &bone->translation, &v))
		{
			struct vec3f tmp = {v.x, v.z, -v.y};
			struct mat4f tmp_mat;
			MAT4_TRANSLATE(tmp_mat, *mat, tmp);
			*mat = tmp_mat;
		}
	}
	if (bone->flags & WOW_M2_BONE_BILLBOARD)
	{
		calc_bone_billboard(instance, bone, mat);
	}
	else
	{
		struct vec4f v;
		if (m2_instance_get_track_value_quat16(instance, &bone->rotation, &v))
		{
			VEC4_NORMALIZE(float, v, v);
			struct vec4f tmp = {v.x, v.z, -v.y, v.w};
			struct mat4f quat;
			QUATERNION_TO_MAT4(float, quat, tmp);
			struct mat4f tmp_mat;
			MAT4_MUL(tmp_mat, *mat, quat);
			*mat = tmp_mat;
		}
	}
	{
		struct vec3f v;
		if (m2_instance_get_track_value_vec3f(instance, &bone->scale, &v))
		{
			struct vec3f tmp = {v.x, v.z, v.y};
			MAT4_SCALE(*mat, *mat, tmp);
		}
	}
	{
		struct vec3f tmp = {-bone->pivot.x, -bone->pivot.y, -bone->pivot.z};
		struct mat4f tmp_mat;
		MAT4_TRANSLATE(tmp_mat, *mat, tmp);
		*mat = tmp_mat;
	}
}

static void calc_bone(struct gx_m2_instance *instance, uint16_t bone_id)
{
	if (bone_id > instance->bone_calc.size * 8)
		return;
	if ((*JKS_ARRAY_GET(&instance->bone_calc, bone_id / 8, uint8_t)) & (1 << (bone_id % 8)))
		return;
	struct wow_m2_bone *bone = &instance->parent->bones[bone_id];
	if (!(bone->flags & 0x278))
	{
		struct mat4f *dst = JKS_ARRAY_GET(&instance->render_frames[g_wow->cull_frame_id].bone_mats, bone_id, struct mat4f);
		if (bone->parent_bone != -1)
		{
			calc_bone(instance, bone->parent_bone);
			*dst = *JKS_ARRAY_GET(&instance->render_frames[g_wow->cull_frame_id].bone_mats, bone->parent_bone, struct mat4f);
		}
		else
		{
			MAT4_IDENTITY(*dst);
		}
		*JKS_ARRAY_GET(&instance->bone_calc, bone_id / 8, uint8_t) |= 1 << (bone_id % 8);
		return;
	}
	struct mat4f mat;
	if (bone->parent_bone != -1)
	{
		calc_bone(instance, bone->parent_bone);
		mat = *JKS_ARRAY_GET(&instance->render_frames[g_wow->cull_frame_id].bone_mats, bone->parent_bone, struct mat4f);
	}
	else
	{
		MAT4_IDENTITY(mat);
	}
	if (instance->sequence && instance->sequence->end > instance->sequence->start)
		calc_bone_sequence(instance, &mat, bone);
	*JKS_ARRAY_GET(&instance->render_frames[g_wow->cull_frame_id].bone_mats, bone_id, struct mat4f) = mat;
	*JKS_ARRAY_GET(&instance->bone_calc, bone_id / 8, uint8_t) |= 1 << (bone_id % 8);
}

void gx_m2_instance_calc_remaining_bones(struct gx_m2_instance *instance)
{
	instance->bones_calculated = true;
	if (!instance->bone_calc.size)
		return;
	for (size_t i = 0; i < instance->parent->bones_nb; ++i)
		calc_bone(instance, i);
}

void gx_m2_instance_calc_bones(struct gx_m2_instance *instance)
{
	if (instance->bones_calculated)
		return;
	memset(instance->bone_calc.data, 0, instance->bone_calc.size);
	gx_m2_instance_calc_remaining_bones(instance);
}

void gx_m2_instance_clear_update(struct gx_m2_instance *instance)
{
	instance->render_frames[g_wow->cull_frame_id].culled = true;
}

static void update_matrix(struct gx_m2_instance *instance, struct gx_m2_render_params *params)
{
	if (instance->camera == (uint32_t)-1)
	{
		MAT4_MUL(instance->render_frames[g_wow->cull_frame_id].mv, g_wow->cull_frame->view_v, instance->m);
		MAT4_MUL(instance->render_frames[g_wow->cull_frame_id].mvp, g_wow->cull_frame->view_p, instance->render_frames[g_wow->cull_frame_id].mv);
		return;
	}
	if (instance->camera >= instance->parent->cameras_nb)
	{
		LOG_WARN("invalid camera id: %u / %u", instance->camera, instance->parent->cameras_nb);
		MAT4_MUL(instance->render_frames[g_wow->cull_frame_id].mv, g_wow->cull_frame->view_v, instance->m);
		MAT4_MUL(instance->render_frames[g_wow->cull_frame_id].mvp, g_wow->cull_frame->view_p, instance->render_frames[g_wow->cull_frame_id].mv);
		return;
	}
	struct wow_m2_camera *camera = &instance->parent->cameras[instance->camera];
	float fov = camera->fov / sqrt(1 + pow(params->aspect, 2));
	struct mat4f p;
	MAT4_PERSPECTIVE(p, fov, params->aspect, camera->near_clip, camera->far_clip);
	struct mat4f v;
	MAT4_IDENTITY(v);
	struct vec3f position = {camera->position_base.x, camera->position_base.z, -camera->position_base.y};
	struct vec3f target = {camera->target_position_base.x, camera->target_position_base.z, -camera->target_position_base.y};
	struct vec3f up = {0, 1, 0};
	MAT4_LOOKAT(float, v, position, target, up);
	/* XXX tracks */
	MAT4_MUL(instance->render_frames[g_wow->cull_frame_id].mv, v, instance->m);
	MAT4_MUL(instance->render_frames[g_wow->cull_frame_id].mvp, p, instance->render_frames[g_wow->cull_frame_id].mv);
}

static void update_sequences_times(struct gx_m2_instance *instance)
{
	if (instance->sequence && instance->sequence->end > instance->sequence->start)
		instance->sequence_time = instance->sequence->start + (uint32_t)((g_wow->frametime - instance->sequence_started) * instance->sequence_speed / 1000000);
	if (instance->prev_sequence && instance->prev_sequence->end > instance->prev_sequence->start)
		instance->prev_sequence_time = instance->prev_sequence->start + (uint32_t)((g_wow->frametime - instance->prev_sequence_started) * instance->sequence_speed / 1000000);
}

void gx_m2_instance_force_update(struct gx_m2_instance *instance, struct gx_m2_render_params *params)
{
	if (!instance->parent->loaded)
	{
		instance->render_frames[g_wow->cull_frame_id].culled = true;
		return;
	}
	update_matrix(instance, params);
	gx_m2_instance_calc_bones(instance);
	update_sequences_times(instance);
	instance->render_frames[g_wow->cull_frame_id].culled = false;
}

void gx_m2_instance_update(struct gx_m2_instance *instance, bool bypass_frustum)
{
	if (instance->update_calculated)
		return;
	instance->update_calculated = true;
	if (!instance->parent->loaded)
	{
		instance->render_frames[g_wow->cull_frame_id].culled = true;
		return;
	}
	/* bypass frustum shouldn't bypass range check */
	struct vec3f tmp;
	VEC3_SUB(tmp, g_wow->cull_frame->cull_pos, instance->pos);
	float distance = VEC3_NORM(tmp);
	float max_distance = g_wow->cull_frame->view_distance * instance->parent->render_distance / 8 * instance->scale;
	if (instance->render_distance_max != -1 && instance->render_distance_max < max_distance)
		max_distance = instance->render_distance_max;
	if (distance > max_distance)
	{
		instance->render_frames[g_wow->cull_frame_id].culled = true;
		return;
	}
	if (!bypass_frustum)
	{
		if (!frustum_check_fast(&g_wow->cull_frame->frustum, &instance->aabb))
		{
			instance->render_frames[g_wow->cull_frame_id].culled = true;
			return;
		}
	}
	update_matrix(instance, NULL);
	gx_m2_instance_calc_bones(instance);
	update_sequences_times(instance);
}

void gx_m2_instance_render(struct gx_m2_instance *instance, bool transparent, struct gx_m2_render_params *params)
{
	if (instance->render_frames[g_wow->draw_frame_id].culled)
		return;
	gx_m2_render_instance(instance->parent, instance, transparent, params);
}

void gx_m2_instance_render_particles(struct gx_m2_instance *instance)
{
	if (instance->render_frames[g_wow->draw_frame_id].culled) /* XXX: still create particles ? */
		return;
	gx_m2_particles_render(instance->gx_particles);
}

void gx_m2_instance_calculate_distance_to_camera(struct gx_m2_instance *instance)
{
	struct vec3f tmp;
	VEC3_SUB(tmp, instance->pos, g_wow->cull_frame->cull_pos);
	instance->render_frames[g_wow->cull_frame_id].distance_to_camera = VEC3_NORM(tmp);
}

void gx_m2_instance_add_to_render(struct gx_m2_instance *instance, bool bypass_frustum)
{
	if (!render_add_m2_instance(instance, bypass_frustum))
		return;
	if (!jks_array_push_back(&instance->parent->render_frames[g_wow->cull_frame_id].to_render, &instance))
	{
		LOG_ERROR("failed to add m2 renderer instance to parent render list");
		return;
	}
	gx_m2_add_to_render(instance->parent);
	if (instance->parent->has_transparent_batches)
		render_add_m2_transparent(instance);
	if (instance->gx_particles)
	{
		gx_m2_particles_update(instance->gx_particles);
		render_add_m2_particles(instance);
	}
}

void gx_m2_instance_set_mat(struct gx_m2_instance *instance, struct mat4f *mat)
{
	instance->m = *mat;
	MAT4_INVERSE(float, instance->m_inv, *mat);
	if (instance->parent->loaded)
		gx_m2_instance_update_aabb(instance);
}

void gx_m2_instance_update_aabb(struct gx_m2_instance *instance)
{
	instance->aabb = instance->parent->aabb;
	aabb_transform(&instance->aabb, &instance->m);
#ifdef WITH_DEBUG_RENDERING
	instance->gx_aabb.aabb = instance->aabb;
	instance->gx_aabb.flags |= GX_AABB_DIRTY;
#endif
	instance->caabb = instance->parent->caabb;
	aabb_transform(&instance->caabb, &instance->m);
#ifdef WITH_DEBUG_RENDERING
	instance->gx_caabb.aabb = instance->caabb;
	instance->gx_caabb.flags |= GX_AABB_DIRTY;
#endif
}

static void resolve_sequence(struct gx_m2_instance *instance)
{
	uint32_t seq_id;
	if (instance->sequence_id < instance->parent->playable_animations_nb)
		seq_id = instance->parent->playable_animations[instance->sequence_id].id;
	else
		seq_id = 0;
	size_t i = seq_id % instance->parent->sequence_lookups_nb;
	uint16_t lookup;
	for (size_t stride = 1;; ++stride)
	{
		lookup = instance->parent->sequence_lookups[i];
		if (lookup == (uint16_t)-1)
		{
			LOG_WARN("sequence not found: %u", seq_id);
			lookup = 0;
			break;
		}
		if (instance->parent->sequences[lookup].id == seq_id)
			break;
		i = (i + stride * stride) % instance->parent->sequence_lookups_nb;
	}
	instance->sequence = &instance->parent->sequences[lookup];
}

void gx_m2_instance_on_parent_loaded(struct gx_m2_instance *instance)
{
	gx_m2_instance_update_aabb(instance);
	for (size_t i = 0; i < sizeof(instance->render_frames) / sizeof(*instance->render_frames); ++i)
	{
		if (!jks_array_resize(&instance->render_frames[i].bone_mats, instance->parent->bones_nb))
		{
			LOG_ERROR("failed to alloc bones matrixes array");
			return;
		}
	}
	if (!jks_array_resize(&instance->bone_calc, instance->parent->bones_nb))
	{
		LOG_ERROR("failed to alloc bones calc array");
		return;
	}
	if (instance->parent->particles_nb != 0)
	{
		instance->gx_particles = gx_m2_particles_new(instance);
		if (!instance->gx_particles)
			LOG_ERROR("failed to load particles renderer");
	}
	resolve_sequence(instance);
}

void gx_m2_instance_set_sequence(struct gx_m2_instance *instance, uint32_t sequence)
{
	if (sequence == instance->sequence_id)
		return;
	instance->prev_sequence = instance->sequence;
	instance->prev_sequence_started = instance->sequence_started;
	instance->prev_sequence_id = instance->sequence_id;
	instance->prev_sequence_time = instance->sequence_time;
	instance->sequence_id = sequence;
	if (instance->parent->loaded)
		resolve_sequence(instance);
	instance->sequence_started = g_wow->frametime;
}

void gx_m2_instance_set_skin_extra_texture(struct gx_m2_instance *instance, struct blp_texture *texture)
{
	instance->skin_extra_texture = texture;
}

void gx_m2_instance_set_skin_texture(struct gx_m2_instance *instance, struct blp_texture *texture)
{
	instance->skin_texture = texture;
}

void gx_m2_instance_set_hair_texture(struct gx_m2_instance *instance, struct blp_texture *texture)
{
	instance->hair_texture = texture;
}

void gx_m2_instance_set_monster_texture(struct gx_m2_instance *instance, int id, struct blp_texture *texture)
{
	instance->monster_textures[id] = texture;
}

void gx_m2_instance_set_object_texture(struct gx_m2_instance *instance, struct blp_texture *texture)
{
	instance->object_texture = texture;
}

void gx_m2_instance_enable_batch(struct gx_m2_instance *instance, uint16_t batch)
{
	if (!jks_array_push_back(&instance->enabled_batches, &batch))
		LOG_ERROR("failed to add enabled batch");
}

void gx_m2_instance_disable_batch(struct gx_m2_instance *instance, uint16_t batch)
{
	for (size_t i = 0; i < instance->enabled_batches.size; ++i)
	{
		if (*(uint16_t*)jks_array_get(&instance->enabled_batches, i) == batch)
		{
			jks_array_erase(&instance->enabled_batches, i);
			return;
		}
	}
}

void gx_m2_instance_enable_batches(struct gx_m2_instance *instance, uint16_t start, uint16_t end)
{
	if (end <= start)
		return;
	uint16_t diff = end - start;
	uint16_t *data = jks_array_grow(&instance->enabled_batches, diff);
	if (data)
	{
		for (uint16_t i = 0; i < diff; ++i)
			data[i] = start + i;
	}
	else
	{
		LOG_ERROR("failed to add enabled batches");
	}
}

void gx_m2_instance_disable_batches(struct gx_m2_instance *instance, uint16_t start, uint16_t end)
{
	for (size_t i = 0; i < instance->enabled_batches.size; ++i)
	{
		uint16_t batch = *(uint16_t*)jks_array_get(&instance->enabled_batches, i);
		if (batch >= start && batch <= end)
		{
			jks_array_erase(&instance->enabled_batches, i);
			i--;
		}
	}
}

void gx_m2_instance_clear_batches(struct gx_m2_instance *instance)
{
	jks_array_resize(&instance->enabled_batches, 0);
}
