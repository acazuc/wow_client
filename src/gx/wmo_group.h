#ifndef GX_WMO_GROUP_H
#define GX_WMO_GROUP_H

#ifdef WITH_DEBUG_RENDERING
# include "gx/wmo_collisions.h"
#endif

#include <jks/array.h>
#include <jks/aabb.h>
#include <jks/vec2.h>

#include <gfx/objects.h>

#include <stdbool.h>

struct gx_wmo_group_init_data;
struct wow_wmo_group_file;
struct gx_wmo_instance;
struct wow_moba_data;
struct gx_wmo_group;
struct gx_wmo_mliq;
struct blp_texture;
struct frustum;
struct gx_wmo;

struct gx_wmo_batch
{
	struct gx_wmo_group *parent;
	uint32_t pipeline_state;
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	char *texture_name;
	struct blp_texture *texture1;
	struct blp_texture *texture2;
	struct aabb aabb;
	uint32_t indices_offset;
	uint32_t indices_nb;
	uint32_t flags1;
	uint32_t flags2;
	uint32_t shader;
	float alpha_test;
	bool fogged;
};

struct gx_wmo_group
{
	struct gx_wmo_group_init_data *init_data;
	struct jks_array batches; /* struct gx_wmo_batch */
	struct jks_array doodads; /* uint16_t */
	struct jks_array lights; /* uint16_t */
	struct jks_array mobn; /* struct wow_mobn_node */
	struct jks_array mobr; /* uint16_t */
	struct jks_array movi; /* uint16_t */
	struct jks_array movt; /* struct wow_vec3f */
	struct jks_array mopy; /* struct wow_mopy_data */
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t vertexes_buffer;
	gfx_buffer_t indices_buffer;
	gfx_buffer_t colors_buffer;
#ifdef WITH_DEBUG_RENDERING
	struct gx_wmo_collisions gx_collisions;
#endif
	struct gx_wmo_mliq *gx_mliq;
	struct gx_wmo *parent;
	struct aabb aabb;
	uint16_t portal_start;
	uint16_t portal_count;
	uint32_t flags;
	uint32_t index;
	char *filename;
	bool initialized;
	bool has_colors;
	bool loading;
	bool loaded;
};

struct gx_wmo_group *gx_wmo_group_new(struct gx_wmo *parent, uint32_t index, uint32_t flags);
void gx_wmo_group_delete(struct gx_wmo_group *group);
bool gx_wmo_group_load(struct gx_wmo_group *group, struct wow_wmo_group_file *file);
int gx_wmo_group_initialize(struct gx_wmo_group *group);
void gx_wmo_group_render(struct gx_wmo_group *group, struct jks_array *instances);
void gx_wmo_group_cull_portal(struct gx_wmo_group *group, struct gx_wmo_instance *instance, struct vec4f *rpos);
void gx_wmo_group_set_m2_ambient(struct gx_wmo_group *group, struct gx_wmo_instance *instance);

#endif
