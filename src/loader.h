#ifndef LOADER_H
#define LOADER_H

#include <stdbool.h>

struct gx_wmo_group;
struct blp_texture;
struct map_tile;
struct gx_wmo;
struct loader;
struct gx_m2;

enum async_task_type /* priority ordered */
{
	ASYNC_TASK_BLP_LOAD,
	ASYNC_TASK_BLP_UNLOAD,
	ASYNC_TASK_MAP_TILE_LOAD,
	ASYNC_TASK_MAP_TILE_UNLOAD,
	ASYNC_TASK_WMO_GROUP_LOAD,
	ASYNC_TASK_WMO_GROUP_UNLOAD,
	ASYNC_TASK_WMO_LOAD,
	ASYNC_TASK_WMO_UNLOAD,
	ASYNC_TASK_M2_LOAD,
	ASYNC_TASK_M2_UNLOAD,
	ASYNC_TASK_MINIMAP_TEXTURE,
	ASYNC_TASK_LAST
};

struct loader *loader_new(void);
void loader_delete(struct loader *loader);
bool loader_has_async(struct loader *loader);
bool loader_has_loading(struct loader *loader);
bool loader_has_gc(struct loader *loader);
void loader_tick(struct loader *loader);
bool loader_push(struct loader *loader, enum async_task_type type, void *data);
bool loader_init_wmo_group(struct loader *loader, struct gx_wmo_group *wmo_group);
bool loader_init_map_tile(struct loader *loader, struct map_tile *map_tile);
bool loader_init_wmo(struct loader *loader, struct gx_wmo *wmo);
bool loader_init_blp(struct loader *loader, struct blp_texture *texture);
bool loader_init_m2(struct loader *loader, struct gx_m2 *m2);
bool loader_gc_wmo_group(struct loader *loader, struct gx_wmo_group *wmo_group);
bool loader_gc_map_tile(struct loader *loader, struct map_tile *map_tile);
bool loader_gc_wmo(struct loader *loader, struct gx_wmo *wmo);
bool loader_gc_blp(struct loader *loader, struct blp_texture *texture);
bool loader_gc_m2(struct loader *loader, struct gx_m2 *m2);
void loader_start_cull(struct loader *loader);
void loader_wait_cull(struct loader *loader);

#endif
