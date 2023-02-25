#include "gx/wmo_group.h"
#include "gx/frame.h"
#include "gx/mcnk.h"
#include "gx/mclq.h"
#include "gx/wmo.h"
#include "gx/m2.h"

#include "map/tile.h"
#include "map/map.h"

#include "loader.h"
#include "memory.h"
#include "const.h"
#include "cache.h"
#include "log.h"
#include "wow.h"

#include <libwow/wmo_group.h>
#include <libwow/adt.h>

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

struct map_tile *map_tile_new(const char *filename, int32_t x, int32_t z)
{
	struct map_tile *tile = mem_zalloc(MEM_MAP, sizeof(*tile));
	if (!tile)
		return NULL;
	tile->x = x;
	tile->z = z;
	tile->filename = mem_strdup(MEM_MAP, filename);
	VEC3_SET(tile->pos, -(z - 32) * CHUNK_WIDTH * 16 - 500, 0, (x - 32) * CHUNK_WIDTH * 16 + 500);
	if (!tile->filename)
		goto err;
	return tile;

err:
	mem_free(MEM_GX, tile);
	return NULL;
}

void map_tile_delete(struct map_tile *tile)
{
	if (!tile)
		return;
	for (size_t i = 0; i < 256; ++i)
	{
		struct map_chunk *chunk = &tile->chunks[i];
		mem_free(MEM_GX, chunk->doodads);
		mem_free(MEM_GX, chunk->wmos);
	}
	gx_mclq_delete(tile->gx_mclq);
	gx_mcnk_delete(tile->gx_mcnk);
	mem_free(MEM_MAP, tile->filename);
	mem_free(MEM_MAP, tile);
}

void map_tile_ask_load(struct map_tile *tile)
{
	if (tile->flags & (MAP_TILE_LOADING | MAP_TILE_LOADED))
		return;
	tile->flags |= MAP_TILE_LOADING;
	loader_push(g_wow->loader, ASYNC_TASK_MAP_TILE_LOAD, tile);
}

void map_tile_ask_unload(struct map_tile *tile)
{
	tile->flags &= ~MAP_TILE_LOADING;
	loader_gc_map_tile(g_wow->loader, tile);
	for (uint32_t i = 0; i < tile->wmo_nb; ++i)
		cache_unref_by_ref_map_wmo(g_wow->cache, tile->wmo[i]);
	for (uint32_t i = 0; i < tile->m2_nb; ++i)
		cache_unref_by_ref_map_m2(g_wow->cache, tile->m2[i]);
	mem_free(MEM_GX, tile->wmo);
	tile->wmo = NULL;
	tile->wmo_nb = 0;
	mem_free(MEM_GX, tile->m2);
	tile->m2 = NULL;
	tile->m2_nb = 0;
}

bool map_tile_load(struct map_tile *tile, struct wow_adt_file *file)
{
	if (!(tile->flags & MAP_TILE_LOADING) || (tile->flags & MAP_TILE_LOADED))
		return true;
	float tile_min_y = +9999;
	float tile_max_y = -9999;
	tile->pos = (struct vec3f){file->mcnk[0].header.position.x, 0, -file->mcnk[0].header.position.y};
	for (size_t i = 0; i < 256; ++i)
	{
		struct wow_mcnk *wow_mcnk = &file->mcnk[i];
		struct map_chunk *chunk = &tile->chunks[i];
		VEC3_SETV(chunk->doodads_aabb.p0, -INFINITY);
		VEC3_SETV(chunk->doodads_aabb.p1, -INFINITY);
		VEC3_SETV(chunk->wmos_aabb.p0, -INFINITY);
		VEC3_SETV(chunk->wmos_aabb.p1, -INFINITY);
		chunk->holes = wow_mcnk->header.holes;
		chunk->doodads_nb = wow_mcnk->mcrf.doodads_nb;
		if (chunk->doodads_nb)
		{
			chunk->doodads = mem_malloc(MEM_GX, sizeof(*chunk->doodads) * chunk->doodads_nb);
			if (!chunk->doodads)
				return false;
			memcpy(chunk->doodads, wow_mcnk->mcrf.doodads, sizeof(*chunk->doodads) * chunk->doodads_nb);
		}
		chunk->wmos_nb = wow_mcnk->mcrf.wmos_nb;
		if (chunk->wmos_nb)
		{
			chunk->wmos = mem_malloc(MEM_GX, sizeof(*chunk->wmos) * chunk->wmos_nb);
			if (!chunk->wmos)
				return false;
			memcpy(chunk->wmos, wow_mcnk->mcrf.wmos, sizeof(*chunk->wmos) * chunk->wmos_nb);
		}
		float min_y = +999999;
		float max_y = -999999;
		for (size_t j = 0; j < 9 * 9 + 8 * 8; ++j)
		{
			float y = wow_mcnk->mcvt.height[j] + wow_mcnk->header.position.z;
			if (y > max_y)
				max_y = y;
			if (y < min_y)
				min_y = y;
			chunk->height[j] = y;
			chunk->norm[j * 3 + 0] =  wow_mcnk->mcnr.normal[j * 3 + 0];
			chunk->norm[j * 3 + 1] =  wow_mcnk->mcnr.normal[j * 3 + 2];
			chunk->norm[j * 3 + 2] = -wow_mcnk->mcnr.normal[j * 3 + 1];
		}
		if (max_y > tile_max_y)
			tile_max_y= max_y;
		if (min_y < tile_min_y)
			tile_min_y = min_y;
		float base_x = -CHUNK_WIDTH * (i / 16);
		float base_z = +CHUNK_WIDTH * (i % 16);
		struct vec3f p0;
		struct vec3f p1;
		VEC3_SET(p0, base_x, min_y, base_z);
		VEC3_ADD(p0, p0, tile->pos);
		VEC3_SET(p1, base_x - CHUNK_WIDTH, max_y, base_z + CHUNK_WIDTH);
		VEC3_ADD(p1, p1, tile->pos);
		VEC3_MIN(chunk->aabb.p0, p0, p1);
		VEC3_MAX(chunk->aabb.p1, p0, p1);
		VEC3_ADD(chunk->center, chunk->aabb.p0, chunk->aabb.p1);
		VEC3_MULV(chunk->center, chunk->center, .5f);
		p0.y = -999999;
		p1.y = +999999;
		VEC3_MIN(chunk->objects_aabb.p0, p0, p1);
		VEC3_MAX(chunk->objects_aabb.p1, p0, p1);
	}
	struct vec3f p0;
	struct vec3f p1;
	VEC3_SET(p0, 0, tile_min_y, 0);
	VEC3_ADD(p0, p0, tile->pos);
	VEC3_SET(p1, -CHUNK_WIDTH * 16, tile_max_y, CHUNK_WIDTH * 16);
	VEC3_ADD(p1, p1, tile->pos);
	VEC3_MIN(tile->aabb.p0, p0, p1);
	VEC3_MAX(tile->aabb.p1, p0, p1);
	p0.y = -9999;
	p1.y = +9999;
	VEC3_MIN(tile->objects_aabb.p0, p0, p1);
	VEC3_MAX(tile->objects_aabb.p1, p0, p1);
	VEC3_ADD(tile->center, tile->aabb.p0, tile->aabb.p1);
	VEC3_MULV(tile->center, tile->center, .5);
	tile->gx_mcnk = gx_mcnk_new(tile, file);
	if (!tile->gx_mcnk)
	{
		tile->flags |= MAP_TILE_LOADED;
		LOG_ERROR("failed to load mcnk");
		return true;
	}
	{
		bool has_liquids = false;
		for (uint32_t i = 0; i < sizeof(file->mcnk) / sizeof(*file->mcnk); ++i)
		{
			struct wow_mcnk *mcnk = &file->mcnk[i];
			if (mcnk->header.size_mclq <= 8 || memcmp("QLCM", (uint8_t*)&mcnk->mclq.header.magic, 4))
				continue;
			has_liquids = true;
			break;
		}
		if (has_liquids)
		{
			tile->gx_mclq = gx_mclq_new(tile, file);
			if (!tile->gx_mclq)
			{
				tile->flags |= MAP_TILE_LOADED;
				LOG_ERROR("failed to load mclq");
				return true;
			}
		}
	}
	tile->flags |= MAP_TILE_LOADED;
	if (!loader_init_map_tile(g_wow->loader, tile))
		return false;
	return true;
}

bool map_tile_load_childs(struct map_tile *tile, struct wow_adt_file *file)
{
	uint32_t wmo_nb = file->modf.data_nb;
	struct map_wmo **wmo;
	uint32_t m2_nb = file->mddf.data_nb;
	struct map_m2 **m2;
	wmo = mem_malloc(MEM_GX, sizeof(*wmo) * wmo_nb);
	if (!wmo)
		goto err1;
	m2 = mem_malloc(MEM_GX, sizeof(*m2) * m2_nb);
	if (!m2)
		goto err2;
	for (uint32_t i = 0; i < wmo_nb; ++i)
	{
		struct wow_modf_data *modf = &file->modf.data[i];
		struct map_wmo *handle;
		cache_lock_map_wmo(g_wow->cache);
		if (!cache_ref_by_key_unmutexed_map_wmo(g_wow->cache, modf->unique_id, &handle))
		{
			cache_unlock_map_wmo(g_wow->cache);
			LOG_ERROR("failed to get wmo handle: %d", modf->unique_id);
			wmo[i] = NULL;
			continue;
		}
		if (!handle->instance)
		{
			char filename[512];
			snprintf(filename, sizeof(filename), "%s", &file->mwmo.data[file->mwid.data[modf->name_id]]);
			normalize_wmo_filename(filename, sizeof(filename));
			map_wmo_load(handle, filename);
			cache_unlock_map_wmo(g_wow->cache);
			float offset = (32 * 16) * CHUNK_WIDTH;
			struct vec3f pos = {offset - modf->position.z, modf->position.y, -(offset - modf->position.x)};
			struct mat4f mat1;
			struct mat4f mat2;
			MAT4_IDENTITY(mat1);
			MAT4_TRANSLATE(mat2, mat1, pos);
			MAT4_ROTATEY(float, mat1, mat2,  (modf->rotation.y + 180) / 180. * M_PI);
			MAT4_ROTATEZ(float, mat2, mat1, -(modf->rotation.x) / 180. * M_PI);
			MAT4_ROTATEX(float, mat1, mat2,  (modf->rotation.z) / 180. * M_PI);
			cache_lock_wmo(g_wow->cache);
			gx_wmo_instance_set_mat(handle->instance, &mat1);
			handle->instance->pos = pos;
			handle->instance->doodad_set = modf->doodad_set;
			if (handle->instance->parent->loaded)
				gx_wmo_instance_on_load(handle->instance);
			else
				gx_wmo_ask_load(handle->instance->parent);
			cache_unlock_wmo(g_wow->cache);
		}
		else
		{
			cache_unlock_map_wmo(g_wow->cache);
		}
		wmo[i] = handle;
	}
	for (uint32_t i = 0; i < m2_nb; ++i)
	{
		struct wow_mddf_data *mddf = &file->mddf.data[i];
		struct map_m2 *handle;
		cache_lock_map_m2(g_wow->cache);
		if (!cache_ref_by_key_unmutexed_map_m2(g_wow->cache, mddf->unique_id, &handle))
		{
			cache_unlock_map_m2(g_wow->cache);
			LOG_ERROR("failed to get m2 handle: %d", mddf->unique_id);
			m2[i] = NULL;
			continue;
		}
		if (!handle->instance)
		{
			char filename[512];
			snprintf(filename, sizeof(filename), "%s", &file->mmdx.data[file->mmid.data[mddf->name_id]]);
			normalize_m2_filename(filename, sizeof(filename));
			map_m2_load(handle, filename);
			cache_unlock_map_m2(g_wow->cache);
			float offset = (32 * 16) * CHUNK_WIDTH;
			struct vec3f pos = {offset - mddf->position.z, mddf->position.y, -(offset - mddf->position.x)};
			struct mat4f mat1;
			struct mat4f mat2;
			MAT4_IDENTITY(mat1);
			MAT4_TRANSLATE(mat2, mat1, pos);
			MAT4_ROTATEY(float, mat1, mat2,  (mddf->rotation.y + 180) / 180. * M_PI);
			MAT4_ROTATEZ(float, mat2, mat1, -(mddf->rotation.x) / 180. * M_PI);
			MAT4_ROTATEX(float, mat1, mat2,  (mddf->rotation.z) / 180. * M_PI);
			MAT4_SCALEV(mat1, mat1, mddf->scale / 1024.);
			cache_lock_m2(g_wow->cache);
			handle->instance->scale = mddf->scale / 1024.;
			gx_m2_instance_set_mat(handle->instance, &mat1);
			handle->instance->pos = pos;
			gx_m2_ask_load(handle->instance->parent);
			cache_unlock_m2(g_wow->cache);
		}
		else
		{
			cache_unlock_map_m2(g_wow->cache);
		}
		m2[i] = handle;
	}
	tile->wmo_nb = wmo_nb;
	tile->wmo = wmo;
	tile->m2_nb = m2_nb;
	tile->m2 = m2;
	return true;

err2:
	mem_free(MEM_GX, wmo);
err1:
	return false;
}

int map_tile_initialize(struct map_tile *tile)
{
	assert(tile->flags & MAP_TILE_LOADED);
	if (tile->flags & MAP_TILE_INIT)
		return 1;
	switch (gx_mcnk_initialize(tile->gx_mcnk))
	{
		case -1:
			return -1;
		case 0:
			return 0;
		case 1:
			break;
	}
	if (tile->gx_mclq)
	{
		switch (gx_mclq_initialize(tile->gx_mclq))
		{
			case -1:
				return -1;
			case 0:
				return 0;
			case 1:
				break;
		}
	}
	tile->flags |= MAP_TILE_INIT;
	return true;
}

void map_tile_cull(struct map_tile *tile)
{
	if (!(tile->flags & MAP_TILE_INIT))
		return;
	gx_mcnk_cull(tile->gx_mcnk);
	if (tile->gx_mclq)
		gx_mclq_cull(tile->gx_mclq);
}

static void add_point(struct map_tile *tile, struct map_chunk *chunk, uint8_t chunk_id, uint16_t idx, struct vec3f *p)
{
	size_t y = idx % 17;
	float z = idx / 17 * 2;
	float x;
	if (y < 9)
	{
		x = y * 2;
	}
	else
	{
		z++;
		x = (y - 9) * 2 + 1;
	}
	p->x = tile->pos.x + (-1 - (ssize_t)(chunk_id / 16) + (16 - z) / 16.f) * CHUNK_WIDTH;
	p->z = tile->pos.z + ((1 + (ssize_t)(chunk_id % 16) - (16 - x) / 16.f) * CHUNK_WIDTH);
	p->y = chunk->height[idx];
}

static void add_chunk(struct map_tile *tile, struct map_chunk *chunk, uint8_t chunk_id, const struct collision_params *params, struct jks_array *triangles)
{
#if 1
	float xmin = tile->pos.x - (1 + (ssize_t)(chunk_id / 16)) * CHUNK_WIDTH;
	ssize_t z_end = floorf((params->aabb.p0.x - xmin) / (CHUNK_WIDTH / 8));
	z_end = 8 - z_end;
	if (z_end > 8)
		z_end = 8;
	else if (z_end < 8)
		z_end++;
	ssize_t z_start = ceilf((params->aabb.p1.x - xmin) / (CHUNK_WIDTH / 8));
	z_start = 8 - z_start;
	if (z_start < 0)
		z_start = 0;
	else if (z_start > 0)
		z_start--;
	if (z_start >= z_end)
		return;
	float zmin = tile->pos.z + (chunk_id % 16) * CHUNK_WIDTH;
	ssize_t x_end = ceilf((params->aabb.p1.z - zmin) / (CHUNK_WIDTH / 8));
	if (x_end > 8)
		x_end = 8;
	else if (x_end < 8)
		x_end++;
	ssize_t x_start = floorf((params->aabb.p0.z - zmin) / (CHUNK_WIDTH / 8));
	if (x_start < 0)
		x_start = 0;
	else if (x_start > 0)
		x_start--;
	if (x_start >= x_end)
		return;
#else
	ssize_t x_start = 0;
	ssize_t x_end = 8;
	ssize_t z_start = 0;
	ssize_t z_end = 8;
#endif
	size_t triangles_nb = triangles->size;
	struct collision_triangle *tmp = jks_array_grow(triangles, (z_end - z_start) * (x_end - x_start) * 4);
	if (!tmp)
	{
		LOG_ERROR("triangles allocation failed");
		return;
	}
	size_t n = 0;
	for (ssize_t z = z_start; z < z_end; ++z)
	{
		for (ssize_t x = x_start; x < x_end; ++x)
		{
			if (chunk->holes & (1 << (z / 2 * 4 + x / 2)))
				continue;
			n++;
			uint16_t idx = 9 + z * 17 + x;
			uint16_t p1 = idx - 9;
			uint16_t p2 = idx - 8;
			uint16_t p3 = idx + 9;
			uint16_t p4 = idx + 8;
			add_point(tile, chunk, chunk_id, p2, &tmp->points[0]);
			add_point(tile, chunk, chunk_id, p1, &tmp->points[1]);
			add_point(tile, chunk, chunk_id, idx, &tmp->points[2]);
			tmp->touched = false;
			tmp++;
			add_point(tile, chunk, chunk_id, p3, &tmp->points[0]);
			add_point(tile, chunk, chunk_id, p2, &tmp->points[1]);
			add_point(tile, chunk, chunk_id, idx, &tmp->points[2]);
			tmp->touched = false;
			tmp++;
			add_point(tile, chunk, chunk_id, p4, &tmp->points[0]);
			add_point(tile, chunk, chunk_id, p3, &tmp->points[1]);
			add_point(tile, chunk, chunk_id, idx, &tmp->points[2]);
			tmp->touched = false;
			tmp++;
			add_point(tile, chunk, chunk_id, p1, &tmp->points[0]);
			add_point(tile, chunk, chunk_id, p4, &tmp->points[1]);
			add_point(tile, chunk, chunk_id, idx, &tmp->points[2]);
			tmp->touched = false;
			tmp++;
		}
	}
	jks_array_resize(triangles, triangles_nb + n * 4);
}

static void add_object_point(struct gx_m2_instance *m2, struct vec3f *point, uint16_t idx)
{
	struct vec4f tmp;
	VEC3_CPY(tmp, m2->parent->collision_vertexes[idx]);
	tmp.w = 1;
	struct vec4f out;
	MAT4_VEC4_MUL(out, m2->m, tmp);
	VEC3_CPY(*point, out);
}

static void add_object(struct gx_m2_instance *m2, struct jks_array *triangles)
{
	if (!m2->parent->collision_triangles_nb)
		return;
	struct collision_triangle *tmp = jks_array_grow(triangles, m2->parent->collision_triangles_nb / 3);
	if (!tmp)
	{
		LOG_ERROR("triangles allocation failed");
		return;
	}
	for (size_t i = 0; i < m2->parent->collision_triangles_nb;)
	{
		add_object_point(m2, &tmp->points[0], m2->parent->collision_triangles[i++]);
		add_object_point(m2, &tmp->points[1], m2->parent->collision_triangles[i++]);
		add_object_point(m2, &tmp->points[2], m2->parent->collision_triangles[i++]);
		tmp->touched = false;
		tmp++;
	}
}

static void add_objects(struct map_tile *tile, struct map_chunk *chunk, const struct collision_params *params, struct collision_state *state, struct jks_array *triangles)
{
	for (size_t i = 0; i < chunk->doodads_nb; ++i)
	{
		struct map_m2 *m2 = tile->m2[chunk->doodads[i]];
		if (!m2->instance->parent->loaded)
			continue;
		for (size_t j = 0; j < state->m2.size; ++j)
		{
			if (*JKS_ARRAY_GET(&state->m2, j, struct gx_m2_instance*) == m2->instance)
				goto next_m2;
		}
		if (!jks_array_push_back(&state->m2, &m2->instance))
			LOG_WARN("failed to add m2 to visited list");
#if 0 /* doesn't work on some objects */
		struct vec3f delta;
		VEC3_SUB(delta, m2->instance->pos, params->center);
		if (VEC3_NORM(delta) > m2->instance->parent->collision_sphere_radius * m2->instance->scale + params->radius)
			continue;
#endif
		if (!aabb_intersect_sphere(&m2->instance->caabb, params->center, params->radius)
		 || !aabb_intersect_aabb(&m2->instance->caabb, &params->aabb))
			continue;
		add_object(m2->instance, triangles);
next_m2:;
	}
}

static void add_wmo_point(struct gx_wmo_instance *wmo, struct gx_wmo_group *group, struct vec3f *point, uint32_t indice)
{
	struct wow_vec3f *src = JKS_ARRAY_GET(&group->movt, *JKS_ARRAY_GET(&group->movi, indice, uint16_t), struct wow_vec3f);
	struct vec4f tmp;
	VEC3_CPY(tmp, *src);
	tmp.w = 1;
	struct vec4f out;
	MAT4_VEC4_MUL(out, wmo->m, tmp);
	VEC3_CPY(*point, out);
}

enum bsp_side
{
	BSP_POS = (1 << 0),
	BSP_NEG = (1 << 1),
};

static enum bsp_side get_aabb_bsp_side(const struct aabb *aabb, const struct wow_mobn_node *node)
{
	enum bsp_side side = 0;
	uint8_t plane = node->flags & WOW_MOBN_NODE_FLAGS_AXIS_MASK;
	if (((float*)&aabb->p0)[plane] <= node->plane_dist)
		side |= BSP_NEG;
	if (((float*)&aabb->p1)[plane] >= node->plane_dist)
		side |= BSP_POS;
	return side;
}

static void bsp_add_triangles(struct gx_wmo_instance *wmo, struct gx_wmo_group *group, const struct wow_mobn_node *node, const struct collision_params *params, struct jks_array *triangles, struct jks_array *triangles_tracker)
{
	if (!node->faces_nb)
		return;
	struct collision_triangle *tmp = jks_array_grow(triangles, node->faces_nb);
	if (!tmp)
	{
		LOG_ERROR("triangles allocation failed");
		return;
	}
	size_t n = 0;
	for (size_t i = 0; i < node->faces_nb; ++i)
	{
		assert(node->face_start + i < group->mobr.size);
		uint32_t indice = *JKS_ARRAY_GET(&group->mobr, node->face_start + i, uint16_t);
		uint8_t flags = JKS_ARRAY_GET(&group->mopy, indice, struct wow_mopy_data)->flags;
		if (!((flags & WOW_MOPY_FLAGS_COLLISION) || ((flags & WOW_MOPY_FLAGS_RENDER) && !(flags & WOW_MOPY_FLAGS_DETAIL))))
			continue;
		if (params->wmo_cam && (flags & WOW_MOPY_FLAGS_NOCAMCOLLIDE))
			continue;
		for (size_t j = 0; j < triangles_tracker->size; ++j)
		{
			if (*JKS_ARRAY_GET(triangles_tracker, j, uint32_t) == indice)
				goto next_triangle;
		}
		if (!jks_array_push_back(triangles_tracker, &indice))
			LOG_ERROR("failed to add triangle to triangles tracker");
		indice *= 3;
		add_wmo_point(wmo, group, &tmp->points[0], indice + 0);
		add_wmo_point(wmo, group, &tmp->points[1], indice + 1);
		add_wmo_point(wmo, group, &tmp->points[2], indice + 2);
		tmp->touched = false;
		tmp++;
		n++;
next_triangle:;
	}
	jks_array_resize(triangles, triangles->size - (node->faces_nb - n));
}

static void bsp_traverse(struct gx_wmo_instance *wmo, struct gx_wmo_group *group, const struct wow_mobn_node *node, const struct aabb *aabb, const struct collision_params *params, struct jks_array *triangles, struct jks_array *triangles_tracker)
{
	if (node->flags & WOW_MOBN_NODE_FLAGS_LEAF)
	{
		bsp_add_triangles(wmo, group, node, params, triangles, triangles_tracker);
		return;
	}
	enum bsp_side side = get_aabb_bsp_side(aabb, node);
	if ((side & BSP_POS) && node->pos_child != -1)
		bsp_traverse(wmo, group, JKS_ARRAY_GET(&group->mobn, node->pos_child, struct wow_mobn_node), aabb, params, triangles, triangles_tracker);
	if ((side & BSP_NEG) && node->neg_child != -1)
		bsp_traverse(wmo, group, JKS_ARRAY_GET(&group->mobn, node->neg_child, struct wow_mobn_node), aabb, params, triangles, triangles_tracker);
}

static void add_wmo_group(struct gx_wmo_instance *wmo, struct gx_wmo_group *group, const struct collision_params *params, struct collision_state *state, struct jks_array *triangles, struct jks_array *triangles_tracker)
{
	for (size_t i = 0; i < group->doodads.size; ++i)
	{
		uint16_t doodad = *JKS_ARRAY_GET(&group->doodads, i, uint16_t);
		if (doodad < wmo->doodad_start || doodad >= wmo->doodad_end)
			continue;
		struct gx_m2_instance *m2 = *JKS_ARRAY_GET(&wmo->m2, doodad - wmo->doodad_start, struct gx_m2_instance*);
		if (!m2->parent->loaded)
			continue;
		for (size_t j = 0; j < state->m2.size; ++j)
		{
			if (*JKS_ARRAY_GET(&state->m2, j, struct gx_m2_instance*) == m2)
				goto next_m2;
		}
		if (!jks_array_push_back(&state->m2, &m2))
			LOG_WARN("failed to add m2 to visited list");
		if (!aabb_intersect_sphere(&m2->caabb, params->center, params->radius)
		 || !aabb_intersect_aabb(&m2->caabb, &params->aabb))
			continue;
		add_object(m2, triangles);
next_m2:;
	}
	if (group->mobn.size)
	{
		struct vec4f p0;
		struct vec4f p1;
		struct vec4f tmp0;
		struct vec4f tmp1;
		struct aabb aabb;
		VEC3_CPY(p0, params->aabb.p0);
		VEC3_CPY(p1, params->aabb.p1);
		p0.w = 1;
		p1.w = 1;
		MAT4_VEC4_MUL(tmp0, wmo->m_inv, p0);
		MAT4_VEC4_MUL(tmp1, wmo->m_inv, p1);
		p0.x = tmp0.x;
		p0.y = -tmp0.z;
		p0.z = tmp0.y;
		p1.x = tmp1.x;
		p1.y = -tmp1.z;
		p1.z = tmp1.y;
		VEC3_MIN(aabb.p0, p0, p1);
		VEC3_MAX(aabb.p1, p0, p1);
		jks_array_resize(triangles_tracker, 0);
		bsp_traverse(wmo, group, group->mobn.data, &aabb, params, triangles, triangles_tracker);
	}
}

static void add_wmo(struct gx_wmo_instance *wmo, const struct collision_params *params, struct collision_state *state, struct jks_array *triangles)
{
	struct jks_array triangles_tracker;
	jks_array_init(&triangles_tracker, sizeof(uint32_t), NULL, NULL);
	for (size_t i = 0; i < wmo->groups.size; ++i)
	{
		struct gx_wmo_group *group = *JKS_ARRAY_GET(&wmo->parent->groups, i, struct gx_wmo_group*);
		if (!group->loaded)
			continue;
		struct gx_wmo_group_instance *group_instance = JKS_ARRAY_GET(&wmo->groups, i, struct gx_wmo_group_instance);
		if (!aabb_intersect_sphere(&group_instance->aabb, params->center, params->radius))
			continue;
		add_wmo_group(wmo, group, params, state, triangles, &triangles_tracker);
	}
	jks_array_destroy(&triangles_tracker);
}

static void add_wmos(struct map_tile *tile, struct map_chunk *chunk, const struct collision_params *params, struct collision_state *state, struct jks_array *triangles)
{
	for (size_t i = 0; i < chunk->wmos_nb; ++i)
	{
		struct map_wmo *wmo = tile->wmo[chunk->wmos[i]];
		if (!wmo->instance->parent->loaded)
			continue;
		for (size_t j = 0; j < state->wmo.size; ++j)
		{
			if (*JKS_ARRAY_GET(&state->wmo, j, struct gx_wmo_instance*) == wmo->instance)
				goto next_wmo;
		}
		if (!jks_array_push_back(&state->wmo, &wmo->instance))
			LOG_WARN("failed to add wmo to visited list");
		if (!aabb_intersect_sphere(&wmo->instance->aabb, params->center, params->radius))
			continue;
		add_wmo(wmo->instance, params, state, triangles);
next_wmo:;
	}
}

void map_tile_collect_collision_triangles(struct map_tile *tile, const struct collision_params *params, struct collision_state *state, struct jks_array *triangles)
{
	if (!tile->gx_mcnk)
		return;
#if 1
	float xmin = tile->pos.x - CHUNK_WIDTH;
	ssize_t z_end = floorf((params->aabb.p0.x - xmin) / CHUNK_WIDTH);
	z_end = 16 - z_end;
	if (z_end > 16)
		z_end = 16;
	else if (z_end < 16)
		z_end++;
	ssize_t z_start = ceilf((params->aabb.p1.x - xmin) / CHUNK_WIDTH);
	z_start = 16 - z_start;
	if (z_start < 0)
		z_start = 0;
	else if (z_start > 0)
		z_start--;
	if (z_start >= z_end)
		return;
	float zmin = tile->pos.z - CHUNK_WIDTH * 15;
	ssize_t x_end = ceilf((params->aabb.p1.z - zmin) / CHUNK_WIDTH);
	if (x_end > 16)
		x_end = 16;
	else if (x_end < 16)
		x_end++;
	ssize_t x_start = floorf((params->aabb.p0.z - zmin) / CHUNK_WIDTH);
	if (x_start < 0)
		x_start = 0;
	else if (x_start > 0)
		x_start--;
	if (x_start >= x_end)
		return;
#else
	ssize_t x_start = 0;
	ssize_t x_end = 16;
	ssize_t z_start = 0;
	ssize_t z_end = 16;
#endif
	if (aabb_intersect_aabb(&tile->aabb, &params->aabb))
	{
		for (ssize_t x = x_start; x < x_end; ++x)
		{
			for (ssize_t z = z_start; z < z_end; ++z)
			{
				uint8_t chunk_id = z * 16 + x;
				struct map_chunk *chunk = &tile->chunks[chunk_id];
				if (aabb_intersect_aabb(&chunk->aabb, &params->aabb))
					add_chunk(tile, chunk, chunk_id, params, triangles);
			}
		}
	}
	if (aabb_intersect_aabb(&tile->objects_aabb, &params->aabb))
	{
		for (ssize_t x = x_start; x < x_end; ++x)
		{
			for (ssize_t z = z_start; z < z_end; ++z)
			{
				uint8_t chunk_id = z * 16 + x;
				struct map_chunk *chunk = &tile->chunks[chunk_id];
				if (chunk->doodads_nb && aabb_intersect_aabb(&chunk->objects_aabb, &params->aabb))
					add_objects(tile, chunk, params, state, triangles);
			}
		}
	}
	if (aabb_intersect_aabb(&tile->wmos_aabb, &params->aabb))
	{
		for (ssize_t x = x_start; x < x_end; ++x)
		{
			for (ssize_t z = z_start; z < z_end; ++z)
			{
				uint8_t chunk_id = z * 16 + x;
				struct map_chunk *chunk = &tile->chunks[chunk_id];
				if (chunk->wmos_nb && aabb_intersect_aabb(&chunk->wmos_aabb, &params->aabb))
					add_wmos(tile, chunk, params, state, triangles);
			}
		}
	}
}

struct map_wmo *map_wmo_new(void)
{
	struct map_wmo *handle = mem_zalloc(MEM_GX, sizeof(*handle));
	if (!handle)
		return NULL;
	return handle;
}

void map_wmo_delete(struct map_wmo *handle)
{
	if (!handle)
		return;
	gx_wmo_instance_gc(handle->instance);
	mem_free(MEM_GX, handle);
}

void map_wmo_load(struct map_wmo *handle, const char *filename)
{
	if (handle->instance)
		return;
	handle->instance = gx_wmo_instance_new(filename);
}

struct map_m2 *map_m2_new(void)
{
	struct map_m2 *handle = mem_zalloc(MEM_GX, sizeof(*handle));
	if (!handle)
		return NULL;
	return handle;
}

void map_m2_delete(struct map_m2 *handle)
{
	if (!handle)
		return;
	gx_m2_instance_gc(handle->instance);
	mem_free(MEM_GX, handle);
}

void map_m2_load(struct map_m2 *handle, const char *filename)
{
	if (handle->instance)
		return;
	handle->instance = gx_m2_instance_new_filename(filename);
}

void map_chunk_get_interp_points(float px, float pz, uint8_t *points)
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

void map_chunk_get_interp_factors(float px, float pz, uint8_t *points, float *factors)
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

void map_chunk_get_y(struct map_chunk *chunk, float px, float pz, uint8_t *points, float *factors, float *y)
{
	static const uint8_t pos[5] = {0, 1, 17, 18, 9};
	uint8_t base = (9 + 8) * (int)pz + (int)px;
	float y0 = chunk->height[base + pos[points[0]]];
	float y1 = chunk->height[base + pos[points[1]]];
	float y2 = chunk->height[base + pos[points[2]]];
	*y = y0 * factors[0] + y1 * factors[1] + y2 * factors[2];
}

void map_chunk_get_n(struct map_chunk *chunk, float px, float pz, uint8_t *points, float *factors, int8_t *n)
{
	static const uint8_t pos[5] = {0, 1, 17, 18, 9};
	uint8_t base = (9 + 8) * (int)pz + (int)px;
	int8_t n0[3];
	int8_t n1[3];
	int8_t n2[3];
	memcpy(n0, &chunk->norm[base + pos[points[0]]], 3);
	memcpy(n1, &chunk->norm[base + pos[points[1]]], 3);
	memcpy(n2, &chunk->norm[base + pos[points[2]]], 3);
	for (int i = 0; i < 3; ++i)
		n[i] = n0[i] * factors[0] + n1[i] * factors[1] + n2[i] * factors[2];
}

