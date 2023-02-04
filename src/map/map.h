#ifndef MAP_H
#define MAP_H

#include <gfx/objects.h>

#include <jks/array.h>
#include <jks/aabb.h>
#include <jks/vec3.h>

#include <libwow/dbc.h>

#include <stdbool.h>
#include <pthread.h>

struct taxi_node;
struct gx_skybox;
struct gx_frame;
struct gx_taxi;
struct gx_wdl;

struct taxi_link
{
	struct taxi_node *dst;
	uint32_t price;
};

struct taxi_node
{
	struct wow_dbc_row dbc;
	struct taxi_link *links;
	size_t links_count;
	size_t pf_dist;
	size_t pf_price;
	struct taxi_node *pf_prev;
	bool pf_visited;
};

struct minimap
{
	gfx_texture_t texture;
	pthread_mutex_t mutex;
	uint8_t *data;
	int texture_x;
	int texture_z;
	int data_x;
	int data_z;
	int last_x;
	int last_z;
	bool computing;
};

struct worldmap
{
	float minx;
	float miny;
	float maxx;
	float maxy;
	float width;
	float height;
};

struct taxi_info
{
	struct taxi_node *nodes;
	size_t nodes_count;
	struct taxi_node **path_nodes;
	size_t path_nodes_count;
	uint32_t path_src;
	uint32_t path_dst;
};

struct map
{
	struct blp_texture *lavag_textures[30];
	struct blp_texture *river_textures[30];
	struct blp_texture *ocean_textures[30];
	struct blp_texture *magma_textures[30];
	struct blp_texture *slime_textures[30];
	struct blp_texture *fasta_textures[30];
	struct map_tile *tile_array[64 * 64];
	uint16_t tiles[64 * 64]; /* List of loaded adts */
	uint32_t tiles_nb;
	struct minimap minimap;
#ifdef WITH_DEBUG_RENDERING
	struct gx_taxi *gx_taxi;
#endif
	struct gx_skybox *gx_skybox;
	struct gx_wdl *gx_wdl;
	char *name;
	uint32_t id;
	float fog_divisor;
	uint64_t adt_exists[64];
	gfx_buffer_t particles_indices_buffer; /* XXX move somewhere else */
	gfx_buffer_t ribbons_indices_buffer; /* XXX move somewhere else */
	gfx_buffer_t mcnk_vertexes_buffer; /* XXX move somewhere else */
	gfx_buffer_t mcnk_indices_buffer;
	struct gx_wmo_instance *wmo;
	struct vec3f last_pos;
	uint32_t mcnk_indices_nb;
	uint32_t flags;
	int64_t last_check;
	float last_view_distance;
	struct worldmap worldmap;
	struct taxi_info taxi;
	bool has_adt;
};

struct collision_params
{
	struct aabb aabb;
	struct vec3f center;
	float radius;
	bool wmo_cam;
};

struct collision_triangle
{
	struct vec3f points[3];
	bool touched;
};

struct collision_state
{
	struct jks_array wmo;
	struct jks_array m2;
};

struct map *map_new(void);
void map_delete(struct map *map);
bool map_setid(struct map *map, uint32_t mapid);
void map_cull(struct map *map, struct gx_frame *gx_frame);
void map_render(struct map *map, struct gx_frame *gx_frame);
bool generate_minimap_texture(struct map *map);
void map_gen_taxi_path(struct map *map, uint32_t src, uint32_t dst);
void map_collect_collision_triangles(struct map *map, const struct collision_params *param, struct jks_array *triangles);

#endif
