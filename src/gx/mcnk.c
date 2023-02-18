#include "gx/frame.h"
#include "gx/mcnk.h"
#include "gx/wmo.h"
#include "gx/m2.h"

#include "map/tile.h"
#include "map/map.h"

#include "performance.h"
#include "graphics.h"
#include "shaders.h"
#include "camera.h"
#include "memory.h"
#include "cache.h"
#include "const.h"
#include "blp.h"
#include "log.h"
#include "wow.h"
#include "dbc.h"

#include <jks/mat3.h>

#include <gfx/device.h>

#include <libwow/wdt.h>
#include <libwow/adt.h>
#include <libwow/dbc.h>

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define GROUND_DOODADS_RANGE_MIN (CHUNK_WIDTH * 5)
#define GROUND_DOODADS_RANGE_MAX (CHUNK_WIDTH * 6)

#if 0
# define GROUND_DOODADS
#endif

#if 0
# define USE_LOW_DEF_ALPHA
#endif

MEMORY_DECL(GX);

struct mcnk_init_data
{
	struct shader_mcnk_input *vertexes;
	uint32_t vertexes_nb;
	uint16_t *indices;
	uint32_t indices_nb;
	uint8_t *alpha[GX_MCNK_BATCHES_NB];
};

struct gx_mcnk_ground_effect
{
	union
	{
		uint8_t no_effect_doodad[8];
		uint64_t no_effect_doodads;
	};
	uint16_t effects_models[8][8][4];
	uint16_t effect_id[8][8];
	bool loaded;
	struct gx_m2 **models;
	uint32_t models_nb;
	struct jks_array doodads; /* gx_m2_instance_t* */
};

static void clean_init_data(struct mcnk_init_data *init_data)
{
	if (!init_data)
		return;
	mem_free(MEM_GX, init_data->vertexes);
	mem_free(MEM_GX, init_data->indices);
	for (uint32_t i = 0; i < GX_MCNK_BATCHES_NB; ++i)
		mem_free(MEM_GX, init_data->alpha[i]);
	mem_free(MEM_GX, init_data);
}

static bool init_init_data(struct gx_mcnk *mcnk, struct wow_adt_file *file)
{
	mcnk->init_data->indices_nb = 0;
	for (size_t batch = 0; batch < GX_MCNK_BATCHES_NB; ++batch)
	{
		struct wow_mcnk *wow_mcnk = &file->mcnk[batch];
		for (size_t z = 0; z < 4; ++z)
		{
			for (size_t x = 0; x < 4; ++x)
			{
				static const uint32_t lod_indices[16] =
				{
					12, 9, 9, 12,
					9 , 6, 6, 9 ,
					9 , 6, 6, 9 ,
					12, 9, 9, 12
				};
				uint32_t idx = z * 4 + x;
				if (!(wow_mcnk->header.holes & (1 << idx)))
					mcnk->init_data->indices_nb += 18 * 4 + lod_indices[idx];
			}
		}
	}
	mcnk->init_data->indices = mem_malloc(MEM_GX, sizeof(*mcnk->init_data->indices) * mcnk->init_data->indices_nb);
	if (!mcnk->init_data->indices)
	{
		LOG_ERROR("allocation failed");
		return false;
	}
	mcnk->init_data->vertexes_nb = (9 * 9 + 8 * 8) * GX_MCNK_BATCHES_NB;
	mcnk->init_data->vertexes = mem_malloc(MEM_GX, sizeof(*mcnk->init_data->vertexes) * mcnk->init_data->vertexes_nb);
	if (!mcnk->init_data->vertexes)
	{
		LOG_ERROR("allocation failed");
		return false;
	}
	return true;
}

static void init_indices(struct wow_mcnk *mcnk, struct gx_mcnk_batch *mcnk_batch, uint32_t batch, struct mcnk_init_data *init_data, uint32_t *indices_pos_ptr)
{
	uint32_t indices_pos = *indices_pos_ptr;
	mcnk_batch->indices_offsets[0] = indices_pos;
	uint16_t base = batch * (9 * 9 + 8 * 8);
	for (size_t z = 0; z < 8; ++z)
	{
		for (size_t x = 0; x < 8; ++x)
		{
			if (mcnk->header.holes & (1 << (z / 2 * 4 + x / 2)))
				continue;
			uint16_t idx = base + 9 + z * 17 + x;
			uint16_t p1 = idx - 9;
			uint16_t p2 = idx - 8;
			uint16_t p3 = idx + 9;
			uint16_t p4 = idx + 8;
			init_data->indices[indices_pos++] = p2;
			init_data->indices[indices_pos++] = p1;
			init_data->indices[indices_pos++] = idx;
			init_data->indices[indices_pos++] = p3;
			init_data->indices[indices_pos++] = p2;
			init_data->indices[indices_pos++] = idx;
			init_data->indices[indices_pos++] = p4;
			init_data->indices[indices_pos++] = p3;
			init_data->indices[indices_pos++] = idx;
			init_data->indices[indices_pos++] = p1;
			init_data->indices[indices_pos++] = p4;
			init_data->indices[indices_pos++] = idx;
		}
	}
	mcnk_batch->indices_nbs[0] = indices_pos - mcnk_batch->indices_offsets[0];
	mcnk_batch->indices_offsets[1] = indices_pos;
	for (size_t z = 0; z < 8; ++z)
	{
		for (size_t x = 0; x < 8; ++x)
		{
			if (mcnk->header.holes & (1 << (z / 2 * 4 + x / 2)))
				continue;
			uint16_t idx = base + 9 + z * 17 + x;
			uint16_t p1 = idx - 9;
			uint16_t p2 = idx - 8;
			uint16_t p3 = idx + 9;
			uint16_t p4 = idx + 8;
			init_data->indices[indices_pos++] = p2;
			init_data->indices[indices_pos++] = p1;
			init_data->indices[indices_pos++] = p3;
			init_data->indices[indices_pos++] = p3;
			init_data->indices[indices_pos++] = p1;
			init_data->indices[indices_pos++] = p4;
		}
	}
	mcnk_batch->indices_nbs[1] = indices_pos - mcnk_batch->indices_offsets[1];
	mcnk_batch->indices_offsets[2] = indices_pos;
	for (size_t z = 0; z < 4; ++z)
	{
		for (size_t x = 0; x < 4; ++x)
		{
			if (mcnk->header.holes & (1 << (z * 4 + x)))
				continue;
			static const uint16_t points[16][13] =
			{
				{12, 1  , 0  , 17 , 36 , 17 , 34 , 36 , 2  , 1  , 36 , 1  , 17},
				{ 9, 36 , 3  , 2  , 38 , 4  , 3  , 38 , 3  , 36},
				{ 9, 38 , 5  , 4  , 40 , 6  , 5  , 40 , 5  , 38},
				{12, 25 , 8  , 7  , 40 , 42 , 25 , 40 , 7  , 6  , 40 , 25 , 7},
				{ 9, 70 , 51 , 68 , 36 , 34 , 51 , 36 , 51 , 70},
				{ 6, 36 , 70 , 72 , 72 , 38 , 36},
				{ 6, 40 , 38 , 72 , 72 , 74 , 40},
				{ 9, 40 , 59 , 42 , 74 , 76 , 59 , 74 , 59 , 40},
				{ 9, 104, 85 , 102, 70 , 68 , 85 , 70 , 85 , 104},
				{ 6, 104, 106, 72 , 72 , 70 , 104},
				{ 6, 72 , 106, 108, 108, 74 , 72},
				{ 9, 74 , 93 , 76 , 108, 110, 93 , 108, 93 , 74},
				{12, 119, 136, 137, 104, 102, 119, 104, 137, 138, 104, 119, 137},
				{ 9, 106, 139, 140, 104, 138, 139, 104, 139, 106},
				{ 9, 108, 141, 142, 106, 140, 141, 106, 141, 108},
				{12, 143, 144, 127, 108, 142, 143, 108, 127, 110, 108, 143, 127},
			};
			for (size_t j = 1; j <= points[z * 4 + x][0]; ++j)
				init_data->indices[indices_pos++] = base + points[z * 4 + x][j];
		}
	}
	mcnk_batch->indices_nbs[2] = indices_pos - mcnk_batch->indices_offsets[2];
	*indices_pos_ptr = indices_pos;
}

static bool init_textures(struct gx_mcnk *mcnk, struct wow_adt_file *file, uint32_t *textures, uint32_t textures_nb)
{
	mcnk->textures_nb = textures_nb;
	mcnk->diffuse_textures = mem_malloc(MEM_GX, sizeof(*mcnk->diffuse_textures) * mcnk->textures_nb);
	if (!mcnk->diffuse_textures)
		return false;
	mcnk->specular_textures = mem_malloc(MEM_GX, sizeof(*mcnk->specular_textures) * mcnk->textures_nb);
	if (!mcnk->specular_textures)
	{
		mem_free(MEM_GX, mcnk->diffuse_textures);
		return false;
	}
	for (uint32_t i = 0; i < textures_nb; ++i)
	{
		uint32_t texture = textures[i];
		if (texture >= file->textures_nb)
		{
			mcnk->diffuse_textures[i] = NULL;
			mcnk->specular_textures[i] = NULL;
			LOG_ERROR("invalid texture: %u / %u", texture, file->textures_nb);
			continue;
		}
		char texture_name[512];
		snprintf(texture_name, sizeof(texture_name), "%s", file->textures[texture]);
		normalize_blp_filename(texture_name, sizeof(texture_name));
		if (cache_ref_by_key_blp(g_wow->cache, texture_name, &mcnk->diffuse_textures[i]))
		{
			blp_texture_ask_load(mcnk->diffuse_textures[i]);
		}
		else
		{
			LOG_WARN("can't get texture: %s", texture_name);
			mcnk->diffuse_textures[i] = NULL;
		}
		size_t len = strlen(texture_name);
		snprintf(texture_name + len - 4, sizeof(texture_name) - len + 4, "_s.blp");
		normalize_blp_filename(texture_name, sizeof(texture_name));
		if (cache_ref_by_key_blp(g_wow->cache, texture_name, &mcnk->specular_textures[i]))
		{
			blp_texture_ask_load(mcnk->specular_textures[i]);
		}
		else
		{
			LOG_WARN("can't get texture: %s", texture_name);
			mcnk->specular_textures[i] = NULL;
		}
	}
	return true;
}

static void init_holes(struct gx_mcnk *mcnk, struct wow_adt_file *file)
{
	for (size_t batch = 0; batch < GX_MCNK_BATCHES_NB; ++batch)
	{
		struct wow_mcnk *wow_mcnk = &file->mcnk[batch];
		if (!wow_mcnk->header.holes)
			continue;
		mcnk->holes = true;
		break;
	}
}

static bool init_ground_effects(struct wow_mcnk *mcnk, struct gx_mcnk_batch *mcnk_batch)
{
#ifdef GROUND_DOODADS
	/* new only if there are ground effects */
	mcnk_batch->ground_effect = mem_malloc(MEM_GX, sizeof(*mcnk_batch->ground_effect));
	if (!mcnk_batch->ground_effect)
		return false;
	jks_array_init(&mcnk_batch->ground_effect->doodads, sizeof(struct gx_m2_instance*), NULL, &jks_array_memory_fn_GX);
	mcnk_batch->ground_effect->no_effect_doodads = mcnk->header.no_effect_doodads;
	mcnk_batch->ground_effect->loaded = false;
	for (uint32_t z = 0; z < 8; ++z)
	{
		for (uint32_t x = 0; x < 8; ++x)
		{
			if (mcnk->header.layers)
			{
				uint8_t layer = (mcnk->header.low_quality_texture[z] >> (x * 2)) & 0x3;
				mcnk_batch->ground_effect->effect_id[z][x] = mcnk->mcly.data[layer].effect_id;
			}
			else
			{
				mcnk_batch->ground_effect->effect_id[z][x] = 0;
			}
		}
	}
#else
	(void)mcnk;
	mcnk_batch->ground_effect = NULL;
#endif
	return true;
}

static bool init_batch(struct gx_mcnk *mcnk, uint32_t batch, struct wow_mcnk *wow_mcnk, struct gx_mcnk_batch *mcnk_batch, uint32_t *indices_pos)
{
	if (!init_ground_effects(wow_mcnk, mcnk_batch))
		return false;
#ifdef WITH_DEBUG_RENDERING
	gx_aabb_init(&mcnk_batch->doodads_gx_aabb, (struct vec4f){1, 1, 1, 1}, 1);
	gx_aabb_init(&mcnk_batch->wmos_gx_aabb, (struct vec4f){.5, .5, 1, 1}, 1);
	gx_aabb_init(&mcnk_batch->gx_aabb, (struct vec4f){0, 1, 1, 1}, 1);
#endif
	mcnk_batch->x = batch / 16;
	mcnk_batch->z = batch % 16;
	mcnk_batch->holes = wow_mcnk->header.holes;
	mcnk_batch->doodads_nb = wow_mcnk->mcrf.doodads_nb;
	if (mcnk_batch->doodads_nb)
	{
		mcnk_batch->doodads = mem_malloc(MEM_GX, sizeof(*mcnk_batch->doodads) * mcnk_batch->doodads_nb);
		if (!mcnk_batch->doodads)
			return false;
		memcpy(mcnk_batch->doodads, wow_mcnk->mcrf.doodads, sizeof(*mcnk_batch->doodads) * mcnk_batch->doodads_nb);
		jks_array_init(&mcnk_batch->doodads_to_aabb, sizeof(uint32_t), NULL, &jks_array_memory_fn_GX);
		uint32_t *doodads_to_aabb = jks_array_grow(&mcnk_batch->doodads_to_aabb, mcnk_batch->doodads_nb);
		if (!doodads_to_aabb)
			return false;
		memcpy(doodads_to_aabb, wow_mcnk->mcrf.doodads, sizeof(*mcnk_batch->doodads) * mcnk_batch->doodads_nb);
	}
	mcnk_batch->wmos_nb = wow_mcnk->mcrf.wmos_nb;
	if (mcnk_batch->wmos_nb)
	{
		mcnk_batch->wmos = mem_malloc(MEM_GX, sizeof(*mcnk_batch->wmos) * mcnk_batch->wmos_nb);
		if (!mcnk_batch->wmos)
			return false;
		memcpy(mcnk_batch->wmos, wow_mcnk->mcrf.wmos, sizeof(*mcnk_batch->wmos) * mcnk_batch->wmos_nb);
		jks_array_init(&mcnk_batch->wmos_to_aabb, sizeof(uint32_t), NULL, &jks_array_memory_fn_GX);
		uint32_t *wmos_to_aabb = jks_array_grow(&mcnk_batch->wmos_to_aabb, mcnk_batch->wmos_nb);
		if (!wmos_to_aabb)
			return false;
		memcpy(wmos_to_aabb, wow_mcnk->mcrf.wmos, sizeof(*mcnk_batch->wmos) * mcnk_batch->wmos_nb);
	}
	VEC3_SETV(mcnk_batch->doodads_aabb.p0, -INFINITY);
	VEC3_SETV(mcnk_batch->doodads_aabb.p1, -INFINITY);
	VEC3_SETV(mcnk_batch->wmos_aabb.p0, -INFINITY);
	VEC3_SETV(mcnk_batch->wmos_aabb.p1, -INFINITY);
	if (!mcnk->holes)
	{
		mcnk_batch->indices_offsets[0] = (8 * 8 * 4 * 3 + 8 * 8 * 2 * 3 + 48 * 3) * batch;
		mcnk_batch->indices_offsets[1] = mcnk_batch->indices_offsets[0] + 8 * 8 * 4 * 3;
		mcnk_batch->indices_offsets[2] = mcnk_batch->indices_offsets[1] + 8 * 8 * 2 * 3;
		mcnk_batch->indices_nbs[0] = 8 * 8 * 4 * 3;
		mcnk_batch->indices_nbs[1] = 8 * 8 * 2 * 3;
		mcnk_batch->indices_nbs[2] = 48 * 3;
		return true;
	}
	init_indices(wow_mcnk, mcnk_batch, batch, mcnk->init_data, indices_pos);
	return true;
}

static bool init_batches(struct gx_mcnk *mcnk, struct wow_adt_file *file)
{
	uint32_t indices_pos = 0;
	for (uint32_t batch = 0; batch < GX_MCNK_BATCHES_NB; ++batch)
	{
		struct wow_mcnk *wow_mcnk = &file->mcnk[batch];
		struct gx_mcnk_batch *mcnk_batch = &mcnk->batches[batch];
		if (!init_batch(mcnk, batch, wow_mcnk, mcnk_batch, &indices_pos))
			return false;
	}
	return true;
}

static void init_batch_textures(struct wow_mcnk *mcnk, struct gx_mcnk_batch *mcnk_batch, uint32_t *textures, size_t *textures_nb)
{
	for (size_t i = 0; i < 4; ++i)
	{
		if (i >= mcnk->header.layers)
		{
			mcnk_batch->textures[i] = 0;
			continue;
		}
		uint32_t texture_id = mcnk->mcly.data[i].texture_id;
		for (size_t j = 0; j < *textures_nb; ++j)
		{
			if (textures[j] == texture_id)
			{
				mcnk_batch->textures[i] = j;
				goto next_texture;
			}
		}
		mcnk_batch->textures[i] = *textures_nb;
		textures[(*textures_nb)++] = texture_id;
next_texture:
		continue;
	}
}

static void init_batch_vertices(struct gx_mcnk *mcnk, struct wow_mcnk *wow_mcnk, struct gx_mcnk_batch *mcnk_batch, uint32_t batch, float *min_max_y)
{
	float mcnk_min_y = +999999;
	float mcnk_max_y = -999999;
	struct shader_mcnk_input *iter = mcnk->init_data->vertexes + (9 * 9 + 8 * 8) * batch;
	for (size_t i = 0; i < 9 * 9 + 8 * 8; ++i)
	{
		float y = wow_mcnk->mcvt.height[i] + wow_mcnk->header.position.z;
		mcnk_batch->height[i] = y;
		if (y > mcnk_max_y)
			mcnk_max_y = y;
		if (y < mcnk_min_y)
			mcnk_min_y = y;
		(iter++)->y = y;
	}
	if (mcnk_max_y > min_max_y[1])
		min_max_y[1] = mcnk_max_y;
	if (mcnk_min_y < min_max_y[0])
		min_max_y[0] = mcnk_min_y;
	float base_x = -CHUNK_WIDTH * mcnk_batch->x;
	float base_z = +CHUNK_WIDTH * mcnk_batch->z;
	struct vec3f p0 = {base_x, mcnk_min_y, base_z};
	VEC3_ADD(p0, p0, mcnk->pos);
	struct vec3f p1 = {base_x - CHUNK_WIDTH, mcnk_max_y, base_z + CHUNK_WIDTH};
	VEC3_ADD(p1, p1, mcnk->pos);
	VEC3_MIN(mcnk_batch->aabb.p0, p0, p1);
	VEC3_MAX(mcnk_batch->aabb.p1, p0, p1);
#ifdef WITH_DEBUG_RENDERING
	mcnk_batch->gx_aabb.aabb = mcnk_batch->aabb;
	mcnk_batch->gx_aabb.flags |= GX_AABB_DIRTY;
#endif
	VEC3_ADD(mcnk_batch->center, mcnk_batch->aabb.p0, mcnk_batch->aabb.p1);
	VEC3_MULV(mcnk_batch->center, mcnk_batch->center, .5f);
	p0.y = -999999;
	p1.y = +999999;
	VEC3_MIN(mcnk_batch->objects_aabb.p0, p0, p1);
	VEC3_MAX(mcnk_batch->objects_aabb.p1, p0, p1);
}

static void init_batch_normals(struct gx_mcnk *mcnk, struct wow_mcnk *wow_mcnk, struct gx_mcnk_batch *mcnk_batch, uint32_t batch)
{
	struct shader_mcnk_input *iter = mcnk->init_data->vertexes + (9 * 9 + 8 * 8) * batch;
	size_t j = 0;
	for (size_t i = 0; i < 9 * 9 + 8 * 8; ++i)
	{
		iter->norm.x =  wow_mcnk->mcnr.normal[j + 0];
		iter->norm.y =  wow_mcnk->mcnr.normal[j + 2];
		iter->norm.z = -wow_mcnk->mcnr.normal[j + 1];
		mcnk_batch->norm[j + 0] = iter->norm.x;
		mcnk_batch->norm[j + 1] = iter->norm.y;
		mcnk_batch->norm[j + 2] = iter->norm.z;
		iter++;
		j += 3;
	}
}

static void init_batch_animations(struct wow_mcnk *mcnk, struct gx_mcnk_batch *mcnk_batch)
{
	for (size_t l = 0; l < mcnk->header.layers; ++l)
	{
		struct wow_mcly_data *mcly = &mcnk->mcly.data[l];
		if (mcly->flags.animation_enabled)
		{
			static const float speeds[] = {1, 2, 4, 8, 16, 32, 48, 64};
			mcnk_batch->layers_animations[l].x = speeds[mcly->flags.animation_speed] * 0.176776695; /* sqrt(2) / 8 */
			mcnk_batch->layers_animations[l].y = mcnk_batch->layers_animations[l].x;
			struct mat3f mat;
			struct mat3f tmp;
			MAT3_IDENTITY(tmp);
			MAT3_ROTATEZ(float, mat, tmp, M_PI * .25 + mcly->flags.animation_rotation * M_PI * .25);
			struct vec3f tmp_animation = {mcnk_batch->layers_animations[l].x, mcnk_batch->layers_animations[l].y, 0};
			struct vec3f res;
			MAT3_VEC3_MUL(res, mat, tmp_animation);
			mcnk_batch->layers_animations[l] = (struct vec2f){res.x, res.y};
		}
		else
		{
			mcnk_batch->layers_animations[l] = (struct vec2f){0, 0};
		}
	}
}

#ifdef USE_LOW_DEF_ALPHA

static void init_batch_alpha_low(struct gx_mcnk *mcnk, struct wow_mcnk *wow_mcnk, uint32_t batch)
{
	uint8_t *iter = mcnk->init_data->alpha[batch];
	if (g_wow->map->flags & WOW_MPHD_FLAG_BIG_ALPHA)
	{
		for (size_t z = 0; z < 64; ++z)
		{
			for (size_t x = 0; x < 64; ++x)
			{
				uint8_t val = ((wow_mcnk->header.low_quality_texture[z / 8] >> (x / 8 * 2)) & 0x3);
				switch (val)
				{
					case 0:
						iter += 4;
						break;
					case 1:
						iter += 3;
						*iter = 0xFF;
						iter += 1;
						break;
					case 2:
						iter += 2;
						*iter = 0xFF;
						iter += 2;
						break;
					case 3:
						iter += 1;
						*iter = 0xFF;
						iter += 3;
						break;
				}
			}
		}
	}
	else
	{
		for (size_t z = 0; z < 64; ++z)
		{
			for (size_t x = 0; x < 64; ++x)
			{
				uint8_t val = ((wow_mcnk->header.low_quality_texture[z / 8] >> (x / 8 * 2)) & 0x3);
				switch (val)
				{
					case 0:
						iter += 2;
						break;
					case 1:
						++iter;
						*iter = 0x0F;
						++iter;
						break;
					case 2:
						*iter = 0xF0;
						iter += 2;
						break;
					case 3:
						*iter = 0x0F;
						iter += 2;
						break;
				}
			}
		}
	}
}

#else

static void init_layer_big_alpha(struct gx_mcnk *mcnk, struct wow_mcnk *wow_mcnk, uint32_t batch, uint32_t l, uint8_t *mcal_iter, uint8_t *tmp_mcal)
{
	uint8_t *large_data;
	if (wow_mcnk->mcly.data[l].flags.alpha_map_compressed)
	{
		large_data = tmp_mcal;
		for (size_t pos = 0; pos < 4096;)
		{
			uint8_t meta = *(mcal_iter++);
			if (!meta)
			{
				LOG_DEBUG("invalid meta");
				break;
			}
			if (meta & 0x80)
			{
				uint8_t data = *(mcal_iter++);
				for (size_t k = 0; k < (meta & 0x7F) && pos < 4096; ++k)
					tmp_mcal[pos++] = data;
			}
			else
			{
				for (size_t k = 0; k < (meta & 0x7F) && pos < 4096; ++k)
					tmp_mcal[pos++] = *(mcal_iter++);
			}
		}
	}
	else
	{
		large_data = mcal_iter;
	}
	uint8_t *iter = mcnk->init_data->alpha[batch] + 3 - l;
	for (size_t z = 0; z < 64; ++z)
	{
		for (size_t x = 0; x < 64; ++x)
		{
			size_t tx;
			size_t tz;
			if (!(wow_mcnk->header.flags & WOW_MCNK_FLAGS_FIX_MCAL))
			{
				tx = x == 63 ? 62 : x;
				tz = z == 63 ? 62 : z;
			}
			else
			{
				tx = x;
				tz = z;
			}
			*iter = large_data[tz * 64 + tx];
			iter += 4;
		}
	}
}

static void init_layer_std_alpha(struct gx_mcnk *mcnk, struct wow_mcnk *wow_mcnk, uint32_t batch, uint32_t l, uint8_t *mcal_iter)
{
	uint8_t *iter = mcnk->init_data->alpha[batch];
	l = 4 - l;
	uint8_t shifter = (l % 2) * 4;
	if (l > 1)
		++iter;
	for (size_t z = 0; z < 64; ++z)
	{
		for (size_t x = 0; x < 64; ++x)
		{
			size_t tx;
			size_t tz;
			if (!(wow_mcnk->header.flags & WOW_MCNK_FLAGS_FIX_MCAL))
			{
				tx = x == 63 ? 62 : x;
				tz = z == 63 ? 62 : z;
			}
			else
			{
				tx = x;
				tz = z;
			}
			*iter |= ((mcal_iter[tz * 32 + tx / 2] >> ((tx & 1) << 2)) & 0xF) << shifter;
			iter += 2;
		}
	}
}

static void init_batch_alpha(struct gx_mcnk *mcnk, struct wow_mcnk *wow_mcnk, uint32_t batch, uint8_t *tmp_mcal)
{
	for (uint32_t l = 1; l < wow_mcnk->header.layers; ++l)
	{
		if (!wow_mcnk->mcly.data[l].flags.use_alpha_map)
			continue;
		uint8_t *mcal_iter = wow_mcnk->mcal.data + wow_mcnk->mcly.data[l].offset_in_mcal;
		if (g_wow->map->flags & WOW_MPHD_FLAG_BIG_ALPHA)
		{
			init_layer_big_alpha(mcnk, wow_mcnk, batch, l, mcal_iter, tmp_mcal);
		}
		else
		{
			init_layer_std_alpha(mcnk, wow_mcnk, batch, l, mcal_iter);
		}
	}
}

#endif

static void init_batch_shadow(struct gx_mcnk *mcnk, struct wow_mcnk *wow_mcnk, uint32_t batch)
{
	if (wow_mcnk->header.flags & WOW_MCNK_FLAGS_MCSH)
	{
		uint8_t *iter = mcnk->init_data->alpha[batch];
		if (g_wow->map->flags & WOW_MPHD_FLAG_BIG_ALPHA)
		{
			iter += 3;
			for (size_t z = 0; z < 64; ++z)
			{
				for (size_t x = 0; x < 64; ++x)
				{
					*iter = ((wow_mcnk->mcsh.shadow[z][x / 8] >> (x % 8)) & 1) ? 0 : 0xFF;
					iter += 4;
				}
			}
		}
		else
		{
			for (size_t z = 0; z < 64; ++z)
			{
				for (size_t x = 0; x < 64; ++x)
				{
					*iter |= ((wow_mcnk->mcsh.shadow[z][x / 8] >> (x % 8)) & 1) ? 0 : 0xF;
					iter += 2;
				}
			}
		}
	}
	else
	{
		uint8_t *iter = mcnk->init_data->alpha[batch];
		if (g_wow->map->flags & WOW_MPHD_FLAG_BIG_ALPHA)
		{
			iter += 3;
			for (size_t z = 0; z < 64; ++z)
			{
				for (size_t x = 0; x < 64; ++x)
				{
					*iter = 0xFF;
					iter += 4;
				}
			}
		}
		else
		{
			for (size_t z = 0; z < 64; ++z)
			{
				for (size_t x = 0; x < 64; ++x)
				{
					*iter |= 0xF;
					iter += 2;
				}
			}
		}
	}
}

struct gx_mcnk *gx_mcnk_new(struct map_tile *parent, struct wow_adt_file *file)
{
	uint8_t tmp_mcal[4096];
	uint32_t textures[16 * 16 * 4];
	size_t textures_nb = 0;
	float min_max_y[2];
	size_t alpha_bpp;
	struct vec3f p0;
	struct vec3f p1;
	struct mat4f m;
	MAT4_IDENTITY(m);
	struct gx_mcnk *mcnk = mem_zalloc(MEM_GX, sizeof(*mcnk));
	if (!mcnk)
		return NULL;
	mcnk->parent = parent;
	mcnk->init_data = mem_zalloc(MEM_GX, sizeof(*mcnk->init_data));
	if (!mcnk->init_data)
		goto err1;
#ifdef WITH_DEBUG_RENDERING
	gx_aabb_init(&mcnk->doodads_gx_aabb, (struct vec4f){.25, .25, 1, 1}, 3);
	gx_aabb_init(&mcnk->wmos_gx_aabb, (struct vec4f){1, .25, .25, 1}, 3);
	gx_aabb_init(&mcnk->gx_aabb, (struct vec4f){1, .4, 0, 1}, 3);
#endif
	mcnk->pos = (struct vec3f){file->mcnk[0].header.position.x, 0, -file->mcnk[0].header.position.y};
	MAT4_TRANSLATE(mcnk->m, m, mcnk->pos);
	init_holes(mcnk, file);
	if (!init_init_data(mcnk, file))
		goto err2;
	if (!init_batches(mcnk, file))
		goto err2;
	min_max_y[0] = +9999;
	min_max_y[1] = -9999;
	alpha_bpp = (g_wow->map->flags & WOW_MPHD_FLAG_BIG_ALPHA) ? 4 : 2;
	for (size_t batch = 0; batch < GX_MCNK_BATCHES_NB; ++batch)
	{
		struct wow_mcnk *wow_mcnk = &file->mcnk[batch];
		struct gx_mcnk_batch *mcnk_batch = &mcnk->batches[batch];
		init_batch_animations(wow_mcnk, mcnk_batch);
		mcnk->init_data->alpha[batch] = mem_malloc(MEM_GX, 64 * 64 * alpha_bpp);
		if (!mcnk->init_data->alpha[batch])
			goto err2;
		memset(mcnk->init_data->alpha[batch], 0, 64 * 64 * alpha_bpp);
		/* alpha */
		{
#ifdef USE_LOW_DEF_ALPHA
			init_batch_alpha_low(mcnk, wow_mcnk, batch);
#else
			init_batch_alpha(mcnk, wow_mcnk, batch, tmp_mcal);
#endif
			init_batch_shadow(mcnk, wow_mcnk, batch);
		}
		init_batch_normals(mcnk, wow_mcnk, mcnk_batch, batch);
		init_batch_vertices(mcnk, wow_mcnk, mcnk_batch, batch, min_max_y);
		init_batch_textures(wow_mcnk, mcnk_batch, textures, &textures_nb);
	}
	VEC3_SET(p0, 0, min_max_y[0], 0);
	VEC3_ADD(p0, p0, mcnk->pos);
	VEC3_SET(p1, -CHUNK_WIDTH * 16, min_max_y[1], CHUNK_WIDTH * 16);
	VEC3_ADD(p1, p1, mcnk->pos);
	VEC3_MIN(mcnk->aabb.p0, p0, p1);
	VEC3_MAX(mcnk->aabb.p1, p0, p1);
#ifdef WITH_DEBUG_RENDERING
	mcnk->gx_aabb.aabb = mcnk->aabb;
	mcnk->gx_aabb.flags |= GX_AABB_DIRTY;
#endif
	p0.y = -9999;
	p1.y = +9999;
	VEC3_MIN(mcnk->objects_aabb.p0, p0, p1);
	VEC3_MAX(mcnk->objects_aabb.p1, p0, p1);
	VEC3_ADD(mcnk->center, mcnk->aabb.p0, mcnk->aabb.p1);
	VEC3_MULV(mcnk->center, mcnk->center, .5);
	if (!init_textures(mcnk, file, textures, textures_nb))
		goto err2;
	mcnk->alpha_texture = GFX_TEXTURE_INIT();
	mcnk->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	mcnk->batches_uniform_buffer = GFX_BUFFER_INIT();
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		mcnk->uniform_buffers[i] = GFX_BUFFER_INIT();
	mcnk->vertexes_buffer = GFX_BUFFER_INIT();
	mcnk->indices_buffer = GFX_BUFFER_INIT();
	return mcnk;

err2:
	clean_init_data(mcnk->init_data);
err1:
	mem_free(MEM_GX, mcnk);
	return NULL;
}

void gx_mcnk_delete(struct gx_mcnk *mcnk)
{
	if (!mcnk)
		return;
	for (uint32_t i = 0; i < mcnk->textures_nb; ++i)
	{
		if (mcnk->specular_textures[i])
			cache_unref_by_ref_blp(g_wow->cache, mcnk->specular_textures[i]);
		if (mcnk->diffuse_textures[i])
			cache_unref_by_ref_blp(g_wow->cache, mcnk->diffuse_textures[i]);
	}
	mem_free(MEM_GX, mcnk->diffuse_textures);
	mem_free(MEM_GX, mcnk->specular_textures);
	for (uint32_t i = 0; i < GX_MCNK_BATCHES_NB; ++i)
	{
		struct gx_mcnk_batch *batch = &mcnk->batches[i];
#ifdef WITH_DEBUG_RENDERING
		gx_aabb_destroy(&batch->doodads_gx_aabb);
		gx_aabb_destroy(&batch->wmos_gx_aabb);
		gx_aabb_destroy(&batch->gx_aabb);
#endif
		if (batch->doodads_nb && batch->doodads_to_aabb.size)
			jks_array_destroy(&batch->doodads_to_aabb);
		if (batch->wmos_nb && batch->wmos_to_aabb.size)
			jks_array_destroy(&batch->wmos_to_aabb);
		mem_free(MEM_GX, batch->doodads);
		mem_free(MEM_GX, batch->wmos);
		if (batch->ground_effect)
		{
			for (uint32_t j = 0; j < batch->ground_effect->doodads.size; ++j)
				gx_m2_instance_gc(*JKS_ARRAY_GET(&batch->ground_effect->doodads, j, struct gx_m2_instance*));
			jks_array_destroy(&batch->ground_effect->doodads);
			mem_free(MEM_GX, batch->ground_effect);
		}
	}
	gfx_delete_texture(g_wow->device, &mcnk->alpha_texture);
	gfx_delete_buffer(g_wow->device, &mcnk->batches_uniform_buffer);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_delete_buffer(g_wow->device, &mcnk->uniform_buffers[i]);
	gfx_delete_buffer(g_wow->device, &mcnk->vertexes_buffer);
	gfx_delete_buffer(g_wow->device, &mcnk->indices_buffer);
	gfx_delete_attributes_state(g_wow->device, &mcnk->attributes_state);
	clean_init_data(mcnk->init_data);
#ifdef WITH_DEBUG_RENDERING
	gx_aabb_destroy(&mcnk->doodads_gx_aabb);
	gx_aabb_destroy(&mcnk->wmos_gx_aabb);
	gx_aabb_destroy(&mcnk->gx_aabb);
#endif
	mem_free(MEM_GX, mcnk);
}

static void get_interp_points(float px, float pz, uint8_t *points)
{
	float pxf = fmod(px, 1);
	float pzf = fmod(pz, 1);
	if (pxf < .5)
	{
		if (pzf < .5)
		{
			if (pxf > pzf) /* bottom */
			{
				points[0] = 0;
				points[1] = 4;
				points[2] = 1;
			}
			else /* left */
			{
				points[0] = 0;
				points[1] = 4;
				points[2] = 2;
			}
		}
		else
		{
			if (pxf > 1 - pzf) /* top */
			{
				points[0] = 2;
				points[1] = 4;
				points[2] = 3;
			}
			else /* left */
			{
				points[0] = 0;
				points[1] = 4;
				points[2] = 2;
			}
		}
	}
	else
	{
		if (pzf < .5)
		{
			if (1 - pxf > pzf) /* bottom */
			{
				points[0] = 0;
				points[1] = 4;
				points[2] = 1;
			}
			else /* right */
			{
				points[0] = 1;
				points[1] = 4;
				points[2] = 3;
			}
		}
		else
		{
			if (pxf > pzf) /* right */
			{
				points[0] = 1;
				points[1] = 4;
				points[2] = 3;
			}
			else /* top */
			{
				points[0] = 2;
				points[1] = 4;
				points[2] = 3;
			}
		}
	}
}

static void get_interp_factors(float px, float pz, uint8_t *points, float *factors)
{
	float pxf = fmod(px, 1);
	float pzf = fmod(pz, 1);
	static const struct vec2f pos[5] =
	{
		{0, 0},
		{1, 0},
		{0, 1},
		{1, 1},
		{.5, .5},
	};
	const struct vec2f *p1 = &pos[points[0]];
	const struct vec2f *p2 = &pos[points[1]];
	const struct vec2f *p3 = &pos[points[2]];
	float f0n = (p2->y - p3->y) * (pxf - p3->x) + (p3->x - p2->x) * (pzf - p3->y);
	float f0d = (p2->y - p3->y) * (p1->x - p3->x) + (p3->x - p2->x) * (p1->y - p3->y);
	float f1n = (p3->y - p1->y) * (pxf - p3->x) + (p1->x - p3->x) * (pzf - p3->y);
	float f1d = (p2->y - p3->y) * (p1->x - p3->x) + (p3->x - p2->x) * (p1->y - p3->y);
	factors[0] = f0n / f0d;
	factors[1] = f1n / f1d;
	factors[2] = 1 - factors[0] - factors[1];
}

static void get_interp_y(struct gx_mcnk_batch *batch, float px, float pz, uint8_t *points, float *factors, float *y)
{
	static const uint8_t pos[5] = {0, 1, 17, 18, 9};
	uint8_t base = (9 + 8) * (int)pz + (int)px;
	float y0 = batch->height[base + pos[points[0]]];
	float y1 = batch->height[base + pos[points[1]]];
	float y2 = batch->height[base + pos[points[2]]];
	*y = y0 * factors[0] + y1 * factors[1] + y2 * factors[2];
}

static void get_interp_n(struct gx_mcnk_batch *batch, float px, float pz, uint8_t *points, float *factors, int8_t *n)
{
	static const uint8_t pos[5] = {0, 1, 17, 18, 9};
	uint8_t base = (9 + 8) * (int)pz + (int)px;
	int8_t n0[3];
	int8_t n1[3];
	int8_t n2[3];
	memcpy(n0, &batch->norm[base + pos[points[0]]], 3);
	memcpy(n1, &batch->norm[base + pos[points[1]]], 3);
	memcpy(n2, &batch->norm[base + pos[points[2]]], 3);
	for (int i = 0; i < 3; ++i)
		n[i] = n0[i] * factors[0] + n1[i] * factors[1] + n2[i] * factors[2];
}

static bool load_ground_effect_doodads(struct gx_mcnk *mcnk, struct gx_mcnk_batch *batch)
{
	struct gx_mcnk_ground_effect *ground_effect = batch->ground_effect;
	if (!ground_effect)
		return true;
	if (ground_effect->loaded)
		return true;
	ground_effect->loaded = true;
	if (ground_effect->no_effect_doodads == (uint64_t)-1)
		return true;
	uint32_t random_generator = rand();
	float chunk_x = +CHUNK_WIDTH - CHUNK_WIDTH * batch->x;
	float chunk_z = -CHUNK_WIDTH + CHUNK_WIDTH * batch->z;
	for (size_t z = 0; z < 8; ++z)
	{
		for (size_t x = 0; x < 8; ++x)
		{
			uint32_t effect_id = ground_effect->effect_id[z][x];
			if (effect_id == 0xFFFF)
				continue;
			if ((ground_effect->no_effect_doodad[z] >> x) & 1)
				continue;
			if (batch->holes & (1 << (z / 2 * 4 + x / 2)))
				continue;
			struct wow_dbc_row row; /* XXX: use cache for dbc_row of ground_effect_texture & ground_effect_doodad to lower CPU usage */
			if (!dbc_get_row_indexed(g_wow->dbc.ground_effect_texture, &row, effect_id))
				continue;
			uint32_t density = wow_dbc_get_u32(&row, 20);
			if (!density)
				continue;
			for (uint32_t i = 0; i < 4; ++i)
			{
				uint32_t doodad_id = wow_dbc_get_u32(&row, 4 + i * 4);
				if (doodad_id == 0xFFFFFFFF)
					break;
				struct wow_dbc_row doodad_row;
				if (!dbc_get_row_indexed(g_wow->dbc.ground_effect_doodad, &doodad_row, doodad_id))
					continue;
				char path[512];
				snprintf(path, sizeof(path), "World/NoDXT/Detail/%s", wow_dbc_get_str(&doodad_row, 8));
				normalize_m2_filename(path, sizeof(path));
				for (uint32_t n = 0; n < density / 4; ++n)
				{
					struct gx_m2_instance *instance = gx_m2_instance_new_filename(path);
					random_generator = random_generator * 69069 + 1;
					float px = x + random_generator / (float)UINT_MAX;
					random_generator = random_generator * 69069 + 1;
					float pz = z + random_generator / (float)UINT_MAX;
					random_generator = random_generator * 69069 + 1;
					float base_x = chunk_x - pz / 8. * CHUNK_WIDTH - CHUNK_WIDTH;
					float base_z = chunk_z + px / 8. * CHUNK_WIDTH + CHUNK_WIDTH;
					uint8_t points[3];
					float factors[3];
					int8_t no[3];
					float y;
					get_interp_points(px, pz, points);
					get_interp_factors(px, pz, points, factors);
					get_interp_y(batch, px, pz, points, factors, &y);
					get_interp_n(batch, px, pz, points, factors, no);
					struct vec3f pos = {base_x, y, base_z};
					VEC3_ADD(pos, pos, mcnk->pos);
					struct mat4f mat;
					struct mat4f m;
					MAT4_IDENTITY(mat);
					MAT4_TRANSLATE(m, mat, pos);
					MAT4_ROTATEY(float, mat, m, M_PI * (random_generator / (float)UINT_MAX) - .5);
					random_generator = random_generator * 69069 + 1;
#if 0
					struct vec3f center = {0.f, 0.f, 0.f};
					struct vec3f norm = {no[0] / 127., no[1] / 127., no[2] / 127.};
					VEC3_NORMALIZE(float, norm, norm);
					LOG_INFO("x: %f, y: %f, z: %f", norm.x, norm.y, norm.z);
					struct vec3f up = {0.f, -1.f, 0.f};
					struct mat4f off;
					MAT4_IDENTITY(m);
					MAT4_TRANSLATE(off, m, pos);
					MAT4_LOOKAT(float, m, norm, center, up);
					MAT4_MUL(mat, off, m);
#endif
					/* TODO: set light as none, diffuse as ambient. If on shadow, lower lightness */
					/* TVec3<int8_t> tmp = batch.normal[((9 + 8) * z + 9 + x)]; */
					/* Vec3 norm(normalize(Vec3(tmp[0] / 127., tmp[1] / 127., tmp[2] / 127.))); */
					instance->scale = 1000000000; /* bypass the out of range scale */
					gx_m2_instance_set_mat(instance, &mat);
					instance->pos = pos;
					gx_m2_ask_load(instance->parent);
					instance->render_distance_max = (.2 + .8 * random_generator / (float)UINT_MAX) * (GROUND_DOODADS_RANGE_MIN - CHUNK_WIDTH);
					random_generator = random_generator * 69069 + 1;
					if (!jks_array_push_back(&ground_effect->doodads, &instance))
						return false;
				}
			}
		}
	}
	return true;
}

static void unload_ground_effect_doodads(struct gx_mcnk_batch *batch)
{
	struct gx_mcnk_ground_effect *ground_effect = batch->ground_effect;
	if (!ground_effect)
		return;
	if (!ground_effect->loaded)
		return;
	for (uint32_t i = 0; i < ground_effect->doodads.size; ++i)
		gx_m2_instance_gc(*JKS_ARRAY_GET(&ground_effect->doodads, i, struct gx_m2_instance*));
	jks_array_resize(&ground_effect->doodads, 0);
	ground_effect->loaded = false;
}

bool gx_mcnk_link_objects(struct gx_mcnk *mcnk)
{
	for (uint32_t i = 0; i < GX_MCNK_BATCHES_NB; ++i)
	{
		struct gx_mcnk_batch *batch = &mcnk->batches[i];
		batch->doodads_instances = mem_malloc(MEM_GX, sizeof(*batch->doodads_instances) * batch->doodads_nb);
		if (!batch->doodads_instances)
			return false;
		for (uint32_t j = 0; j < batch->doodads_nb; ++j)
			batch->doodads_instances[j] = mcnk->parent->m2[batch->doodads[j]]->instance;
	}
	return true;
}

int gx_mcnk_initialize(struct gx_mcnk *mcnk)
{
	if (mcnk->initialized)
		return 1;
	if (!mcnk->alpha_texture.handle.u64)
	{
		gfx_create_texture(g_wow->device, &mcnk->alpha_texture, GFX_TEXTURE_2D_ARRAY, (g_wow->map->flags & WOW_MPHD_FLAG_BIG_ALPHA) ? GFX_R8G8B8A8 : GFX_R4G4B4A4, 1, 64, 64, 256);
		gfx_set_texture_filtering(&mcnk->alpha_texture, GFX_FILTERING_LINEAR, GFX_FILTERING_LINEAR, GFX_FILTERING_NONE);
		gfx_set_texture_anisotropy(&mcnk->alpha_texture, 16);
		gfx_set_texture_addressing(&mcnk->alpha_texture, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP);
		gfx_set_texture_levels(&mcnk->alpha_texture, 0, 0);
	}
	for (uint32_t i = 0; i < GX_MCNK_BATCHES_NB; ++i)
	{
		if (!mcnk->init_data->alpha[i])
			continue;
		size_t alpha_bpp = (g_wow->map->flags & WOW_MPHD_FLAG_BIG_ALPHA) ? 4 : 2;
		gfx_set_texture_data(&mcnk->alpha_texture, 0, i, 64, 64, 1, 64 * 64 * alpha_bpp, mcnk->init_data->alpha[i]);
		mem_free(MEM_GX, mcnk->init_data->alpha[i]);
		mcnk->init_data->alpha[i] = NULL;
		return 0;
	}
	gfx_create_buffer(g_wow->device, &mcnk->vertexes_buffer, GFX_BUFFER_VERTEXES, mcnk->init_data->vertexes, mcnk->init_data->vertexes_nb * sizeof(*mcnk->init_data->vertexes), GFX_BUFFER_IMMUTABLE);
	if (mcnk->holes)
		gfx_create_buffer(g_wow->device, &mcnk->indices_buffer, GFX_BUFFER_INDICES, mcnk->init_data->indices, mcnk->init_data->indices_nb * sizeof(*mcnk->init_data->indices), GFX_BUFFER_IMMUTABLE);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_create_buffer(g_wow->device, &mcnk->uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_mcnk_model_block), GFX_BUFFER_STREAM);
	{
		mcnk->batch_uniform_padded_size = (sizeof(struct shader_mcnk_mesh_block));
		uint32_t batches_ubo_size = mcnk->batch_uniform_padded_size * 256;
		uint8_t *batches_ubo_data = mem_malloc(MEM_GX, batches_ubo_size);
		if (!batches_ubo_data)
		{
			LOG_ERROR("allocation failed");
			return -1;
		}
		for (uint32_t i = 0; i < 256; ++i)
		{
			struct shader_mcnk_mesh_block *mesh_block = (struct shader_mcnk_mesh_block*)(batches_ubo_data + mcnk->batch_uniform_padded_size * i);
			for (size_t j = 0; j < 4; ++j)
				mesh_block->uv_offsets[j] = mcnk->batches[i].layers_animations[j];
		}
		gfx_create_buffer(g_wow->device, &mcnk->batches_uniform_buffer, GFX_BUFFER_UNIFORM, batches_ubo_data, batches_ubo_size, GFX_BUFFER_IMMUTABLE);
		mem_free(MEM_GX, batches_ubo_data);
	}
	clean_init_data(mcnk->init_data);
	mcnk->init_data = NULL;
	const struct gfx_attribute_bind binds[] =
	{
		{&mcnk->vertexes_buffer           , sizeof(struct shader_mcnk_input), offsetof(struct shader_mcnk_input, norm)},
		{&g_wow->map->mcnk_vertexes_buffer, sizeof(struct vec2f) * 2        , sizeof(struct vec2f) * 0},
		{&g_wow->map->mcnk_vertexes_buffer, sizeof(struct vec2f) * 2        , sizeof(struct vec2f) * 1},
		{&mcnk->vertexes_buffer           , sizeof(struct shader_mcnk_input), offsetof(struct shader_mcnk_input, y)},
	};
	gfx_create_attributes_state(g_wow->device, &mcnk->attributes_state, binds, sizeof(binds) / sizeof(*binds), mcnk->holes ? &mcnk->indices_buffer : &g_wow->map->mcnk_indices_buffer, GFX_INDEX_UINT16);
	mcnk->initialized = true;
	return 1;
}

void gx_mcnk_cull(struct gx_mcnk *mcnk)
{
	if (!mcnk->initialized)
		return;
	struct vec3f delta;
	VEC3_SUB(delta, mcnk->center, g_wow->cull_frame->cull_pos);
	mcnk->distance_to_camera = sqrt(delta.x * delta.x + delta.z * delta.z);
	if (mcnk->distance_to_camera > g_wow->cull_frame->view_distance * 1.4142) /* sqrt(2) is the worst case of frustum plane */
		return;
	if (!frustum_check_fast(&g_wow->cull_frame->frustum, &mcnk->objects_aabb))
		return;
	render_add_mcnk_objects(mcnk);
	enum frustum_result result = frustum_check(&g_wow->cull_frame->frustum, &mcnk->aabb);
	MAT4_TRANSLATE(mcnk->render_frames[g_wow->cull_frame_id].mv, g_wow->cull_frame->view_v, mcnk->pos);
	MAT4_MUL(mcnk->render_frames[g_wow->cull_frame_id].mvp, g_wow->cull_frame->view_p, mcnk->render_frames[g_wow->cull_frame_id].mv);
	render_add_mcnk(mcnk);
	mcnk->render_frames[g_wow->cull_frame_id].to_render_nb = 0;
	uint8_t doodads_batches[256];
	size_t doodads_batches_count = 0;
	uint8_t wmos_batches[256];
	size_t wmos_batches_count = 0;
	for (size_t i = 0; i < GX_MCNK_BATCHES_NB; ++i)
	{
		struct gx_mcnk_batch *batch = &mcnk->batches[i];
		VEC3_SUB(delta, batch->center, g_wow->cull_frame->cull_pos);
		batch->render_frames[g_wow->cull_frame_id].distance_to_camera = sqrt(delta.x * delta.x + delta.z * delta.z);
		if (batch->render_frames[g_wow->cull_frame_id].distance_to_camera > g_wow->cull_frame->view_distance * 1.4142)
		{
			batch->render_frames[g_wow->cull_frame_id].culled = true;
			continue;
		}
		if (g_wow->wow_opt & WOW_OPT_AABB_OPTIMIZE)
		{
			for (size_t j = 0; j < batch->doodads_to_aabb.size; ++j)
			{
				struct gx_m2_instance *m2 = mcnk->parent->m2[*(uint32_t*)jks_array_get(&batch->doodads_to_aabb, j)]->instance;
				if (!m2->parent->loaded)
					continue;
				if (batch->doodads_to_aabb.size == batch->doodads_nb)
					aabb_set_min_max(&batch->doodads_aabb, &m2->aabb);
				else
					aabb_add_min_max(&batch->doodads_aabb, &m2->aabb);
				aabb_sub_min_max(&batch->doodads_aabb, &batch->objects_aabb); /* avoid aabb from going outside of chunk */
				if (!mcnk->has_doodads_aabb)
				{
					aabb_set_min_max(&mcnk->doodads_aabb, &batch->doodads_aabb);
					mcnk->has_doodads_aabb = true;
				}
				else
				{
					aabb_add_min_max(&mcnk->doodads_aabb, &batch->doodads_aabb);
				}
#ifdef WITH_DEBUG_RENDERING
				mcnk->doodads_gx_aabb.aabb = mcnk->doodads_aabb;
				mcnk->doodads_gx_aabb.flags |= GX_AABB_DIRTY;
				batch->doodads_gx_aabb.aabb = batch->doodads_aabb;
				batch->doodads_gx_aabb.flags |= GX_AABB_DIRTY;
#endif
				jks_array_erase(&batch->doodads_to_aabb, j);
				j--;
				if (!batch->doodads_to_aabb.size)
					jks_array_destroy(&batch->doodads_to_aabb);
			}
			for (size_t j = 0; j < batch->wmos_to_aabb.size; ++j)
			{
				struct gx_wmo_instance *wmo = mcnk->parent->wmo[*(uint32_t*)jks_array_get(&batch->wmos_to_aabb, j)]->instance;
				if (!wmo->parent->loaded)
					continue;
				if (batch->wmos_to_aabb.size == batch->wmos_nb)
					aabb_set_min_max(&batch->wmos_aabb, &wmo->aabb);
				else
					aabb_add_min_max(&batch->wmos_aabb, &wmo->aabb);
				aabb_sub_min_max(&batch->wmos_aabb, &batch->objects_aabb); /* avoid aabb from going outside of chunk */
				if (!mcnk->has_wmos_aabb)
				{
					aabb_set_min_max(&mcnk->wmos_aabb, &batch->wmos_aabb);
					mcnk->has_wmos_aabb = true;
				}
				else
				{
					aabb_add_min_max(&mcnk->wmos_aabb, &batch->wmos_aabb);
				}
#ifdef WITH_DEBUG_RENDERING
				mcnk->wmos_gx_aabb.aabb = mcnk->wmos_aabb;
				mcnk->wmos_gx_aabb.flags |= GX_AABB_DIRTY;
				batch->wmos_gx_aabb.aabb = batch->wmos_aabb;
				batch->wmos_gx_aabb.flags |= GX_AABB_DIRTY;
#endif
				jks_array_erase(&batch->wmos_to_aabb, j);
				j--;
				if (!batch->wmos_to_aabb.size)
					jks_array_destroy(&batch->wmos_to_aabb);
			}
			if (batch->doodads_to_aabb.size == batch->doodads_nb)
				batch->doodads_frustum_result = FRUSTUM_OUTSIDE;
			else if (batch->doodads_nb < 3) /* threshold where multiplying frustums harms */
				batch->doodads_frustum_result = FRUSTUM_COLLIDE;
			else
				doodads_batches[doodads_batches_count++] = i;
			if (batch->wmos_to_aabb.size == batch->wmos_nb)
				batch->wmos_frustum_result = FRUSTUM_OUTSIDE;
			else if (batch->wmos_nb < 3) /* threshold where multiplying frustums harms */
				batch->wmos_frustum_result = FRUSTUM_COLLIDE;
			else
				wmos_batches[wmos_batches_count++] = i;
		}
		switch (result)
		{
			case FRUSTUM_INSIDE:
				batch->frustum_result = FRUSTUM_INSIDE;
				batch->render_frames[g_wow->cull_frame_id].culled = false;
				mcnk->render_frames[g_wow->cull_frame_id].to_render[mcnk->render_frames[g_wow->cull_frame_id].to_render_nb++] = i;
				break;
			case FRUSTUM_OUTSIDE:
				batch->render_frames[g_wow->cull_frame_id].culled = true;
				break;
			case FRUSTUM_COLLIDE:
				batch->frustum_result = frustum_check(&g_wow->cull_frame->frustum, &batch->aabb);
				if (!(batch->render_frames[g_wow->cull_frame_id].culled = !batch->frustum_result))
					mcnk->render_frames[g_wow->cull_frame_id].to_render[mcnk->render_frames[g_wow->cull_frame_id].to_render_nb++] = i;
				break;
		}
	}
	if (g_wow->wow_opt & WOW_OPT_AABB_OPTIMIZE)
	{
		if (mcnk->has_doodads_aabb)
		{
			mcnk->doodads_frustum_result = frustum_check(&g_wow->cull_frame->frustum, &mcnk->doodads_aabb);
			if (mcnk->doodads_frustum_result == FRUSTUM_COLLIDE)
			{
				for (size_t i = 0; i < doodads_batches_count; ++i)
				{
					struct gx_mcnk_batch *batch = &mcnk->batches[doodads_batches[i]];
					batch->doodads_frustum_result = frustum_check(&g_wow->cull_frame->frustum, &batch->doodads_aabb);
				}
			}
		}
		else
		{
			mcnk->doodads_frustum_result = FRUSTUM_OUTSIDE;
		}
		if (mcnk->has_wmos_aabb)
		{
			mcnk->wmos_frustum_result = frustum_check(&g_wow->cull_frame->frustum, &mcnk->wmos_aabb);
			if (mcnk->wmos_frustum_result == FRUSTUM_COLLIDE)
			{
				for (size_t i = 0; i < wmos_batches_count; ++i)
				{
					struct gx_mcnk_batch *batch = &mcnk->batches[wmos_batches[i]];
					batch->wmos_frustum_result = frustum_check(&g_wow->cull_frame->frustum, &batch->wmos_aabb);
				}
			}
		}
		else
		{
			mcnk->wmos_frustum_result = FRUSTUM_OUTSIDE;
		}
	}
}

void gx_mcnk_add_objects_to_render(struct gx_mcnk *mcnk)
{
	if (!mcnk->initialized)
		return;
	for (uint32_t i = 0; i < GX_MCNK_BATCHES_NB; ++i)
	{
		struct gx_mcnk_batch *batch = &mcnk->batches[i];
		if (batch->render_frames[g_wow->cull_frame_id].distance_to_camera > GROUND_DOODADS_RANGE_MAX)
			unload_ground_effect_doodads(batch);
		if (batch->render_frames[g_wow->cull_frame_id].distance_to_camera < GROUND_DOODADS_RANGE_MIN)
		{
			if (!load_ground_effect_doodads(mcnk, batch))
				LOG_ERROR("failed to load ground doodads");
		}
		if (batch->render_frames[g_wow->cull_frame_id].distance_to_camera > g_wow->cull_frame->view_distance + (CHUNK_WIDTH * 1.4142))
			continue;
		if (!batch->render_frames[g_wow->cull_frame_id].culled && batch->ground_effect)
		{
			for (uint32_t doodad = 0; doodad < batch->ground_effect->doodads.size; ++doodad)
			{
				struct gx_m2_instance *instance = *(struct gx_m2_instance**)jks_array_get(&batch->ground_effect->doodads, doodad);
				struct vec3f delta;
				VEC3_SUB(delta, g_wow->cull_frame->view_pos, instance->pos);
				if (sqrt(delta.x * delta.x + delta.z * delta.z) < CHUNK_WIDTH * 4)
					gx_m2_instance_add_to_render(instance, true, &g_wow->cull_frame->m2_params);/* batch.frustumResult == FRUSTUM_INSIDE); */
			}
		}
		if (g_wow->wow_opt & WOW_OPT_AABB_OPTIMIZE)
		{
			if (batch->doodads_nb && mcnk->doodads_frustum_result != FRUSTUM_OUTSIDE)
			{
				if (mcnk->doodads_frustum_result == FRUSTUM_INSIDE || batch->doodads_frustum_result != FRUSTUM_OUTSIDE)
				{
					PERFORMANCE_BEGIN(M2_CULL);
					bool bypass = mcnk->doodads_frustum_result == FRUSTUM_INSIDE || batch->doodads_frustum_result == FRUSTUM_INSIDE;
					for (uint32_t doodad = 0; doodad < batch->doodads_nb; ++doodad)
						gx_m2_instance_add_to_render(mcnk->parent->m2[batch->doodads[doodad]]->instance, bypass, &g_wow->cull_frame->m2_params);
					PERFORMANCE_END(M2_CULL);
				}
			}
			if (batch->wmos_nb && mcnk->wmos_frustum_result != FRUSTUM_OUTSIDE)
			{
				if (mcnk->wmos_frustum_result == FRUSTUM_INSIDE || batch->wmos_frustum_result != FRUSTUM_OUTSIDE)
				{
					PERFORMANCE_BEGIN(WMO_CULL);
					bool bypass = mcnk->wmos_frustum_result == FRUSTUM_INSIDE || batch->wmos_frustum_result == FRUSTUM_INSIDE;
					for (uint32_t wmo = 0; wmo < batch->wmos_nb; ++wmo)
						gx_wmo_instance_add_to_render(mcnk->parent->wmo[batch->wmos[wmo]]->instance, bypass);
					PERFORMANCE_END(WMO_CULL);
				}
			}
		}
		else
		{
			for (uint32_t doodad = 0; doodad < batch->doodads_nb; ++doodad)
				gx_m2_instance_add_to_render(mcnk->parent->m2[batch->doodads[doodad]]->instance, false, &g_wow->cull_frame->m2_params);
			for (uint32_t wmo = 0; wmo < batch->wmos_nb; ++wmo)
				gx_wmo_instance_add_to_render(mcnk->parent->wmo[batch->wmos[wmo]]->instance, false);
		}
	}
}

void gx_mcnk_render(struct gx_mcnk *mcnk)
{
	if (!mcnk->initialized)
		return;
	if (!mcnk->render_frames[g_wow->draw_frame_id].to_render_nb)
		return;
	PERFORMANCE_BEGIN(MCNK_RENDER_DATA);
	struct shader_mcnk_model_block model_block;
	model_block.v = *(struct mat4f*)&g_wow->draw_frame->view_v;
	model_block.mv = mcnk->render_frames[g_wow->draw_frame_id].mv;
	model_block.mvp = mcnk->render_frames[g_wow->draw_frame_id].mvp;
	model_block.offset_time = g_wow->frametime / 333000000000.;
	gfx_set_buffer_data(&mcnk->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
	PERFORMANCE_END(MCNK_RENDER_DATA);
	{
		PERFORMANCE_BEGIN(MCNK_RENDER_BIND);
		gfx_bind_constant(g_wow->device, 1, &mcnk->uniform_buffers[g_wow->draw_frame_id], sizeof(struct shader_mcnk_model_block), 0);
		gfx_bind_attributes_state(g_wow->device, &mcnk->attributes_state, &g_wow->graphics->mcnk_input_layout);
		gfx_bind_constant(g_wow->device, 0, &mcnk->batches_uniform_buffer, sizeof(struct shader_mcnk_mesh_block) * 256, 0);
		PERFORMANCE_END(MCNK_RENDER_BIND);
	}
	for (size_t i = 0; i < mcnk->render_frames[g_wow->draw_frame_id].to_render_nb; ++i)
	{
		const gfx_texture_t *textures[9];
		struct gx_mcnk_batch *batch = &mcnk->batches[mcnk->render_frames[g_wow->draw_frame_id].to_render[i]];
		uint32_t indices_buffer;
		PERFORMANCE_BEGIN(MCNK_RENDER_BIND);
		if (batch->render_frames[g_wow->draw_frame_id].distance_to_camera < CHUNK_WIDTH * 16)
			indices_buffer = 0;
		else if (batch->render_frames[g_wow->draw_frame_id].distance_to_camera < CHUNK_WIDTH * 32)
			indices_buffer = 1;
		else
			indices_buffer = 2;
		if (!batch->indices_nbs[indices_buffer])
			continue;
		textures[0] = &mcnk->alpha_texture;
		size_t n = 1;
		for (size_t j = 0; j < 4; ++j)
		{
			uint32_t texture_index = batch->textures[j];
			if (texture_index >= mcnk->textures_nb)
			{
				textures[n++] = NULL;
				textures[n++] = NULL;
			}
			else
			{
				if (mcnk->diffuse_textures[texture_index]->initialized)
					textures[n++] = &mcnk->diffuse_textures[texture_index]->texture;
				else
					textures[n++] = NULL;
				if (mcnk->specular_textures[texture_index]->initialized)
					textures[n++] = &mcnk->specular_textures[texture_index]->texture;
				else
					textures[n++] = NULL;
			}
		}
		gfx_bind_samplers(g_wow->device, 0, 9, textures);
		PERFORMANCE_END(MCNK_RENDER_BIND);
		PERFORMANCE_BEGIN(MCNK_RENDER_DRAW);
		gfx_draw_indexed(g_wow->device, batch->indices_nbs[indices_buffer], batch->indices_offsets[indices_buffer]);
		PERFORMANCE_END(MCNK_RENDER_DRAW);
	}
}

#ifdef WITH_DEBUG_RENDERING
void gx_mcnk_render_aabb(struct gx_mcnk *mcnk)
{
	if (!mcnk->initialized)
		return;
	if (mcnk->has_doodads_aabb)
	{
		switch (mcnk->doodads_frustum_result)
		{
			case FRUSTUM_OUTSIDE:
				mcnk->doodads_gx_aabb.color = (struct vec4f){1, 0, 0, 1};
				break;
			case FRUSTUM_COLLIDE:
				mcnk->doodads_gx_aabb.color = (struct vec4f){1, .5, 0, 1};
				break;
			case FRUSTUM_INSIDE:
				mcnk->doodads_gx_aabb.color = (struct vec4f){0, 1, 0, 1};
				break;
		}
		gx_aabb_update(&mcnk->doodads_gx_aabb, &g_wow->draw_frame->view_vp);
		gx_aabb_render(&mcnk->doodads_gx_aabb);
	}
	if (mcnk->has_wmos_aabb)
	{
		gx_aabb_update(&mcnk->wmos_gx_aabb, &g_wow->draw_frame->view_vp);
		gx_aabb_render(&mcnk->wmos_gx_aabb);
	}
	gx_aabb_update(&mcnk->gx_aabb, &g_wow->draw_frame->view_vp);
	gx_aabb_render(&mcnk->gx_aabb);
	for (uint32_t i = 0; i < GX_MCNK_BATCHES_NB; ++i)
	{
		struct gx_mcnk_batch *batch = &mcnk->batches[i];
		if (batch->doodads_to_aabb.size != batch->doodads_nb)
		{
			switch (batch->doodads_frustum_result)
			{
				case FRUSTUM_OUTSIDE:
					batch->doodads_gx_aabb.color = (struct vec4f){1, 0, 0, 1};
					break;
				case FRUSTUM_COLLIDE:
					batch->doodads_gx_aabb.color = (struct vec4f){1, .5, 0, 1};
					break;
				case FRUSTUM_INSIDE:
					batch->doodads_gx_aabb.color = (struct vec4f){0, 1, 0, 1};
					break;
			}
			gx_aabb_update(&batch->doodads_gx_aabb, &g_wow->draw_frame->view_vp);
			gx_aabb_render(&batch->doodads_gx_aabb);
		}
		if (batch->wmos_to_aabb.size != batch->wmos_nb)
		{
			gx_aabb_update(&batch->wmos_gx_aabb, &g_wow->draw_frame->view_vp);
			gx_aabb_render(&batch->wmos_gx_aabb);
		}
		if (batch->render_frames[g_wow->draw_frame_id].culled)
			continue;
		gx_aabb_update(&batch->gx_aabb, &g_wow->draw_frame->view_vp);
		gx_aabb_render(&batch->gx_aabb);
	}
}
#endif
