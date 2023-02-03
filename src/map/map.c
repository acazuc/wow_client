#include "ppe/render_target.h"
#include "ppe/render_pass.h"

#include "gx/m2_particles.h"
#include "gx/wmo_mliq.h"
#include "gx/skybox.h"
#include "gx/frame.h"
#include "gx/mcnk.h"
#include "gx/mclq.h"
#include "gx/text.h"
#include "gx/taxi.h"
#include "gx/wmo.h"
#include "gx/wdl.h"
#include "gx/m2.h"

#include "obj/player.h"

#include "map/tile.h"
#include "map/map.h"

#include "performance.h"
#include "graphics.h"
#include "shaders.h"
#include "camera.h"
#include "memory.h"
#include "loader.h"
#include "cache.h"
#include "const.h"
#include "blp.h"
#include "log.h"
#include "wow.h"
#include "dbc.h"

#include <gfx/device.h>

#include <libwow/wdt.h>
#include <libwow/wdl.h>
#include <libwow/mpq.h>
#include <libwow/dbc.h>
#include <libwow/blp.h>

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

MEMORY_DECL(GENERIC);

struct map *map_new(void)
{
	struct map *map = mem_zalloc(MEM_GX, sizeof(*map));
	if (!map)
		return NULL;
	map->id = -1;
	map->fog_divisor = 1300;
	map->minimap.texture = GFX_TEXTURE_INIT();
	pthread_mutex_init(&map->minimap.mutex, NULL);
	for (uint32_t i = 0; i < sizeof(map->lavag_textures) / sizeof(*map->lavag_textures); ++i)
	{
		char filename[256];
		snprintf(filename, sizeof(filename), "XTEXTURES\\LAVAGREEN\\LAVAGREEN.%u.BLP", i + 1);
		if (cache_ref_by_key_blp(g_wow->cache, filename, &map->lavag_textures[i]))
			blp_texture_ask_load(map->lavag_textures[i]);
		else
			LOG_ERROR("failed to ref %s", filename);
	}
	for (uint32_t i = 0; i < sizeof(map->river_textures) / sizeof(*map->river_textures); ++i)
	{
		char filename[256];
		snprintf(filename, sizeof(filename), "XTEXTURES\\RIVER\\LAKE_A.%u.BLP", i + 1);
		if (cache_ref_by_key_blp(g_wow->cache, filename, &map->river_textures[i]))
			blp_texture_ask_load(map->river_textures[i]);
		else
			LOG_ERROR("failed to ref %s", filename);
	}
	for (uint32_t i = 0; i < sizeof(map->ocean_textures) / sizeof(*map->ocean_textures); ++i)
	{
		char filename[256];
		snprintf(filename, sizeof(filename), "XTEXTURES\\OCEAN\\OCEAN_H.%u.blp", i + 1);
		if (cache_ref_by_key_blp(g_wow->cache, filename, &map->ocean_textures[i]))
			blp_texture_ask_load(map->ocean_textures[i]);
		else
			LOG_ERROR("failed to ref %s", filename);
	}
	for (uint32_t i = 0; i < sizeof(map->magma_textures) / sizeof(*map->magma_textures); ++i)
	{
		char filename[256];
		snprintf(filename, sizeof(filename), "XTEXTURES\\LAVA\\LAVA.%u.BLP", i + 1);
		if (cache_ref_by_key_blp(g_wow->cache, filename, &map->magma_textures[i]))
			blp_texture_ask_load(map->magma_textures[i]);
		else
			LOG_ERROR("failed to ref %s", filename);
	}
	for (uint32_t i = 0; i < sizeof(map->slime_textures) / sizeof(*map->slime_textures); ++i)
	{
		char filename[256];
		snprintf(filename, sizeof(filename), "XTEXTURES\\SLIME\\SLIME.%u.BLP", i + 1);
		if (cache_ref_by_key_blp(g_wow->cache, filename, &map->slime_textures[i]))
			blp_texture_ask_load(map->slime_textures[i]);
		else
			LOG_ERROR("failed to ref %s", filename);
	}
	for (uint32_t i = 0; i < sizeof(map->fasta_textures) / sizeof(*map->fasta_textures); ++i)
	{
		char filename[256];
		snprintf(filename, sizeof(filename), "XTEXTURES\\RIVER\\FAST_A.%u.BLP", i + 1);
		if (cache_ref_by_key_blp(g_wow->cache, filename, &map->fasta_textures[i]))
			blp_texture_ask_load(map->fasta_textures[i]);
		else
			LOG_ERROR("failed to ref %s", filename);
	}
	return map;
}

void map_delete(struct map *map)
{
	if (!map)
		return;
	for (uint32_t i = 0; i < map->tiles_nb; ++i)
		map_tile_ask_unload(map->tile_array[map->tiles[i]]);
	gfx_delete_buffer(g_wow->device, &map->particles_indices_buffer);
	gfx_delete_buffer(g_wow->device, &map->mcnk_vertexes_buffer);
	gfx_delete_buffer(g_wow->device, &map->mcnk_indices_buffer);
	for (uint32_t i = 0; i < sizeof(map->lavag_textures) / sizeof(*map->lavag_textures); ++i)
		cache_unref_by_ref_blp(g_wow->cache, map->lavag_textures[i]);
	for (uint32_t i = 0; i < sizeof(map->ocean_textures) / sizeof(*map->ocean_textures); ++i)
		cache_unref_by_ref_blp(g_wow->cache, map->ocean_textures[i]);
	for (uint32_t i = 0; i < sizeof(map->river_textures) / sizeof(*map->river_textures); ++i)
		cache_unref_by_ref_blp(g_wow->cache, map->river_textures[i]);
	for (uint32_t i = 0; i < sizeof(map->magma_textures) / sizeof(*map->magma_textures); ++i)
		cache_unref_by_ref_blp(g_wow->cache, map->magma_textures[i]);
	for (uint32_t i = 0; i < sizeof(map->slime_textures) / sizeof(*map->slime_textures); ++i)
		cache_unref_by_ref_blp(g_wow->cache, map->slime_textures[i]);
	for (uint32_t i = 0; i < sizeof(map->fasta_textures) / sizeof(*map->fasta_textures); ++i)
		cache_unref_by_ref_blp(g_wow->cache, map->fasta_textures[i]);
	gx_wmo_instance_gc(map->wmo);
	mem_free(MEM_GX, map->name);
	gx_skybox_delete(map->gx_skybox);
	gx_wdl_delete(map->gx_wdl);
	gfx_delete_texture(g_wow->device, &map->minimap.texture);
#ifdef WITH_DEBUG_RENDERING
	gx_taxi_delete(map->gx_taxi);
#endif
	pthread_mutex_destroy(&map->minimap.mutex);
	mem_free(MEM_GX, map);
}

static bool load_tile(struct map *map, int32_t x, int32_t z)
{
	uint32_t adt_idx = z * 64 + x;
	if (!(map->adt_exists[z] & (1ull << x)))
		return true;
	if (map->tile_array[adt_idx])
		return true;
	char buf[256];
	snprintf(buf, sizeof(buf), "WORLD\\MAPS\\%s\\%s_%d_%d.adt", map->name, map->name, x, z);
	struct map_tile *tile = map_tile_new(buf, x, z);
	if (!tile)
		return false;
	map->tiles[map->tiles_nb++] = adt_idx;
	map->tile_array[adt_idx] = tile;
	map_tile_ask_load(tile);
	return true;
}

static void map_load_tick(struct map *map)
{
	if (!map->has_adt)
		return;
	struct vec3f pos = g_wow->cull_frame->cull_pos;
	struct vec3f delta;
	VEC3_SUB(delta, pos, map->last_pos);
	if (map->last_view_distance == g_wow->cull_frame->view_distance && sqrt(delta.x * delta.x + delta.z * delta.z) < CHUNK_WIDTH)
		return;
	map->last_view_distance = g_wow->cull_frame->view_distance;
	map->last_pos = pos;
	int32_t distance = ceil(g_wow->cull_frame->view_distance / 16. / CHUNK_WIDTH);
	if (distance < 0)
		distance = 0;
	int32_t adt_x = floor(32 + pos.z / (16 * CHUNK_WIDTH));
	int32_t adt_z = floor(32 - pos.x / (16 * CHUNK_WIDTH));
	int32_t adt_start_x = floor(adt_x - distance);
	int32_t adt_start_z = floor(adt_z - distance);
	int32_t adt_end_x = ceil(adt_x + distance);
	int32_t adt_end_z = ceil(adt_z + distance);
	adt_start_x = (adt_start_x < 0 ? 0 : (adt_start_x > 63 ? 63 : adt_start_x));
	adt_start_z = (adt_start_z < 0 ? 0 : (adt_start_z > 63 ? 63 : adt_start_z));
	adt_end_x = (adt_end_x < 0 ? 0 : (adt_end_x > 63 ? 63 : adt_end_x));
	adt_end_z = (adt_end_z < 0 ? 0 : (adt_end_z > 63 ? 63 : adt_end_z));
	for (int32_t z = adt_start_z; z <= adt_end_z; ++z)
	{
		for (int32_t x = adt_start_x; x <= adt_end_x; ++x)
		{
			if (!load_tile(map, x, z))
				LOG_ERROR("failed load adt %" PRId32 "x%" PRId32, x, z);
		}
	}
	/* deleting them after because of instance caching (M2 & WMO) */
	for (uint32_t i = 0; i < map->tiles_nb; ++i)
	{
		uint16_t adt_id = map->tiles[i];
		struct map_tile *tile = map->tile_array[adt_id];
		if (tile->x < adt_start_x - 1
		 || tile->x > adt_end_x + 1
		 || tile->z < adt_start_z - 1
		 || tile->z > adt_end_z + 1)
		{
			map->tile_array[adt_id] = NULL;
			memmove(&map->tiles[i], &map->tiles[i + 1], sizeof(*map->tiles) * (map->tiles_nb - i - 1));
			map->tiles_nb--;
			map_tile_ask_unload(tile);
			i--;
		}
	}
}

bool generate_minimap_texture(struct map *map)
{
	char mapname[256];
	snprintf(mapname, sizeof(mapname), "%s", map->name);
	normalize_mpq_filename(mapname, sizeof(mapname));
	struct trs_dir *trs_dir = jks_hmap_get(g_wow->trs, JKS_HMAP_KEY_STR(mapname));
	if (!trs_dir)
	{
		LOG_WARN("no TRS found");
		return false;
	}
	uint8_t *data = mem_malloc(MEM_GENERIC, 768 * 768 * 4);
	if (!data)
		return false;
	memset(data, 0, 768 * 768 * 4);
	int last_x = map->minimap.last_x;
	int last_z = map->minimap.last_z;
	int orgx = 30 - last_x;
	int orgz = 31 + last_z;
	for (int y = 0; y < 3; ++y)
	{
		for (int x = 0; x < 3; ++x)
		{
			int mx = orgz + x;
			int mz = orgx + y;
			if (mx < 0 || mx >= 64 || mz < 0 || mx >= 64)
				continue;
			char name[512];
			snprintf(name, sizeof(name), "%s/map%02d_%02d.blp", mapname, mx, mz);
			normalize_mpq_filename(name, sizeof(name));
			char **hashp = jks_hmap_get(trs_dir->entries, JKS_HMAP_KEY_STR(name));
			if (!hashp)
			{
				LOG_ERROR("trs entry not found: %s", name);
				continue;
			}
			char path[512];
			snprintf(path, sizeof(path), "TEXTURES\\MINIMAP\\%s", *hashp);
			struct wow_mpq_file *mpq = wow_mpq_get_file(g_wow->mpq_compound, path);
			if (!mpq)
			{
				LOG_ERROR("failed to open %s", path);
				continue;
			}
			struct wow_blp_file *blp = wow_blp_file_new(mpq);
			wow_mpq_file_delete(mpq);
			if (!blp)
			{
				LOG_ERROR("failed to parse %s", path);
				continue;
			}
			uint8_t *blp_data;
			uint32_t blp_width;
			uint32_t blp_height;
			if (!blp_decode_rgba(blp, 0, &blp_width, &blp_height, &blp_data))
			{
				LOG_ERROR("failed to decode %s", path);
				wow_blp_file_delete(blp);
				continue;
			}
			wow_blp_file_delete(blp);
			if (blp_width > 256 || blp_height > 256)
			{
				LOG_ERROR("image too big: %" PRIu32 "x%" PRIu32, blp_width, blp_height);
				mem_free(MEM_GENERIC, blp_data);
				continue;
			}
			for (uint32_t sy = 0; sy < blp_height; ++sy)
			{
				for (uint32_t sx = 0; sx < blp_width; ++sx)
				{
					uint8_t *s = &blp_data[(sy * blp_width + sx) * 4];
					uint8_t *d = &data[((sy + y * 256) * 768 + sx + x * 256) * 4];
					d[0] = s[2];
					d[1] = s[1];
					d[2] = s[0];
					d[3] = s[3];
				}
			}
			mem_free(MEM_GENERIC, blp_data);
		}
	}
	pthread_mutex_lock(&map->minimap.mutex);
	mem_free(MEM_GENERIC, map->minimap.data);
	map->minimap.data = data;
	map->minimap.data_x = last_x;
	map->minimap.data_z = last_z;
	map->minimap.computing = false;
	pthread_mutex_unlock(&map->minimap.mutex);
	return true;
}

static bool create_wdl(struct map *map)
{
	char buf[256];
	snprintf(buf, sizeof(buf), "WORLD\\MAPS\\%s\\%s.WDL", map->name, map->name);
	struct wow_mpq_file *file = wow_mpq_get_file(g_wow->mpq_compound, buf);
	if (!file)
	{
		LOG_ERROR("failed to get wdl file \"%s\"", buf);
		return false;
	}
	struct wow_wdl_file *wdl = wow_wdl_file_new(file);
	wow_mpq_file_delete(file);
	if (!wdl)
	{
		LOG_ERROR("failed to parse wdl file \"%s\"", buf);
		return false;
	}
	map->gx_wdl = gx_wdl_new(wdl);
	wow_wdl_file_delete(wdl);
	if (!map->gx_wdl)
	{
		LOG_ERROR("failed to create wdl renderer");
		return false;
	}
	return true;
}

/*
LOD 1: 256

 1    2    3    4    5    6    7    8    9
   10   11   12   13   14   15   16   17
 18   19   20   21   22   23   24   25   26
   27   28   29   30   31   32   33   34
 35   36   37   38   39   40   41   42   43
   44   45   46   47   48   49   50   51
 52   53   54   55   56   57   58   59   60
   61   62   63   64   65   66   67   68
 69   70   71   72   73   74   75   76   77
   78   79   80   81   82   83   84   85
 86   87   88   89   90   91   92   93   94
   95   96   97   98  99  100  101  102
103  104  105  106  107  108  109  110  111
  112  113  114  115  116  117  118  119
120  121  122  123  124  125  126  127  128
  129  130  131  132  133  134  135  136
137  138  139  140  141  142  143  144  145
 */

/* 
LOD 2: 128

 1    2    3    4    5    6    7    8    9

 18   19   20   21   22   23   24   25   26

 35   36   37   38   39   40   41   42   43

 52   53   54   55   56   57   58   59   60

 69   70   71   72   73   74   75   76   77

 86   87   88   89   90   91   92   93   94

103  104  105  106  107  108  109  110  111

120  121  122  123  124  125  126  127  128

137  138  139  140  141  142  143  144  145
 */

/*
LOD 3: 48

 1    2    3    4    5    6    7    8    9

 18                                      26

 35        37        39        41        43

 52                                      60

 69        71        73        75        77

 86                                      94

103       105       107       109       111

120                                     128

137  138  139  140  141  142  143  144  145*/

/* Possible lod opti: if no holes, place sync points only near other chunks */
/* One more lod if no holes ? */
static bool init_mcnk_data(struct map *map)
{
	bool ret = false;
	uint16_t *indices;
	uint32_t points_nb = (16 * 16 * (9 * 9 + 8 * 8));
	struct
	{
		struct vec2f xz;
		struct vec2f uv;
	} *vertexes;
	uint32_t indices_pos = 0;
	uint16_t *particles_indices;
	indices = mem_malloc(MEM_GX, sizeof(*indices) * (16 * 16 * ((8 * 8 * 4 * 3) + (8 * 8 * 2 * 3) + (48 * 3))));
	if (!indices)
		goto err1;
	vertexes = mem_malloc(MEM_GX, sizeof(*vertexes) * points_nb);
	if (!vertexes)
		goto err2;
	for (size_t cz = 0; cz < 16; ++cz)
	{
		for (size_t cx = 0; cx < 16; ++cx)
		{
			size_t base = (cz * 16 + cx) * (9 * 9 + 8 * 8);
			for (size_t i = 0; i < 9 * 9 + 8 * 8; ++i)
			{
				size_t y2 = i % 17;
				float z = i / 17 * 2;
				float x;
				if (y2 < 9)
				{
					x = y2 * 2;
				}
				else
				{
					z++;
					x = (y2 - 9) * 2 + 1;
				}
				vertexes[base + i].xz = (struct vec2f){(-1 - (ssize_t)cz + (16 - z) / 16.f) * CHUNK_WIDTH, ((1 + cx - (16 - x) / 16.f) * CHUNK_WIDTH)};
				vertexes[base + i].uv = (struct vec2f){x / 16.f, z / 16.f};
			}
			for (size_t z = 0; z < 8; ++z)
			{
				for (size_t x = 0; x < 8; ++x)
				{
					uint16_t idx = base + 9 + z * 17 + x;
					uint16_t p1 = idx - 9;
					uint16_t p2 = idx - 8;
					uint16_t p3 = idx + 9;
					uint16_t p4 = idx + 8;
					indices[indices_pos++] = p2;
					indices[indices_pos++] = p1;
					indices[indices_pos++] = idx;
					indices[indices_pos++] = p3;
					indices[indices_pos++] = p2;
					indices[indices_pos++] = idx;
					indices[indices_pos++] = p4;
					indices[indices_pos++] = p3;
					indices[indices_pos++] = idx;
					indices[indices_pos++] = p1;
					indices[indices_pos++] = p4;
					indices[indices_pos++] = idx;
				}
			}
			for (size_t z = 0; z < 8; ++z)
			{
				for (size_t x = 0; x < 8; ++x)
				{
					uint16_t idx = base + 9 + z * 17 + x;
					uint16_t p1 = idx - 9;
					uint16_t p2 = idx - 8;
					uint16_t p3 = idx + 9;
					uint16_t p4 = idx + 8;
					indices[indices_pos++] = p2;
					indices[indices_pos++] = p1;
					indices[indices_pos++] = p3;
					indices[indices_pos++] = p3;
					indices[indices_pos++] = p1;
					indices[indices_pos++] = p4;
				}
			}
			static const uint16_t points[144] =
			{
				1  , 0  , 17 , 36 , 17 , 34 , 36 , 2  , 1  , 36 , 1  , 17,
				36 , 3  , 2  , 38 , 4  , 3  , 38 , 3  , 36,
				38 , 5  , 4  , 40 , 6  , 5  , 40 , 5  , 38,
				25 , 8  , 7  , 40 , 42 , 25 , 40 , 7  , 6  , 40 , 25 , 7,
				70 , 51 , 68 , 36 , 34 , 51 , 36 , 51 , 70,
				36 , 70 , 72 , 72 , 38 , 36 ,
				40 , 38 , 72 , 72 , 74 , 40 ,
				40 , 59 , 42 , 74 , 76 , 59 , 74 , 59 , 40,
				104, 85 , 102, 70 , 68 , 85 , 70 , 85 , 104,
				104, 106, 72 , 72 , 70 , 104,
				72 , 106, 108, 108, 74 , 72 ,
				74 , 93 , 76 , 108, 110, 93 , 108, 93 , 74,
				119, 136, 137, 104, 102, 119, 104, 137, 138, 104, 119, 137,
				106, 139, 140, 104, 138, 139, 104, 139, 106,
				108, 141, 142, 106, 140, 141, 106, 141, 108,
				143, 144, 127, 108, 142, 143, 108, 127, 110, 108, 143, 127
			};
			for (size_t i = 0; i < 144; ++i)
				indices[indices_pos++] = base + points[i];
		}
	}
	particles_indices = mem_malloc(MEM_GX, sizeof(*particles_indices) * MAX_PARTICLES * 6);
	if (!particles_indices)
		goto err3;
	for (size_t i = 0; i < MAX_PARTICLES; ++i)
	{
		uint16_t *tmp = &particles_indices[i * 6];
		size_t n = i * 4;
		tmp[0] = n + 0;
		tmp[1] = n + 1;
		tmp[2] = n + 2;
		tmp[3] = n + 0;
		tmp[4] = n + 2;
		tmp[5] = n + 3;
	}
	map->particles_indices_buffer = GFX_BUFFER_INIT();
	gfx_create_buffer(g_wow->device, &map->particles_indices_buffer, GFX_BUFFER_INDICES, particles_indices, 6 * MAX_PARTICLES * sizeof(*particles_indices), GFX_BUFFER_IMMUTABLE);
	map->mcnk_vertexes_buffer = GFX_BUFFER_INIT();
	map->mcnk_indices_nb = indices_pos;
	map->mcnk_indices_buffer = GFX_BUFFER_INIT();
	gfx_create_buffer(g_wow->device, &map->mcnk_vertexes_buffer, GFX_BUFFER_VERTEXES, vertexes, points_nb * sizeof(*vertexes), GFX_BUFFER_IMMUTABLE);
	gfx_create_buffer(g_wow->device, &map->mcnk_indices_buffer, GFX_BUFFER_INDICES, indices, indices_pos * sizeof(*indices), GFX_BUFFER_IMMUTABLE);
	ret = true;

	mem_free(MEM_GX, particles_indices);
err3:
	mem_free(MEM_GX, vertexes);
err2:
	mem_free(MEM_GX, indices);
err1:
	return ret;
}

static bool load_wdt(struct map *map, struct wow_wdt_file *file)
{
	map->wmo = NULL;
	map->last_pos = (struct vec3f){-999999, -999999, -999999};
	map->flags = file->mphd.flags;
	map->last_check = 0;
	map->last_view_distance = 0;
	map->has_adt = false;
	LOG_INFO("WDT flags: %x", map->flags);
	for (size_t z = 0; z < 64; ++z)
	{
		map->adt_exists[z] = 0;
		for (size_t x = 0; x < 64; ++x)
		{
			if (file->main.data[z * 64 + x].flags & 1)
			{
				map->adt_exists[z] |= 1ull << x;
				map->has_adt = true;
			}
		}
	}
	if (map->has_adt)
		return init_mcnk_data(map);
	char filename[512];
	snprintf(filename, sizeof(filename), "%s", &file->mwmo.data[0]);
	normalize_wmo_filename(filename, sizeof(filename));
	map->wmo = gx_wmo_instance_new(filename);
	if (!map->wmo)
	{
		LOG_ERROR("failed to create wmo world instance");
		return true;
	}
	float offset = 0;/* (32 * 16 + 1) * CHUNK_WIDTH; */
	struct vec3f pos;
	VEC3_SET(pos, offset - file->modf.data[0].position.z, file->modf.data[0].position.y, -(offset - file->modf.data[0].position.x));
	struct mat4f mat1;
	struct mat4f mat2;
	MAT4_IDENTITY(mat1);
	MAT4_TRANSLATE(mat2, mat1, pos);
	MAT4_ROTATEY(float, mat1, mat2, (file->modf.data[0].rotation.y + 180) / 180. * M_PI);
	MAT4_ROTATEZ(float, mat2, mat1, -(file->modf.data[0].rotation.x) / 180. * M_PI);
	MAT4_ROTATEX(float, mat1, mat2, (file->modf.data[0].rotation.z) / 180. * M_PI);
	gx_wmo_instance_set_mat(map->wmo, &mat1);
	VEC3_CPY(map->wmo->pos, pos);
	map->wmo->doodad_set = file->modf.data[0].doodad_set;
	gx_wmo_ask_load(map->wmo->parent);
	return true;
}

static bool create_wdt(struct map *map)
{
	char buf[256];
	snprintf(buf, sizeof(buf), "WORLD\\MAPS\\%s\\%s.WDT", map->name, map->name);
	struct wow_mpq_file *file = wow_mpq_get_file(g_wow->mpq_compound, buf);
	if (!file)
	{
		LOG_ERROR("failed to get wdt file \"%s\"", buf);
		return false;
	}
	struct wow_wdt_file *wdt = wow_wdt_file_new(file);
	wow_mpq_file_delete(file);
	if (!wdt)
	{
		LOG_ERROR("failed to parse wdt file \"%s\"", buf);
		return false;
	}
	if (!load_wdt(map, wdt))
	{
		wow_wdt_file_delete(wdt);
		LOG_ERROR("failed to create wdt renderer");
		return false;
	}
	wow_wdt_file_delete(wdt);
	return true;
}

static void load_worldmap(struct map *map)
{
	for (size_t i = 0; i < g_wow->dbc.world_map_continent->file->header.record_count; ++i)
	{
		struct wow_dbc_row row = wow_dbc_get_row(g_wow->dbc.world_map_continent->file, i);
		uint32_t continent = wow_dbc_get_u32(&row, 4);
		if (continent != map->id)
			continue;
		map->worldmap.miny = wow_dbc_get_flt(&row, 36);
		map->worldmap.minx = wow_dbc_get_flt(&row, 40);
		map->worldmap.maxy = wow_dbc_get_flt(&row, 44);
		map->worldmap.maxx = wow_dbc_get_flt(&row, 48);
		map->worldmap.width = map->worldmap.maxx - map->worldmap.minx;
		map->worldmap.height = map->worldmap.maxy - map->worldmap.miny;
		return;
	}
	map->worldmap.minx = 0;
	map->worldmap.miny = 0;
	map->worldmap.maxx = 34133.4016;
	map->worldmap.maxy = 34133.4016;
	map->worldmap.width = map->worldmap.maxx - map->worldmap.minx;
	map->worldmap.height = map->worldmap.maxy - map->worldmap.miny;
	LOG_WARN("no worldmap found");
}

static struct taxi_node *find_taxi_node(struct map *map, uint32_t id)
{
	for (size_t i = 0; i < map->taxi.nodes_count; ++i)
	{
		if (wow_dbc_get_u32(&map->taxi.nodes[i].dbc, 0) == id)
			return &map->taxi.nodes[i];
	}
	return NULL;
}

static bool load_taxi(struct map *map)
{
	for (size_t i = 0; i < g_wow->dbc.taxi_nodes->file->header.record_count; ++i)
	{
		struct wow_dbc_row row = wow_dbc_get_row(g_wow->dbc.taxi_nodes->file, i);
		uint32_t continent = wow_dbc_get_u32(&row, 4);
		if (continent != map->id)
			continue;
		float x = wow_dbc_get_flt(&row, 12);
		float y = wow_dbc_get_flt(&row, 8);
		if (x < map->worldmap.minx || x > map->worldmap.maxx
		 || y < map->worldmap.miny || y > map->worldmap.maxy)
			continue;
		if (continent == 530)
		{
			uint32_t nodeid = wow_dbc_get_u32(&row, 0);
			if (nodeid != 99
			 && nodeid != 100
			 && nodeid != 102
			 && nodeid != 117
			 && nodeid != 118
			 && nodeid != 119
			 && nodeid != 120
			 && nodeid != 121
			 && nodeid != 122
			 && nodeid != 123
			 && nodeid != 124
			 && nodeid != 125
			 && nodeid != 126
			 && nodeid != 127
			 && nodeid != 128
			 && nodeid != 129
			 && nodeid != 130
			 && nodeid != 139
			 && nodeid != 140
			 && nodeid != 141
			 && nodeid != 149
			 && nodeid != 150
			 && nodeid != 156
			 && nodeid != 159
			 && nodeid != 160
			 && nodeid != 163
			 && nodeid != 164)
				continue;
		}
		struct taxi_node *tmp = mem_realloc(MEM_UI, map->taxi.nodes, sizeof(*tmp) * (map->taxi.nodes_count + 1));
		if (!tmp)
		{
			LOG_ERROR("nodes allocation failed");
			return false;
		}
		map->taxi.nodes = tmp;
		map->taxi.nodes[map->taxi.nodes_count].dbc = row;
		map->taxi.nodes[map->taxi.nodes_count].links = NULL;
		map->taxi.nodes[map->taxi.nodes_count].links_count = 0;
		map->taxi.nodes_count++;
	}
	for (size_t i = 0; i < g_wow->dbc.taxi_path->file->header.record_count; ++i)
	{
		struct wow_dbc_row row = wow_dbc_get_row(g_wow->dbc.taxi_path->file, i);
		struct taxi_node *src = find_taxi_node(map, wow_dbc_get_u32(&row, 4));
		if (!src)
			continue;
		struct taxi_node *dst = find_taxi_node(map, wow_dbc_get_u32(&row, 8));
		if (!dst)
			continue;
		struct taxi_link *tmp = mem_realloc(MEM_UI, src->links, sizeof(*tmp) * (src->links_count + 1));
		if (!tmp)
		{
			LOG_ERROR("links allocation failed");
			return false;
		}
		src->links = tmp;
		src->links[src->links_count].dst = dst;
		src->links[src->links_count].price = wow_dbc_get_u32(&row, 12);
		src->links_count++;
	}
	return true;
}

static void setup_pf_dist(void)
{
	while (1)
	{
		struct taxi_node *cur = NULL;
		for (size_t i = 0; i < g_wow->map->taxi.nodes_count; ++i)
		{
			if (g_wow->map->taxi.nodes[i].pf_visited
			 || g_wow->map->taxi.nodes[i].pf_dist == (size_t)-1
			 || (cur && g_wow->map->taxi.nodes[i].pf_dist >= cur->pf_dist))
				continue;
			cur = &g_wow->map->taxi.nodes[i];
		}
		if (!cur)
			break;
		cur->pf_visited = true;
		for (size_t i = 0; i < cur->links_count; ++i)
		{
			struct taxi_node *dst = cur->links[i].dst;
			size_t nxt_dist = cur->pf_dist + 1;
			size_t nxt_price = cur->pf_price + cur->links[i].price;
			if (dst->pf_visited
			 || (dst->pf_dist != (size_t)-1 && (dst->pf_dist < nxt_dist || dst->pf_price < nxt_price)))
				continue;
			dst->pf_price = nxt_price;
			dst->pf_prev = cur;
			dst->pf_dist = nxt_dist;
		}
	}
}

void map_gen_taxi_path(struct map *map, uint32_t srcid, uint32_t dstid)
{
	if (srcid == map->taxi.path_src
	 && dstid == map->taxi.path_dst)
		return;
	map->taxi.path_src = srcid;
	map->taxi.path_dst = dstid;
	mem_free(MEM_UI, map->taxi.path_nodes);
	map->taxi.path_nodes = NULL;
	map->taxi.path_nodes_count = 0;
	if (srcid >= map->taxi.nodes_count)
	{
		LOG_ERROR("invalid src node");
		return;
	}
	if (dstid >= map->taxi.nodes_count)
	{
		LOG_ERROR("invalid dst node");
		return;
	}
	struct taxi_node *src = &map->taxi.nodes[srcid];
	struct taxi_node *dst = &map->taxi.nodes[dstid];
	for (size_t i = 0; i < g_wow->map->taxi.nodes_count; ++i)
	{
		map->taxi.nodes[i].pf_visited = false;
		map->taxi.nodes[i].pf_dist = (size_t)-1;
		map->taxi.nodes[i].pf_prev = NULL;
	}
	src->pf_dist = 0;
	setup_pf_dist();
	if (!dst->pf_prev)
	{
		LOG_ERROR("not path found for %u -> %u", srcid, dstid);
		return;
	}
	struct taxi_node *node = dst;
	while (node)
	{
		struct taxi_node **tmp = mem_realloc(MEM_UI, map->taxi.path_nodes, sizeof(*tmp) * (map->taxi.path_nodes_count + 1));
		if (!tmp)
		{
			LOG_ERROR("path nodes allocation failed");
			return;
		}
		map->taxi.path_nodes = tmp;
		map->taxi.path_nodes[g_wow->map->taxi.path_nodes_count] = node;
		map->taxi.path_nodes_count++;
		node = node->pf_prev;
	}
}

bool map_setid(struct map *map, uint32_t id)
{
	if (map->id == id)
		return true;
	gx_skybox_delete(map->gx_skybox);
	map->gx_skybox = NULL;
	gx_wdl_delete(map->gx_wdl);
	map->gx_wdl = NULL;
	for (size_t i = 0; i < map->taxi.nodes_count; ++i)
		mem_free(MEM_UI, map->taxi.nodes[i].links);
	mem_free(MEM_UI, map->taxi.nodes);
	map->taxi.nodes = NULL;
	map->taxi.nodes_count = 0;
	mem_free(MEM_UI, map->taxi.path_nodes);
	map->taxi.path_nodes = NULL;
	map->taxi.path_nodes_count = 0;
	map->id = id;
	for (uint32_t i = 0; i < g_wow->dbc.map->file->header.record_count; ++i)
	{
		struct wow_dbc_row row = dbc_get_row(g_wow->dbc.map, i);
		if (wow_dbc_get_u32(&row, 0) == id)
		{
			mem_free(MEM_GX, map->name);
			map->name = mem_strdup(MEM_GX, wow_dbc_get_str(&row, 4));
			if (!map->name)
			{
				LOG_ERROR("failed to allocate map name");
				return false;
			}
			goto map_found;
		}
	}
	LOG_ERROR("invalid mapid: %u", id);
	map->id = -1;
	mem_free(MEM_GX, map->name);
	map->name = NULL;
	return true;
map_found:
	LOG_INFO("mapid: %u, mapname: %s", id, map->name);
	load_worldmap(map);
	if (!load_taxi(map))
	{
		LOG_ERROR("failed to load taxi nodes");
		return false;
	}
	map->gx_skybox = gx_skybox_new(id);
	if (!map->gx_skybox)
	{
		LOG_ERROR("failed to create skybox renderer");
		return false;
	}
	if (!create_wdt(map))
	{
		LOG_ERROR("failed to create wdt renderer");
		return false;
	}
	if (!create_wdl(map))
	{
		LOG_ERROR("failed to create wdl renderer");
		return false;
	}
#ifdef WITH_DEBUG_RENDERING
	map->gx_taxi = gx_taxi_new(id);
	if (!map->gx_taxi)
	{
		LOG_ERROR("failed to create taxi renderer");
		return false;
	}
#endif
	return true;
}

static void render_skybox(struct map *map)
{
	if (!(g_wow->render_opt & RENDER_OPT_SKYBOX))
		return;
	PERFORMANCE_BEGIN(SKYBOX_RENDER);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->skybox_pipeline_state);
	gx_skybox_render(map->gx_skybox);
	PERFORMANCE_END(SKYBOX_RENDER);
}

static void render_wdl(struct map *map)
{
	if (!(g_wow->render_opt & RENDER_OPT_WDL))
		return;
	PERFORMANCE_BEGIN(WDL_RENDER);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->wdl_pipeline_state);
	gx_wdl_render(map->gx_wdl);
	PERFORMANCE_END(WDL_RENDER);
}

static void render_mcnk(struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & RENDER_OPT_MCNK) || !gx_frame->render_lists.mcnk.size)
		return;
	PERFORMANCE_BEGIN(MCNK_RENDER);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->mcnk_pipeline_state);
	gfx_bind_constant(g_wow->device, 2, &g_wow->draw_frame->mcnk_uniform_buffer, sizeof(struct shader_mcnk_scene_block), 0);
	for (uint32_t i = 0; i < gx_frame->render_lists.mcnk.size; ++i)
		gx_mcnk_render(*JKS_ARRAY_GET(&gx_frame->render_lists.mcnk, i, struct gx_mcnk*));
	PERFORMANCE_END(MCNK_RENDER);
}

static void render_wmo(struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & RENDER_OPT_WMO) || !gx_frame->render_lists.wmo.size)
		return;
	PERFORMANCE_BEGIN(WMO_RENDER);
	gfx_bind_constant(g_wow->device, 2, &g_wow->draw_frame->wmo_uniform_buffer, sizeof(struct shader_wmo_scene_block), 0);
	for (size_t i = 0; i < gx_frame->render_lists.wmo.size; ++i)
		gx_wmo_render(*JKS_ARRAY_GET(&gx_frame->render_lists.wmo, i, struct gx_wmo*));
	PERFORMANCE_END(WMO_RENDER);
}

static void render_opaque_m2(struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & RENDER_OPT_M2) || !gx_frame->render_lists.m2_opaque.size)
		return;
	PERFORMANCE_BEGIN(M2_RENDER);
	gfx_bind_constant(g_wow->device, 2, &g_wow->draw_frame->m2_uniform_buffer, sizeof(struct shader_m2_scene_block), 0);
	for (size_t i = 0; i < gx_frame->render_lists.m2_opaque.size; ++i)
		gx_m2_render(*JKS_ARRAY_GET(&gx_frame->render_lists.m2_opaque, i, struct gx_m2*), false);
	PERFORMANCE_END(M2_RENDER);
}

static void render_transparent_m2(struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & RENDER_OPT_M2) || !gx_frame->render_lists.m2_transparent.size)
		return;
	PERFORMANCE_BEGIN(M2_RENDER);
	gfx_bind_constant(g_wow->device, 2, &g_wow->draw_frame->m2_uniform_buffer, sizeof(struct shader_m2_scene_block), 0);
	for (size_t i = 0; i < gx_frame->render_lists.m2_transparent.size; ++i)
		gx_m2_instance_render(*JKS_ARRAY_GET(&gx_frame->render_lists.m2_transparent, i, struct gx_m2_instance*), true, NULL);
	PERFORMANCE_END(M2_RENDER);
}

static void render_m2_particles(struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & RENDER_OPT_M2_PARTICLES) || !gx_frame->render_lists.m2_particles.size)
		return;
	PERFORMANCE_BEGIN(M2_PARTICLES_RENDER);
	gfx_bind_constant(g_wow->device, 2, &g_wow->draw_frame->particle_uniform_buffer, sizeof(struct shader_particle_scene_block), 0);
	for (size_t i = 0; i < gx_frame->render_lists.m2_particles.size; ++i)
		gx_m2_instance_render_particles(*JKS_ARRAY_GET(&gx_frame->render_lists.m2_particles, i, struct gx_m2_instance*));
	PERFORMANCE_END(M2_PARTICLES_RENDER);
}

static void render_texts(struct gx_frame *gx_frame)
{
	if (!gx_frame->render_lists.text.size)
		return;
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->text_pipeline_state);
	for (size_t i = 0; i < gx_frame->render_lists.text.size; ++i)
		gx_text_render(*JKS_ARRAY_GET(&gx_frame->render_lists.text, i, struct gx_text*));
}

#ifdef WITH_DEBUG_RENDERING
static void render_wmo_aabb(struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & RENDER_OPT_WMO_AABB) || !gx_frame->render_lists.wmo.size)
		return;
	PERFORMANCE_BEGIN(WMO_AABB_RENDER);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->aabb_pipeline_state);
	for (size_t i = 0; i < gx_frame->render_lists.wmo.size; ++i)
		gx_wmo_render_aabb(*JKS_ARRAY_GET(&gx_frame->render_lists.wmo, i, struct gx_wmo*));
	PERFORMANCE_END(WMO_AABB_RENDER);
}

static void render_m2_aabb(struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & RENDER_OPT_M2_AABB) || !gx_frame->render_lists.m2.size)
		return;
	PERFORMANCE_BEGIN(M2_AABB_RENDER);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->aabb_pipeline_state);
	for (size_t i = 0; i < gx_frame->render_lists.m2.size; ++i)
		gx_m2_render_aabb(*JKS_ARRAY_GET(&gx_frame->render_lists.m2, i, struct gx_m2*));
	PERFORMANCE_END(M2_AABB_RENDER);
}

static void render_wdl_aabb(struct map *map)
{
	if (!(g_wow->render_opt & RENDER_OPT_WDL_AABB))
		return;
	PERFORMANCE_BEGIN(WDL_AABB_RENDER);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->aabb_pipeline_state);
	gx_wdl_render_aabb(map->gx_wdl);
	PERFORMANCE_END(WDL_AABB_RENDER);
}

static void render_mcnk_aabb(struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & RENDER_OPT_MCNK_AABB) || !gx_frame->render_lists.mcnk.size)
		return;
	PERFORMANCE_BEGIN(MCNK_AABB_RENDER);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->aabb_pipeline_state);
	for (size_t i = 0; i < gx_frame->render_lists.mcnk.size; ++i)
		gx_mcnk_render_aabb(*JKS_ARRAY_GET(&gx_frame->render_lists.mcnk, i, struct gx_mcnk*));
	PERFORMANCE_END(MCNK_AABB_RENDER);
}

static void render_mclq_aabb(struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & RENDER_OPT_MCLQ_AABB))
		return;
	PERFORMANCE_BEGIN(MCLQ_AABB_RENDER);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->aabb_pipeline_state);
	for (size_t i = 0; i < sizeof(gx_frame->render_lists.mclq) / sizeof(*gx_frame->render_lists.mclq); ++i)
	{
		struct jks_array *render_list = &gx_frame->render_lists.mclq[i];
		for (size_t j = 0; j < render_list->size; ++j)
			gx_mclq_render_aabb(*JKS_ARRAY_GET(render_list, j, struct gx_mclq*));
	}
	PERFORMANCE_END(MCLQ_AABB_RENDER);
}

static void render_wmo_portals(struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & RENDER_OPT_WMO_PORTALS) || !gx_frame->render_lists.wmo.size)
		return;
	PERFORMANCE_BEGIN(WMO_PORTALS_RENDER);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->wmo_portals_pipeline_state);
	for (size_t i = 0; i < gx_frame->render_lists.wmo.size; ++i)
		gx_wmo_render_portals(*JKS_ARRAY_GET(&gx_frame->render_lists.wmo, i, struct gx_wmo*));
	PERFORMANCE_END(WMO_PORTALS_RENDER);
}

static void render_wmo_lights(struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & RENDER_OPT_WMO_LIGHTS) || !gx_frame->render_lists.wmo.size)
		return;
	PERFORMANCE_BEGIN(WMO_LIGHTS_RENDER);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->wmo_lights_pipeline_state);
	for (size_t i = 0; i < gx_frame->render_lists.wmo.size; ++i)
		gx_wmo_render_lights(*JKS_ARRAY_GET(&gx_frame->render_lists.wmo, i, struct gx_wmo*));
	PERFORMANCE_END(WMO_LIGHTS_RENDER);
}

static void render_m2_lights(struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & RENDER_OPT_M2_LIGHTS) || !gx_frame->render_lists.m2.size)
		return;
	PERFORMANCE_BEGIN(M2_LIGHTS_RENDER);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->m2_lights_pipeline_state);
	for (size_t i = 0; i < gx_frame->render_lists.m2.size; ++i)
		gx_m2_render_lights(*JKS_ARRAY_GET(&gx_frame->render_lists.m2, i, struct gx_m2*));
	PERFORMANCE_END(M2_LIGHTS_RENDER);
}

static void render_m2_bones(struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & RENDER_OPT_M2_BONES) || !gx_frame->render_lists.m2.size)
		return;
	PERFORMANCE_BEGIN(M2_BONES_RENDER);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->m2_bones_lines_pipeline_state);
	for (size_t i = 0; i < gx_frame->render_lists.m2.size; ++i)
		gx_m2_render_bones_lines(*JKS_ARRAY_GET(&gx_frame->render_lists.m2, i, struct gx_m2*));
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->m2_bones_points_pipeline_state);
	for (size_t i = 0; i < gx_frame->render_lists.m2.size; ++i)
		gx_m2_render_bones_points(*JKS_ARRAY_GET(&gx_frame->render_lists.m2, i, struct gx_m2*));
	PERFORMANCE_END(M2_BONES_RENDER);
}

static void render_m2_collisions(struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & RENDER_OPT_M2_COLLISIONS) || !gx_frame->render_lists.m2.size)
		return;
	PERFORMANCE_BEGIN(M2_COLLISIONS_RENDER);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->m2_collisions_lines_pipeline_state);
	for (size_t i = 0; i < gx_frame->render_lists.m2.size; ++i)
		gx_m2_render_collisions(*JKS_ARRAY_GET(&gx_frame->render_lists.m2, i, struct gx_m2*), false);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->m2_collisions_triangles_pipeline_state);
	for (size_t i = 0; i < gx_frame->render_lists.m2.size; ++i)
		gx_m2_render_collisions(*JKS_ARRAY_GET(&gx_frame->render_lists.m2, i, struct gx_m2*), true);
	PERFORMANCE_END(M2_COLLISIONS_RENDER);
}

static void render_wmo_collisions(struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & RENDER_OPT_WMO_COLLISIONS) || !gx_frame->render_lists.wmo.size)
		return;
	PERFORMANCE_BEGIN(WMO_COLLISIONS_RENDER);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->wmo_collisions_lines_pipeline_state);
	for (size_t i = 0; i < gx_frame->render_lists.wmo.size; ++i)
		gx_wmo_render_collisions(*JKS_ARRAY_GET(&gx_frame->render_lists.wmo, i, struct gx_wmo*), false);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->wmo_collisions_triangles_pipeline_state);
	for (size_t i = 0; i < gx_frame->render_lists.wmo.size; ++i)
		gx_wmo_render_collisions(*JKS_ARRAY_GET(&gx_frame->render_lists.wmo, i, struct gx_wmo*), true);
	PERFORMANCE_END(WMO_COLLISIONS_RENDER);
}

static void render_collisions(struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & RENDER_OPT_COLLISIONS))
		return;
	PERFORMANCE_BEGIN(COLLISIONS_RENDER);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->collisions_lines_pipeline_state);
	gx_collisions_render(&gx_frame->gx_collisions, false);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->collisions_triangles_pipeline_state);
	gx_collisions_render(&gx_frame->gx_collisions, true);
	PERFORMANCE_END(COLLISIONS_RENDER);
}

static void render_taxi(struct map *map)
{
	if (!(g_wow->render_opt & RENDER_OPT_TAXI))
		return;
	PERFORMANCE_BEGIN(TAXI_RENDER);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->aabb_pipeline_state);
	gx_taxi_render(map->gx_taxi);
	PERFORMANCE_END(TAXI_RENDER);
}
#endif

static void render_wmo_mliq(struct map *map, struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & RENDER_OPT_WMO_LIQUIDS) || !gx_frame->render_lists.wmo.size)
		return;
	PERFORMANCE_BEGIN(WMO_LIQUIDS_RENDER);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->wmo_mliq_pipeline_state);
	gfx_bind_constant(g_wow->device, 2, &g_wow->draw_frame->mliq_uniform_buffer, sizeof(struct shader_mliq_scene_block), 0);
	uint8_t idx = (g_wow->frametime / 30000000) % 30;
	for (size_t type = 0; type < 9; ++type)
	{
		if (!gx_frame->render_lists.wmo_mliq[type].size)
			continue;
		switch (type)
		{
			case 0:
				blp_texture_bind(map->river_textures[idx], 0);
				break;
			case 1:
				blp_texture_bind(map->ocean_textures[idx], 0);
				break;
			case 2:
				if (map->id == 530)
					blp_texture_bind(map->lavag_textures[idx], 0);
				else
					blp_texture_bind(map->magma_textures[idx], 0);
				break;
			case 3:
				blp_texture_bind(map->slime_textures[idx], 0);
				break;
			case 4:
				blp_texture_bind(map->river_textures[idx], 0);
				break;
			case 5:
				continue;
			case 6:
				if (map->id == 530)
					blp_texture_bind(map->lavag_textures[idx], 0);
				else
					blp_texture_bind(map->magma_textures[idx], 0);
				break;
			case 7:
				blp_texture_bind(map->slime_textures[idx], 0);
				break;
			case 8:
				blp_texture_bind(map->fasta_textures[idx], 0);
				break;
			default:
				continue;
		}
		for (size_t i = 0; i < gx_frame->render_lists.wmo_mliq[type].size; ++i)
			gx_wmo_mliq_render(*JKS_ARRAY_GET(&gx_frame->render_lists.wmo_mliq[type], i, struct gx_wmo_mliq*), type);
	}
	PERFORMANCE_END(WMO_LIQUIDS_RENDER);
}

static void render_mclq(struct map *map, struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & RENDER_OPT_MCLQ))
		return;
	PERFORMANCE_BEGIN(MCLQ_RENDER);
	if (g_wow->render_opt & RENDER_OPT_SSR)
	{
		const gfx_texture_t *textures[] =
		{
			&g_wow->post_process.dummy1->color_texture,
			&g_wow->post_process.dummy1->normal_texture,
			&g_wow->post_process.dummy1->position_texture,
		};
		gfx_bind_samplers(g_wow->device, 1, 3, textures);
	}
	uint8_t idx = (g_wow->frametime / 30000000) % 30;
	for (size_t type = 0; type < 4; ++type)
	{
		if (!gx_frame->render_lists.mclq[type].size)
			continue;
		switch (type)
		{
			case 0:
				gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->mclq_water_pipeline_state);
				gfx_bind_constant(g_wow->device, 2, &g_wow->draw_frame->river_uniform_buffer, sizeof(struct shader_mclq_water_scene_block), 0);
				break;
			case 1:
				gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->mclq_water_pipeline_state);
				gfx_bind_constant(g_wow->device, 2, &g_wow->draw_frame->ocean_uniform_buffer, sizeof(struct shader_mclq_water_scene_block), 0);
				break;
			case 2:
			case 3:
				gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->mclq_magma_pipeline_state);
				gfx_bind_constant(g_wow->device, 2, &g_wow->draw_frame->magma_uniform_buffer, sizeof(struct shader_mclq_magma_scene_block), 0);
				break;
		}
		switch (type)
		{
			case 0:
				blp_texture_bind(map->river_textures[idx], 0);
				break;
			case 1:
				blp_texture_bind(map->ocean_textures[idx], 0);
				break;
			case 2:
				if (map->id == 530)
					blp_texture_bind(map->lavag_textures[idx], 0);
				else
					blp_texture_bind(map->magma_textures[idx], 0);
				break;
			case 3:
				blp_texture_bind(map->slime_textures[idx], 0);
				break;
		}
		for (uint32_t i = 0; i < gx_frame->render_lists.mclq[type].size; ++i)
			gx_mclq_render(*JKS_ARRAY_GET(&gx_frame->render_lists.mclq[type], i, struct gx_mclq*), type);
	}
	PERFORMANCE_END(MCLQ_RENDER);
}

static void cull_wmo_portal(struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & (RENDER_OPT_M2 | RENDER_OPT_M2_AABB | RENDER_OPT_WMO | RENDER_OPT_WMO_AABB)))
		return;
	PERFORMANCE_BEGIN(WMO_PORTALS_CULL);
	for (size_t i = 0; i < gx_frame->render_lists.wmo.size; ++i)
		gx_wmo_cull_portal(*JKS_ARRAY_GET(&gx_frame->render_lists.wmo, i, struct gx_wmo*));
	PERFORMANCE_END(WMO_PORTALS_CULL);
}

static void cull_wdl(struct map *map)
{
	PERFORMANCE_BEGIN(WDL_CULL);
	gx_wdl_cull(map->gx_wdl);
	PERFORMANCE_END(WDL_CULL);
}

static void cull_tiles(struct map *map)
{
	PERFORMANCE_BEGIN(ADT_CULL);
	if (!map->has_adt)
	{
		if (map->wmo)
			gx_wmo_instance_add_to_render(map->wmo, true);
		return;
	}
	for (uint32_t i = 0; i < map->tiles_nb; ++i)
		map_tile_cull(map->tile_array[map->tiles[i]]);
	PERFORMANCE_END(ADT_CULL);
}

static void cull_adt_objects(struct gx_frame *gx_frame)
{
	if (!(g_wow->render_opt & (RENDER_OPT_WMO | RENDER_OPT_M2)))
		return;
	PERFORMANCE_BEGIN(ADT_OBJECTS_CULL);
	for (uint32_t i = 0; i < gx_frame->render_lists.mcnk_objects.size; ++i)
		gx_mcnk_add_objects_to_render(*JKS_ARRAY_GET(&gx_frame->render_lists.mcnk_objects, i, struct gx_mcnk*));
	PERFORMANCE_END(ADT_OBJECTS_CULL);
}

static int m2_instance_compare(const void *instance1, const void *instance2)
{
	float d1 = (*(const struct gx_m2_instance**)instance1)->render_frames[g_wow->cull_frame_id].distance_to_camera;
	float d2 = (*(const struct gx_m2_instance**)instance2)->render_frames[g_wow->cull_frame_id].distance_to_camera;
	if (d1 < d2)
		return 1;
	if (d1 > d2)
		return -1;
	return 0;
}

void map_cull(struct map *map, struct gx_frame *gx_frame)
{
	map_load_tick(map);
	cull_wdl(map);
	cull_tiles(map);
	cull_adt_objects(gx_frame);
	cull_wmo_portal(gx_frame);
	render_release_obj(gx_frame);
	if (gx_frame->render_lists.m2_transparent.size)
		qsort(gx_frame->render_lists.m2_transparent.data, gx_frame->render_lists.m2_transparent.size, sizeof(struct gx_m2_instance*), m2_instance_compare);
}

static void update_minimap_texture(struct map *map)
{
	int player_x = floor(((struct worldobj*)g_wow->player)->position.x / 533.3333334f);
	int player_z = floor(((struct worldobj*)g_wow->player)->position.z / 533.3333334f);
	pthread_mutex_lock(&map->minimap.mutex);
	if (!map->minimap.computing && (player_x != map->minimap.last_x || player_z != map->minimap.last_z))
	{
		map->minimap.computing = true;
		map->minimap.last_x = player_x;
		map->minimap.last_z = player_z;
		loader_push(g_wow->loader, ASYNC_TASK_MINIMAP_TEXTURE, NULL);
	}
	if (!map->minimap.data)
	{
		pthread_mutex_unlock(&map->minimap.mutex);
		return;
	}
	gfx_delete_texture(g_wow->device, &map->minimap.texture);
	gfx_create_texture(g_wow->device, &map->minimap.texture, GFX_TEXTURE_2D, GFX_B8G8R8A8, 1, 768, 768, 0);
	gfx_set_texture_levels(&map->minimap.texture, 0, 0);
	gfx_set_texture_anisotropy(&map->minimap.texture, g_wow->anisotropy);
	gfx_set_texture_filtering(&map->minimap.texture, GFX_FILTERING_LINEAR, GFX_FILTERING_LINEAR, GFX_FILTERING_LINEAR);
	gfx_set_texture_addressing(&map->minimap.texture, GFX_TEXTURE_ADDRESSING_REPEAT, GFX_TEXTURE_ADDRESSING_REPEAT, GFX_TEXTURE_ADDRESSING_REPEAT);
	gfx_set_texture_data(&map->minimap.texture, 0, 0, 768, 768, 0, 768 * 768 * 4, map->minimap.data);
	map->minimap.texture_x = map->minimap.data_x;
	map->minimap.texture_z = map->minimap.data_z;
	mem_free(MEM_GENERIC, map->minimap.data);
	map->minimap.data = NULL;
	pthread_mutex_unlock(&map->minimap.mutex);
}

void map_render(struct map *map, struct gx_frame *gx_frame)
{
	if (map->id == (uint32_t)-1)
		return;
	update_minimap_texture(map);
	struct render_pass_def
	{
		struct render_pass *render_pass;
		uint32_t targets;
	} render_passes[20];
	struct render_target *render_target;
	size_t render_passes_nb = 0;
	bool post_process;
	uint32_t render_buffer_bits = RENDER_TARGET_COLOR_BUFFER_BIT;
	if (g_wow->post_process.ssao->enabled)
		render_passes[render_passes_nb++] = (struct render_pass_def){g_wow->post_process.ssao, RENDER_TARGET_NORMAL_BUFFER_BIT | RENDER_TARGET_POSITION_BUFFER_BIT};
	if (g_wow->post_process.sobel->enabled)
		render_passes[render_passes_nb++] = (struct render_pass_def){g_wow->post_process.sobel, RENDER_TARGET_NORMAL_BUFFER_BIT};
	if (g_wow->post_process.glow->enabled)
		render_passes[render_passes_nb++] = (struct render_pass_def){g_wow->post_process.glow, 0};
	if (g_wow->post_process.bloom->enabled)
		render_passes[render_passes_nb++] = (struct render_pass_def){g_wow->post_process.bloom, 0};
	if (g_wow->post_process.cel->enabled)
		render_passes[render_passes_nb++] = (struct render_pass_def){g_wow->post_process.cel, 0};
	if (g_wow->post_process.fxaa->enabled)
		render_passes[render_passes_nb++] = (struct render_pass_def){g_wow->post_process.fxaa, 0};
	if (g_wow->post_process.sharpen->enabled)
		render_passes[render_passes_nb++] = (struct render_pass_def){g_wow->post_process.sharpen, 0};
	if (g_wow->post_process.chromaber->enabled)
		render_passes[render_passes_nb++] = (struct render_pass_def){g_wow->post_process.chromaber, 0};
	if (g_wow->render_opt & RENDER_OPT_SSR)
		render_buffer_bits |= RENDER_TARGET_NORMAL_BUFFER_BIT | RENDER_TARGET_POSITION_BUFFER_BIT;
	for (size_t i = render_passes_nb; i > 0; --i)
	{
		render_buffer_bits |= render_passes[i - 1].targets;
		render_passes[i - 1].targets = render_buffer_bits;
	}
	if (g_wow->post_process.msaa->enabled)
	{
		render_target = g_wow->post_process.msaa;
		render_target_bind(g_wow->post_process.msaa, render_buffer_bits);
		post_process = true;
		/* Force shading of more MSAA samples
		 * Can be almost compared to FSAA when 1
		 * Can be used to mimic CSAA
		 */
		/*glEnable(GL_SAMPLE_SHADING);
		glMinSampleShading(1);*/
		/*glEnable(GL_SAMPLE_COVERAGE);
		glSampleCoverage(.5, false);*/
	}
	else if (render_passes_nb || g_wow->fsaa != 1 || (g_wow->render_opt & RENDER_OPT_SSR))
	{
		render_target = g_wow->post_process.dummy1;
		render_target_bind(g_wow->post_process.dummy1, render_buffer_bits);
		post_process = true;
	}
	else
	{
		render_target = NULL;
		gfx_bind_render_target(g_wow->device, NULL);
		gfx_set_viewport(g_wow->device, 0, 0, g_wow->render_width, g_wow->render_height);
		post_process = false;
	}

	gfx_set_scissor(g_wow->device, 0, 0, g_wow->render_width * g_wow->fsaa, g_wow->render_height * g_wow->fsaa);
	/* XXX: gfx_bind_depth_stencil_state(g_wow->device, &world->depth_stencil_states[WORLD_DEPTH_STENCIL_RW_RW]); */
	if (render_target)
	{
		render_target_clear(render_target, render_buffer_bits);
	}
	else
	{
		gfx_clear_color(g_wow->device, NULL, GFX_RENDERTARGET_ATTACHMENT_COLOR0, (struct vec4f){0, 1, 0, 1});
		gfx_clear_depth_stencil(g_wow->device, NULL, 1, 0);
	}
	gx_skybox_update(map->gx_skybox);
	gx_frame_build_uniform_buffers(gx_frame);
	render_wmo(gx_frame);
	render_opaque_m2(gx_frame);
	render_mcnk(gx_frame);
	/* XXX: SSAO & sobel here (how to handle MSAA / CSAA ?) */
	if (post_process)
	{
		uint32_t draw_buffers[3] = {GFX_RENDERTARGET_ATTACHMENT_COLOR0, GFX_RENDERTARGET_ATTACHMENT_NONE, GFX_RENDERTARGET_ATTACHMENT_NONE};
		gfx_set_render_target_draw_buffers(&render_target->render_target, draw_buffers, 3);
	}
	render_wdl(map);
	render_skybox(map);
#ifdef WITH_DEBUG_RENDERING
	render_wdl_aabb(map);
	render_wmo_aabb(gx_frame);
	render_m2_aabb(gx_frame);
	render_mcnk_aabb(gx_frame);
	render_mclq_aabb(gx_frame);
	render_collisions(gx_frame);
	render_wmo_portals(gx_frame);
	render_wmo_lights(gx_frame);
	render_m2_lights(gx_frame);
#endif
	if (g_wow->render_opt & RENDER_OPT_SSR)
	{
		gfx_bind_render_target(g_wow->device, NULL);
		gfx_set_viewport(g_wow->device, 0, 0, g_wow->render_width, g_wow->render_height);
		gfx_clear_color(g_wow->device, NULL, GFX_RENDERTARGET_ATTACHMENT_COLOR0, (struct vec4f){0, 0, 0, 1});
		gfx_clear_depth_stencil(g_wow->device, NULL, 1, 0);
		render_target_resolve(g_wow->post_process.dummy1, NULL, RENDER_TARGET_COLOR_BUFFER_BIT | RENDER_TARGET_DEPTH_BUFFER_BIT);
		post_process = false;
	}
	render_m2_particles(gx_frame);
	render_wmo_mliq(map, gx_frame);
	render_mclq(map, gx_frame);
	render_transparent_m2(gx_frame);
	render_texts(gx_frame);
#ifdef WITH_DEBUG_RENDERING
	render_wmo_collisions(gx_frame);
	render_m2_collisions(gx_frame);
	render_m2_bones(gx_frame);
	render_taxi(map);
#endif
	gfx_set_scissor(g_wow->device, 0, 0, g_wow->render_width, g_wow->render_height);
	if (!post_process)
		return;
	struct render_target *output = (render_passes_nb || g_wow->fsaa != 1) ? g_wow->post_process.dummy1 : NULL;
	if (g_wow->post_process.msaa->enabled)
		render_target_resolve(g_wow->post_process.msaa, output, render_buffer_bits);
	if (!render_passes_nb)
	{
		if (g_wow->fsaa != 1)
			render_pass_process(g_wow->post_process.fsaa, g_wow->post_process.dummy1, NULL, RENDER_TARGET_COLOR_BUFFER_BIT);
		return;
	}
	struct render_target *target_in = g_wow->post_process.dummy1;
	struct render_target *target_out = g_wow->post_process.dummy2;
	for (size_t i = 0; i < render_passes_nb; ++i)
	{
		struct render_target *out = (i == render_passes_nb - 1) ? NULL : target_out;
		render_pass_process(render_passes[i].render_pass, target_in, out, render_passes[i].targets);
		struct render_target *tmp;
		tmp = target_in;
		target_in = target_out;
		target_out = tmp;
	}
}

void map_collect_collision_triangles(struct map *map, const struct collision_params *params, struct jks_array *triangles)
{
	struct collision_state state;
	jks_array_init(&state.wmo, sizeof(struct gx_wmo_instance*), NULL, &jks_array_memory_fn_GENERIC);
	jks_array_init(&state.m2, sizeof(struct gx_m2_instance*), NULL, &jks_array_memory_fn_GENERIC);
	for (size_t i = 0; i < map->tiles_nb; ++i)
		map_tile_collect_collision_triangles(map->tile_array[map->tiles[i]], params, &state, triangles);
	jks_array_destroy(&state.wmo);
	jks_array_destroy(&state.m2);
}
