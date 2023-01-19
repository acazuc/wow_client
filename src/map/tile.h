#ifndef MAP_TILE_H
#define MAP_TILE_H

#include <jks/vec3.h>
#include <jks/mat4.h>

#include <stdbool.h>
#include <stdint.h>

#define MAP_TILE_INIT           (1 << 0)
#define MAP_TILE_LOADING        (1 << 1)
#define MAP_TILE_LOADED         (1 << 2)
#define MAP_TILE_IN_RENDER_LIST (1 << 3)

struct collision_params;
struct collision_state;
struct gx_wmo_instance;
struct gx_m2_instance;
struct wow_adt_file;
struct jks_array;
struct gx_mcnk;
struct gx_mclq;

struct map_wmo
{
	struct gx_wmo_instance *instance;
};

struct map_m2
{
	struct gx_m2_instance *instance;
};

struct map_tile
{
	struct map_wmo **wmo;
	uint32_t wmo_nb;
	struct map_m2 **m2;
	uint32_t m2_nb;
	struct gx_mcnk *gx_mcnk;
	struct gx_mclq *gx_mclq;
	uint8_t flags;
	char *filename;
	struct vec3f pos;
	struct mat4f mvp;
	int32_t x;
	int32_t z;
};

struct map_tile *map_tile_new(const char *filename, int32_t x, int32_t z);
void map_tile_delete(struct map_tile *tile);
void map_tile_ask_load(struct map_tile *tile);
void map_tile_ask_unload(struct map_tile *tile);
bool map_tile_load(struct map_tile *tile, struct wow_adt_file *file);
bool map_tile_load_childs(struct map_tile *tile, struct wow_adt_file *file);
int map_tile_initialize(struct map_tile *tile);
void map_tile_cull(struct map_tile *tile);
void map_tile_collect_collision_triangles(struct map_tile *tile, const struct collision_params *params, struct collision_state *state, struct jks_array *triangles);

struct map_wmo *map_wmo_new(void);
void map_wmo_delete(struct map_wmo *handle);
void map_wmo_load(struct map_wmo *handle, const char *filename);

struct map_m2 *map_m2_new(void);
void map_m2_delete(struct map_m2 *handle);
void map_m2_load(struct map_m2 *handle, const char *filename);

#endif
