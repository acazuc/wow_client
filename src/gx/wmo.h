#ifndef GX_WMO_H
#define GX_WMO_H

#ifdef WITH_DEBUG_RENDERING
# include "gx/wmo_portals.h"
# include "gx/wmo_lights.h"
# include "gx/aabb.h"
#endif

#include <jks/frustum.h>
#include <jks/array.h>
#include <jks/vec4.h>
#include <jks/aabb.h>

#include <libwow/wmo.h>

#include <gfx/objects.h>

#include <stdbool.h>

struct gx_wmo_instance;
struct gx_wmo_group;

struct gx_wmo_frame
{
	struct jks_array to_render; /* struct gx_wmo_instance* */
};

struct gx_wmo
{
	struct gx_wmo_frame render_frames[RENDER_FRAMES_COUNT];
	struct jks_array instances; /* struct gx_wmo_instance* */
	struct jks_array groups; /* struct gx_wmo_group* */
	struct jks_array modd; /* struct wow_modd_data */
	struct jks_array mods; /* struct wow_mods_data */
	struct jks_array molt; /* struct wow_molt_data */
	struct jks_array mopt; /* struct wow_mopt_data */
	struct jks_array mopr; /* struct wow_mopr_data */
	struct jks_array momt; /* struct wow_momt_data */
	struct jks_array mopv; /* struct wow_vec3f */
	struct jks_array modn; /* char */
	struct jks_array motx; /* char */
#ifdef WITH_DEBUG_RENDERING
	struct gx_wmo_portals gx_portals;
	struct gx_wmo_lights gx_lights;
#endif
	bool load_asked;
	bool loading;
	bool loaded;
	bool initialized;
	char *filename;
	struct vec4f ambient;
	struct aabb aabb;
	uint32_t flags;
	bool in_render_list;
};

struct gx_wmo *gx_wmo_new(const char *filename);
void gx_wmo_delete(struct gx_wmo *wmo);
void gx_wmo_ask_load(struct gx_wmo *wmo);
void gx_wmo_ask_unload(struct gx_wmo *wmo);
bool gx_wmo_load(struct gx_wmo *wmo, struct wow_wmo_file *file);
int gx_wmo_initialize(struct gx_wmo *wmo);
void gx_wmo_clear_update(struct gx_wmo *wmo);
void gx_wmo_cull_portal(struct gx_wmo *wmo);
void gx_wmo_render(struct gx_wmo *wmo);
#ifdef WITH_DEBUG_RENDERING
void gx_wmo_render_aabb(struct gx_wmo *wmo);
void gx_wmo_render_portals(struct gx_wmo *wmo);
void gx_wmo_render_lights(struct gx_wmo *wmo);
void gx_wmo_render_collisions(struct gx_wmo *wmo, bool triangles);
#endif

struct gx_wmo_batch_instance_frame
{
	bool culled;
};

struct gx_wmo_batch_instance
{
	struct gx_wmo_batch_instance_frame render_frames[RENDER_FRAMES_COUNT];
#ifdef WITH_DEBUG_RENDERING
	struct gx_aabb gx_aabb;
#endif
	struct aabb aabb;
};

void gx_wmo_batch_instance_init(struct gx_wmo_batch_instance *instance);
void gx_wmo_batch_instance_destroy(struct gx_wmo_batch_instance *instance);
#ifdef WITH_DEBUG_RENDERING
void gx_wmo_batch_instance_render_aabb(struct gx_wmo_batch_instance *instance, const struct mat4f *mvp);
#endif

struct gx_wmo_group_instance_frame
{
	bool cull_source;
	bool culled;
};

struct gx_wmo_group_instance
{
	struct gx_wmo_group_instance_frame render_frames[RENDER_FRAMES_COUNT];
	struct jks_array batches; /* struct gx_wmo_batch_instance */
#ifdef WITH_DEBUG_RENDERING
	struct gx_aabb gx_aabb;
#endif
	struct aabb aabb;
};

void gx_wmo_group_instance_init(struct gx_wmo_group_instance *instance);
void gx_wmo_group_instance_destroy(struct gx_wmo_group_instance *instance);
bool gx_wmo_group_instance_on_load(struct gx_wmo_instance *instance, struct gx_wmo_group *group, struct gx_wmo_group_instance *group_instance);

struct gx_wmo_instance_frame
{
	struct mat4f mvp;
	struct mat4f mv;
	float distance_to_camera;
	bool culled;
};

struct gx_wmo_instance
{
	struct gx_wmo_instance_frame render_frames[RENDER_FRAMES_COUNT];
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	struct jks_array groups; /* struct gx_wmo_group_instance */
	struct jks_array m2; /* struct gx_m2_instance* */
	uint8_t *traversed_portals; /* bitmask */
#ifdef WITH_DEBUG_RENDERING
	struct gx_aabb gx_aabb;
#endif
	struct gx_wmo *parent;
	struct frustum frustum;
	struct aabb aabb;
	struct mat4f m_inv;
	struct mat4f m;
	struct vec3f pos;
	uint16_t doodad_start;
	uint16_t doodad_end;
	uint16_t doodad_set;
	bool in_render_list;
};

struct gx_wmo_instance *gx_wmo_instance_new(const char *filename);
void gx_wmo_instance_delete(struct gx_wmo_instance *instance);
void gx_wmo_instance_gc(struct gx_wmo_instance *instance);
void gx_wmo_instance_on_load(struct gx_wmo_instance *instance);
void gx_wmo_instance_load_doodad_set(struct gx_wmo_instance *instance);
void gx_wmo_instance_cull_portal(struct gx_wmo_instance *instance);
void gx_wmo_instance_update(struct gx_wmo_instance *instance, bool bypass_frustum);
void gx_wmo_instance_update_aabb(struct gx_wmo_instance *instance);
void gx_wmo_instance_add_to_render(struct gx_wmo_instance *instance, bool bypass_frustum);
void gx_wmo_instance_calculate_distance_to_camera(struct gx_wmo_instance *instance);
void gx_wmo_instance_set_mat(struct gx_wmo_instance *instance, const struct mat4f *mat);

#endif
