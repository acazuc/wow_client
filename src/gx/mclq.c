#include "gx/frame.h"
#include "gx/mcnk.h"
#include "gx/mclq.h"

#include "graphics.h"
#include "shaders.h"
#include "camera.h"
#include "memory.h"
#include "const.h"
#include "log.h"
#include "wow.h"

#include <gfx/device.h>

#include <libwow/adt.h>

#include <jks/array.h>

#include <sys/types.h>
#include <string.h>
#include <math.h>

MEMORY_DECL(GX);

struct gx_mclq_init_data
{
	struct jks_array indices; /* uint16_t */
	struct jks_array vertexes; /* struct shader_mclq_input */
	struct jks_array depths; /* uint8_t */
};

static void clear_init_data(struct gx_mclq_init_data *init_data)
{
	if (!init_data)
		return;
	jks_array_destroy(&init_data->indices);
	jks_array_destroy(&init_data->vertexes);
	jks_array_destroy(&init_data->depths);
	mem_free(MEM_GX, init_data);
}

static struct gx_mclq_init_data *create_init_data(void)
{
	struct gx_mclq_init_data *init_data = mem_malloc(MEM_GX, sizeof(*init_data));
	if (!init_data)
		return NULL;
	jks_array_init(&init_data->indices, sizeof(uint16_t), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&init_data->vertexes, sizeof(struct shader_mclq_input), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&init_data->depths, sizeof(uint8_t), NULL, &jks_array_memory_fn_GX);
	return init_data;
}

static bool get_liquid_id(uint8_t liquid_type, uint8_t *type)
{
	switch (liquid_type & WOW_MCLQ_TILE_LIQUID_TYPE)
	{
		case WOW_MCLQ_TILE_LIQUID_OCEAN:
			*type = 1;
			return true;
		case WOW_MCLQ_TILE_LIQUID_RIVER:
			*type = 0;
			return true;
		case WOW_MCLQ_TILE_LIQUID_MAGMA:
			*type = 2;
			return true;
		case WOW_MCLQ_TILE_LIQUID_SLIME:
			*type = 3;
			return true;
	}
	LOG_ERROR("unknown MCLQ liquid: %d", (int)(liquid_type & WOW_MCLQ_TILE_LIQUID_TYPE));
	return false;
}

static bool is_uniform_batch(struct wow_mclq_data *mclq)
{
	float height = mclq->verts[0].water.height;
	uint8_t depth = mclq->verts[0].water.depth;
	uint8_t liquid_type = mclq->tiles[0][0] & WOW_MCLQ_TILE_LIQUID_TYPE;
	if (mclq->height_min != mclq->height_max)
		return false;
	for (size_t z = 0; z < 8; ++z)
	{
		for (size_t x = 0; x < 8; ++x)
		{
			if (mclq->tiles[z][x] & WOW_MCLQ_TILE_HIDDEN)
				return false;
			if ((mclq->tiles[z][x] & WOW_MCLQ_TILE_LIQUID_TYPE) != liquid_type)
				return false;
		}
	}
	for (size_t z = 0; z < 9; ++z)
	{
		for (size_t x = 0; x < 9; ++x)
		{
			if (liquid_type != WOW_MCLQ_TILE_LIQUID_OCEAN && liquid_type != WOW_MCLQ_TILE_LIQUID_RIVER)
				return false;
			if (mclq->verts[z * 9 + x].water.depth != depth)
				return false;
			if (mclq->verts[z * 9 + x].water.height != height)
				return false;
		}
	}
	return true;
}

static void init_liquid(struct gx_mclq_liquid *liquid)
{
	liquid->init_data = NULL;
	liquid->batches = NULL;
	liquid->batches_nb = 0;
#ifdef WITH_DEBUG_RENDERING
	gx_aabb_init(&liquid->gx_aabb, (struct vec4f){0, .5, 1, 1}, 3);
	liquid->gx_aabb.aabb = liquid->aabb;
	liquid->gx_aabb.flags |= GX_AABB_DIRTY;
#endif
	liquid->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		liquid->uniform_buffers[i] = GFX_BUFFER_INIT();
	liquid->vertexes_buffer = GFX_BUFFER_INIT();
	liquid->indices_buffer = GFX_BUFFER_INIT();
	liquid->depths_buffer = GFX_BUFFER_INIT();
	liquid->in_render_list = false;
}

static struct gx_mclq_batch *get_new_batch(struct gx_mclq_liquid *liquid)
{
	struct gx_mclq_batch *tmp = mem_realloc(MEM_GX, liquid->batches, sizeof(*liquid->batches) * (liquid->batches_nb + 1));
	if (!tmp)
		return NULL;
	liquid->batches = tmp;
	return &tmp[liquid->batches_nb++];
}

static bool init_uniform_batch(struct gx_mclq *mclq, struct wow_mclq_data *wow_mclq, uint32_t cx, uint32_t cz, struct vec3f *min_pos, struct vec3f *max_pos, bool *first_min_max)
{
	if (wow_mclq->tiles[0][0] & WOW_MCLQ_TILE_HIDDEN)
		return true;
	uint8_t liquid_type = wow_mclq->tiles[0][0] & WOW_MCLQ_TILE_LIQUID_TYPE;
	float height = wow_mclq->verts[0].water.height;
	uint8_t depth = wow_mclq->verts[0].water.depth;
	uint8_t type;
	if (!get_liquid_id(liquid_type, &type))
		return true;
	struct gx_mclq_liquid *liquid = &mclq->liquids[type];
	struct gx_mclq_batch *batch = get_new_batch(liquid);
	if (!batch)
		return false;
	if (!liquid->init_data)
	{
		liquid->init_data = create_init_data();
		if (!liquid->init_data)
		{
			mem_free(MEM_GX, batch);
			return false;
		}
	}
	struct gx_mclq_init_data *init_data = liquid->init_data;
	batch->indices_offset = init_data->indices.size;
	if (type < 2)
	{
		uint8_t *tmp = jks_array_grow(&init_data->depths, 4);
		if (!tmp)
		{
			LOG_ERROR("allocation failed");
			return false;
		}
		tmp[0] = depth;
		tmp[1] = depth;
		tmp[2] = depth;
		tmp[3] = depth;
	}
	{
		struct shader_mclq_input *vertex = jks_array_grow(&init_data->vertexes, 4);
		if (!vertex)
		{
			LOG_ERROR("allocation failed");
			return false;
		}
		float x0 = (-(ssize_t)cz + 0) * CHUNK_WIDTH;
		float x1 = (-(ssize_t)cz - 1) * CHUNK_WIDTH;
		float z0 = ( (ssize_t)cx + 0) * CHUNK_WIDTH;
		float z1 = ( (ssize_t)cx + 1) * CHUNK_WIDTH;
		VEC3_SET(vertex[0].position, x0, height, z0);
		VEC3_SET(vertex[1].position, x0, height, z1);
		VEC3_SET(vertex[2].position, x1, height, z1);
		VEC3_SET(vertex[3].position, x1, height, z0);
		float u0 = 0;
		float u1 = 2;
		float v0 = 0;
		float v1 = 2;
		VEC2_SET(vertex[0].uv, u0, v0);
		VEC2_SET(vertex[1].uv, u1, v0);
		VEC2_SET(vertex[2].uv, u1, v1);
		VEC2_SET(vertex[3].uv, u0, v1);
	}
	{
		uint16_t *data = jks_array_grow(&init_data->indices, 6);
		if (!data)
		{
			LOG_ERROR("allocation failed");
			return false;
		}
		uint16_t idx1 = init_data->vertexes.size - 4;
		uint16_t idx2 = idx1 + 1;
		uint16_t idx3 = idx1 + 2;
		uint16_t idx4 = idx1 + 3;
		data[0] = idx1;
		data[1] = idx3;
		data[2] = idx2;
		data[3] = idx1;
		data[4] = idx4;
		data[5] = idx3;
	}

	struct vec3f p0 = {(-(ssize_t)cz - 0) * CHUNK_WIDTH, height, ((ssize_t)cx + 0) * CHUNK_WIDTH};
	struct vec3f p1 = {(-(ssize_t)cz - 1) * CHUNK_WIDTH, height, ((ssize_t)cx + 1) * CHUNK_WIDTH};
	VEC3_MIN(batch->aabb.p0, p0, p1);
	VEC3_MAX(batch->aabb.p1, p0, p1);
	aabb_move(&batch->aabb, mclq->pos);
#ifdef WITH_DEBUG_RENDERING
	gx_aabb_init(&batch->gx_aabb, (struct vec4f){0, 1, .5, 1}, 1);
	batch->gx_aabb.aabb = batch->aabb;
	batch->gx_aabb.flags |= GX_AABB_DIRTY;
#endif
	VEC3_ADD(batch->center, batch->aabb.p0, batch->aabb.p1);
	VEC3_MULV(batch->center, batch->center, .5f);
	batch->indices_nb = liquid->init_data->indices.size - batch->indices_offset;
	if (first_min_max[type])
	{
		min_pos[type] = batch->aabb.p0;
		max_pos[type] = batch->aabb.p1;
		first_min_max[type] = false;
	}
	else
	{
		VEC3_MIN(min_pos[type], min_pos[type], batch->aabb.p0);
		VEC3_MAX(max_pos[type], max_pos[type], batch->aabb.p1);
	}
	return true;
}

static bool init_full_batch(struct gx_mclq *mclq, struct wow_mclq_data *wow_mclq, uint32_t cx, uint32_t cz, struct vec3f *min_pos, struct vec3f *max_pos, bool *first_min_max)
{
	uint32_t indices_bck[GX_MCLQ_LIQUIDS_NB];
	for (size_t i = 0; i < GX_MCLQ_LIQUIDS_NB; ++i)
	{
		if (mclq->liquids[i].init_data)
			indices_bck[i] = mclq->liquids[i].init_data->indices.size;
		else
			indices_bck[i] = 0;
	}
	bool batch_min_max_set[GX_MCLQ_LIQUIDS_NB] = {false};
	struct vec3f batch_min[GX_MCLQ_LIQUIDS_NB];
	struct vec3f batch_max[GX_MCLQ_LIQUIDS_NB];
	for (size_t z = 0; z < 8; ++z)
	{
		for (size_t x = 0; x < 8; ++x)
		{
			if (wow_mclq->tiles[z][x] & WOW_MCLQ_TILE_HIDDEN)
				continue;
			uint8_t type;
			if (!get_liquid_id(wow_mclq->tiles[z][x], &type))
				continue;
			struct gx_mclq_liquid *liquid = &mclq->liquids[type];
			if (!liquid->init_data)
			{
				liquid->init_data = create_init_data();
				if (!liquid->init_data)
					return false;
			}
			struct gx_mclq_init_data *init_data = liquid->init_data;
			switch (wow_mclq->tiles[z][x] & WOW_MCLQ_TILE_LIQUID_TYPE)
			{
				case WOW_MCLQ_TILE_LIQUID_OCEAN:
				case WOW_MCLQ_TILE_LIQUID_RIVER:
				{
					uint8_t *depth = jks_array_grow(&init_data->depths, 4);
					if (!depth)
					{
						LOG_ERROR("allocation failed");
						return false;
					}
					depth[0] = wow_mclq->verts[ z      * 9 +  x     ].water.depth;
					depth[1] = wow_mclq->verts[ z      * 9 + (x + 1)].water.depth;
					depth[2] = wow_mclq->verts[(z + 1) * 9 + (x + 1)].water.depth;
					depth[3] = wow_mclq->verts[(z + 1) * 9 +  x     ].water.depth;
					break;
				}
			}
			{
				struct shader_mclq_input *vertex = jks_array_grow(&init_data->vertexes, 4);
				if (!vertex)
				{
					LOG_ERROR("allocation failed");
					return false;
				}
				float x0 = -(1 + (ssize_t)cz - (8 -  z     ) / 8.) * CHUNK_WIDTH;
				float x1 = -(1 + (ssize_t)cz - (8 - (z + 1)) / 8.) * CHUNK_WIDTH;
				float z0 =  (1 + (ssize_t)cx - (8 -  x     ) / 8.) * CHUNK_WIDTH;
				float z1 =  (1 + (ssize_t)cx - (8 - (x + 1)) / 8.) * CHUNK_WIDTH;
				float y1 = wow_mclq->verts[ z      * 9 +  x     ].water.height;
				float y2 = wow_mclq->verts[ z      * 9 + (x + 1)].water.height;
				float y3 = wow_mclq->verts[(z + 1) * 9 + (x + 1)].water.height;
				float y4 = wow_mclq->verts[(z + 1) * 9 +  x     ].water.height;
				VEC3_SET(vertex[0].position, x0, y1, z0);
				VEC3_SET(vertex[1].position, x0, y2, z1);
				VEC3_SET(vertex[2].position, x1, y3, z1);
				VEC3_SET(vertex[3].position, x1, y4, z0);
				if (!batch_min_max_set[type])
				{
					batch_min[type].x = x1;
					batch_max[type].x = x0;
					batch_min[type].y = y1;
					if (y2 < batch_min[type].y)
						batch_min[type].y = y2;
					if (y3 < batch_min[type].y)
						batch_min[type].y = y3;
					if (y4 < batch_min[type].y)
						batch_min[type].y = y4;
					batch_max[type].y = y1;
					if (y2 > batch_max[type].y)
						batch_max[type].y = y2;
					if (y3 > batch_max[type].y)
						batch_max[type].y = y3;
					if (y4 > batch_max[type].y)
						batch_max[type].y = y4;
					batch_min[type].z = z0;
					batch_max[type].z = z1;
					batch_min_max_set[type] = true;
				}
				else
				{
					if (x1 < batch_min[type].x)
						batch_min[type].x = x1;
					if (x0 > batch_max[type].x)
						batch_max[type].x = x0;
					if (y1 < batch_min[type].y)
						batch_min[type].y = y1;
					if (y2 < batch_min[type].y)
						batch_min[type].y = y2;
					if (y3 < batch_min[type].y)
						batch_min[type].y = y3;
					if (y4 < batch_min[type].y)
						batch_min[type].y = y4;
					if (y1 > batch_max[type].y)
						batch_max[type].y = y1;
					if (y2 > batch_max[type].y)
						batch_max[type].y = y2;
					if (y3 > batch_max[type].y)
						batch_max[type].y = y3;
					if (y4 > batch_max[type].y)
						batch_max[type].y = y4;
					if (z0 < batch_min[type].z)
						batch_min[type].z = z0;
					if (z1 > batch_max[type].z)
						batch_max[type].z = z1;
				}
				switch (wow_mclq->tiles[z][x] & WOW_MCLQ_TILE_LIQUID_TYPE)
				{
					case WOW_MCLQ_TILE_LIQUID_MAGMA:
					case WOW_MCLQ_TILE_LIQUID_SLIME:
					{
						size_t n = z * 9 + x;
						float dividor = 160 / 2;
						VEC2_SET(vertex[0].uv, wow_mclq->verts[n +  0].magma.s / dividor, wow_mclq->verts[n +  0].magma.t / dividor);
						VEC2_SET(vertex[1].uv, wow_mclq->verts[n +  1].magma.s / dividor, wow_mclq->verts[n +  1].magma.t / dividor);
						VEC2_SET(vertex[2].uv, wow_mclq->verts[n + 10].magma.s / dividor, wow_mclq->verts[n + 10].magma.t / dividor);
						VEC2_SET(vertex[3].uv, wow_mclq->verts[n +  9].magma.s / dividor, wow_mclq->verts[n +  9].magma.t / dividor);
						break;
					}
					default:
					{
						float u0 = 2 * x / 8.;
						float u1 = 2 * (x + 1) / 8.;
						float v0 = 2 * z / 8.;
						float v1 = 2 * (z + 1) / 8.;
						VEC2_SET(vertex[0].uv, u0, v0);
						VEC2_SET(vertex[1].uv, u1, v0);
						VEC2_SET(vertex[2].uv, u1, v1);
						VEC2_SET(vertex[3].uv, u0, v1);
						break;
					}
				}
			}
			{
				uint16_t *data = jks_array_grow(&init_data->indices, 6);
				if (!data)
				{
					LOG_ERROR("allocation failed");
					return false;
				}
				uint16_t p1 = init_data->vertexes.size - 4;
				uint16_t p2 = p1 + 1;
				uint16_t p3 = p1 + 2;
				uint16_t p4 = p1 + 3;
				data[0] = p1;
				data[1] = p3;
				data[2] = p2;
				data[3] = p1;
				data[4] = p4;
				data[5] = p3;
			}
		}
	}
	for (size_t i = 0; i < GX_MCLQ_LIQUIDS_NB; ++i)
	{
		struct gx_mclq_liquid *liquid = &mclq->liquids[i];
		if (!liquid->init_data)
			continue;
		if (!(liquid->init_data->indices.size - indices_bck[i]))
			continue;
		struct gx_mclq_batch *batch = get_new_batch(liquid);
		batch->indices_offset = indices_bck[i];
		batch->indices_nb = liquid->init_data->indices.size - batch->indices_offset;
		batch->aabb.p0 = batch_min[i];
		batch->aabb.p1 = batch_max[i];
		aabb_move(&batch->aabb, mclq->pos);
#ifdef WITH_DEBUG_RENDERING
		gx_aabb_init(&batch->gx_aabb, (struct vec4f){0, 1, .5, 1}, 1);
		batch->gx_aabb.aabb = batch->aabb;
		batch->gx_aabb.flags |= GX_AABB_DIRTY;
#endif
		VEC3_ADD(batch->center, batch->aabb.p0, batch->aabb.p1);
		VEC3_MULV(batch->center, batch->center, .5f);
		if (first_min_max[i])
		{
			min_pos[i] = batch->aabb.p0;
			max_pos[i] = batch->aabb.p1;
			first_min_max[i] = false;
		}
		else
		{
			VEC3_MIN(min_pos[i], min_pos[i], batch->aabb.p0);
			VEC3_MAX(max_pos[i], max_pos[i], batch->aabb.p1);
		}
	}
	return true;
}

struct gx_mclq *gx_mclq_new(struct map_tile *parent, struct wow_adt_file *file)
{
	struct gx_mclq *mclq = mem_malloc(MEM_GX, sizeof(*mclq));
	if (!mclq)
		return NULL;
	mclq->parent = parent;
	mclq->initialized = false;
	VEC3_SET(mclq->pos, file->mcnk[0].header.position.x, 0, -file->mcnk[0].header.position.y);
	struct mat4f tmp;
	MAT4_IDENTITY(tmp);
	MAT4_TRANSLATE(mclq->m, tmp, mclq->pos);
	struct vec3f min_pos[GX_MCLQ_LIQUIDS_NB];
	struct vec3f max_pos[GX_MCLQ_LIQUIDS_NB];
	bool first_min_max[GX_MCLQ_LIQUIDS_NB];
	memset(first_min_max, 1, sizeof(first_min_max));
	for (size_t i = 0; i < GX_MCLQ_LIQUIDS_NB; ++i)
		init_liquid(&mclq->liquids[i]);
	for (uint32_t cz = 0; cz < 16; ++cz)
	{
		for (uint32_t cx = 0; cx < 16; ++cx)
		{
			struct wow_mcnk *mcnk = &file->mcnk[cz * 16 + cx];
			if (mcnk->header.size_mclq <= 8 || memcmp("QLCM", (uint8_t*)&mcnk->mclq.header.magic, 4))
				continue;
			for (size_t i = 0; i < mcnk->mclq.data_nb; ++i)
			{
				wow_mclq_data_t *wow_mclq = &mcnk->mclq.data[i];
				if (is_uniform_batch(wow_mclq))
				{
					if (!init_uniform_batch(mclq, wow_mclq, cx, cz, min_pos, max_pos, first_min_max))
						goto err;
				}
				else
				{
					if (!init_full_batch(mclq, wow_mclq, cx, cz, min_pos, max_pos, first_min_max))
						goto err;
				}
			}
		}
	}
	for (size_t i = 0; i < GX_MCLQ_LIQUIDS_NB; ++i)
	{
		struct gx_mclq_liquid *liquid = &mclq->liquids[i];
		liquid->aabb.p0 = min_pos[i];
		liquid->aabb.p1 = max_pos[i];
#ifdef WITH_DEBUG_RENDERING
		liquid->gx_aabb.aabb = liquid->aabb;
		liquid->gx_aabb.flags |= GX_AABB_DIRTY;
#endif
	}
	return mclq;

err:
	gx_mclq_delete(mclq);
	return NULL;
}

void gx_mclq_delete(struct gx_mclq *mclq)
{
	if (!mclq)
		return;
	for (size_t i = 0; i < GX_MCLQ_LIQUIDS_NB; ++i)
	{
		struct gx_mclq_liquid *liquid = &mclq->liquids[i];
		for (size_t j = 0; j < RENDER_FRAMES_COUNT; ++j)
			gfx_delete_buffer(g_wow->device, &liquid->uniform_buffers[j]);
		gfx_delete_buffer(g_wow->device, &liquid->vertexes_buffer);
		gfx_delete_buffer(g_wow->device, &liquid->indices_buffer);
		gfx_delete_buffer(g_wow->device, &liquid->depths_buffer);
		gfx_delete_attributes_state(g_wow->device, &liquid->attributes_state);
		clear_init_data(liquid->init_data);
		for (size_t j = 0; j < liquid->batches_nb; ++j)
		{
#ifdef WITH_DEBUG_RENDERING
			struct gx_mclq_batch *batch = &liquid->batches[j];
			gx_aabb_destroy(&batch->gx_aabb);
#endif
		}
		mem_free(MEM_GX, liquid->batches);
#ifdef WITH_DEBUG_RENDERING
		gx_aabb_destroy(&liquid->gx_aabb);
#endif
	}
	mem_free(MEM_GX, mclq);
}

int gx_mclq_initialize(struct gx_mclq *mclq)
{
	if (mclq->initialized)
		return 1;
	for (size_t i = 0; i < GX_MCLQ_LIQUIDS_NB; ++i)
	{
		struct gx_mclq_liquid *liquid = &mclq->liquids[i];
		if (!liquid->init_data)
			continue;
		struct gx_mclq_init_data *init_data = liquid->init_data;
		gfx_create_buffer(g_wow->device, &liquid->indices_buffer, GFX_BUFFER_INDICES, init_data->indices.data, init_data->indices.size * init_data->indices.data_size, GFX_BUFFER_IMMUTABLE);
		if (i < 2)
		{
			gfx_create_buffer(g_wow->device, &liquid->depths_buffer, GFX_BUFFER_VERTEXES, init_data->depths.data, init_data->depths.size * sizeof(uint8_t), GFX_BUFFER_IMMUTABLE);
			for (size_t j = 0; j < RENDER_FRAMES_COUNT; ++j)
				gfx_create_buffer(g_wow->device, &liquid->uniform_buffers[j], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_mclq_water_model_block), GFX_BUFFER_STREAM);
		}
		else
		{
			for (size_t j = 0; j < RENDER_FRAMES_COUNT; ++j)
				gfx_create_buffer(g_wow->device, &liquid->uniform_buffers[j], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_mclq_magma_model_block), GFX_BUFFER_STREAM);
		}
		gfx_create_buffer(g_wow->device, &liquid->vertexes_buffer, GFX_BUFFER_VERTEXES, init_data->vertexes.data, init_data->vertexes.size * init_data->vertexes.data_size, GFX_BUFFER_IMMUTABLE);
		clear_init_data(init_data);
		liquid->init_data = NULL;
		if (i < 2)
		{
			gfx_attribute_bind_t binds[] =
			{
				{&liquid->vertexes_buffer, sizeof(struct shader_mclq_input), offsetof(struct shader_mclq_input, position)},
				{&liquid->depths_buffer  , sizeof(uint8_t)                 , 0},
				{&liquid->vertexes_buffer, sizeof(struct shader_mclq_input), offsetof(struct shader_mclq_input, uv)},
			};
			gfx_create_attributes_state(g_wow->device, &liquid->attributes_state, binds, sizeof(binds) / sizeof(*binds), &liquid->indices_buffer, GFX_INDEX_UINT16);
		}
		else
		{
			gfx_attribute_bind_t binds[] =
			{
				{&liquid->vertexes_buffer, sizeof(struct shader_mclq_input), offsetof(struct shader_mclq_input, position)},
				{&liquid->vertexes_buffer, sizeof(struct shader_mclq_input), offsetof(struct shader_mclq_input, uv)},
			};
			gfx_create_attributes_state(g_wow->device, &liquid->attributes_state, binds, sizeof(binds) / sizeof(*binds), &liquid->indices_buffer, GFX_INDEX_UINT16);
		}
	}
	mclq->initialized = true;
	return 1;
}

void gx_mclq_cull(struct gx_mclq *mclq)
{
	if (!mclq->initialized)
		return;
	bool added = false;
	for (size_t i = 0; i < GX_MCLQ_LIQUIDS_NB; ++i)
	{
		struct gx_mclq_liquid *liquid = &mclq->liquids[i];
		if (!liquid->batches)
			continue;
		enum frustum_result result = frustum_check(&g_wow->cull_frame->frustum, &liquid->aabb);
		if (!result)
			continue;
		render_add_mclq(i, mclq);
		if (!added)
		{
			MAT4_TRANSLATE(mclq->render_frames[g_wow->cull_frame_id].mvp, g_wow->cull_frame->view_vp, mclq->pos);
			MAT4_TRANSLATE(mclq->render_frames[g_wow->cull_frame_id].mv, g_wow->cull_frame->view_v, mclq->pos);
			added = true;
		}
		for (size_t j = 0; j < liquid->batches_nb; ++j)
		{
			struct gx_mclq_batch *batch = &liquid->batches[j];
			struct vec3f delta;
			VEC3_SUB(delta, batch->center, g_wow->cull_frame->cull_pos);
			float distance_to_camera = sqrt(delta.x * delta.x + delta.z * delta.z);
			if (distance_to_camera > g_wow->cull_frame->view_distance * 1.4142)
			{
				batch->render_frames[g_wow->cull_frame_id].culled = true;
				continue;
			}
			if (result == FRUSTUM_INSIDE)
			{
				batch->render_frames[g_wow->cull_frame_id].culled = false;
				continue;
			}
			batch->render_frames[g_wow->cull_frame_id].culled = !frustum_check_fast(&g_wow->cull_frame->frustum, &batch->aabb);
		}
	}
}

void gx_mclq_render(struct gx_mclq *mclq, uint8_t type)
{
	if (!mclq->initialized)
		return;
	struct gx_mclq_liquid *liquid = &mclq->liquids[type];
	if (!liquid->attributes_state.handle.u64)
		return;
	bool initialized = false;
	for (size_t i = 0; i < liquid->batches_nb; ++i)
	{
		struct gx_mclq_batch *batch = &liquid->batches[i];
		if (batch->render_frames[g_wow->draw_frame_id].culled)
			continue;
		if (!initialized)
		{
			switch (type)
			{
				case 0:
				case 1:
				{
					struct shader_mclq_water_model_block model_block;
					model_block.p = *(struct mat4f*)&g_wow->draw_frame->view_p;
					model_block.v = *(struct mat4f*)&g_wow->draw_frame->view_v;
					model_block.mv = mclq->render_frames[g_wow->draw_frame_id].mv;
					model_block.mvp = mclq->render_frames[g_wow->draw_frame_id].mvp;
					gfx_set_buffer_data(&liquid->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
					gfx_bind_attributes_state(g_wow->device, &liquid->attributes_state, &g_wow->graphics->mclq_water_input_layout);
					break;
				}
				case 2:
				case 3:
				{
					struct shader_mclq_magma_model_block model_block;
					model_block.v = *(struct mat4f*)&g_wow->draw_frame->view_v;
					model_block.mv = mclq->render_frames[g_wow->draw_frame_id].mv;
					model_block.mvp = mclq->render_frames[g_wow->draw_frame_id].mvp;
					gfx_set_buffer_data(&liquid->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
					gfx_bind_attributes_state(g_wow->device, &liquid->attributes_state, &g_wow->graphics->mclq_magma_input_layout);
					break;
				}
			}
			gfx_bind_constant(g_wow->device, 1, &liquid->uniform_buffers[g_wow->draw_frame_id], sizeof(struct shader_mclq_water_model_block), 0);
			initialized = true;
		}
		gfx_draw_indexed(g_wow->device, batch->indices_nb, batch->indices_offset);
	}
}

#ifdef WITH_DEBUG_RENDERING
void gx_mclq_render_aabb(struct gx_mclq *mclq)
{
	if (!mclq->initialized)
		return;
	for (size_t i = 0; i < GX_MCLQ_LIQUIDS_NB; ++i)
	{
		struct gx_mclq_liquid *liquid = &mclq->liquids[i];
		if (!liquid->batches)
			continue;
		gx_aabb_update(&liquid->gx_aabb, (struct mat4f*)&g_wow->draw_frame->view_vp);
		gx_aabb_render(&liquid->gx_aabb);
		for (size_t j = 0; j < liquid->batches_nb; ++j)
		{
			struct gx_mclq_batch *batch = &liquid->batches[j];
			if (batch->render_frames[g_wow->draw_frame_id].culled)
				continue;
			gx_aabb_update(&batch->gx_aabb, (struct mat4f*)&g_wow->draw_frame->view_vp);
			gx_aabb_render(&batch->gx_aabb);
		}
	}
}
#endif
