#ifndef RENDER_FRAME_H
#define RENDER_FRAME_H

#ifdef WITH_DEBUG_RENDERING
# include "gx/collisions.h"
#endif

#include <jks/frustum.h>
#include <jks/array.h>
#include <jks/mat4.h>
#include <jks/vec3.h>

#include <gfx/objects.h>

#include <stdbool.h>
#include <pthread.h>

struct gx_wmo_instance;
struct gx_m2_instance;
struct gx_wmo_mliq;
struct gx_mclq;
struct gx_mcnk;
struct gx_text;
struct camera;
struct gx_m2;

struct gx_frame
{
	struct jks_array wmo_mliq_render_list[9]; /* struct gx_wmo_mliq* */
	struct jks_array wmo_mliq_backref; /* struct gx_wmo_mliq */
	struct jks_array mclq_render_list[4]; /* struct gx_mclq* */
	struct jks_array wmo_instances_backref; /* struct gx_wmo_instance* */
	struct jks_array m2_instances_backref; /* struct gx_m2_instance* */
	struct jks_array wmo_render_list; /* struct gx_wmo* */
	struct jks_array m2_particles_render_list; /* struct gx_m2_instance* */
	struct jks_array m2_transparent_render_list; /* struct gx_m2_instance* */
	struct jks_array m2_opaque_render_list; /* struct gx_m2* */
	struct jks_array m2_render_list; /* struct gx_m2* */
	struct jks_array mcnk_objects_render_list; /* struct gx_mcnk* */
	struct jks_array mcnk_render_list; /* struct gx_mcnk* */
	struct jks_array text_render_list; /* struct gx_text* */
	struct jks_array text_gc_list; /* struct gx_text* */
	struct jks_array wmo_gc_list; /* struct gx_wmo_list* */
	struct jks_array m2_gc_list; /* struct gx_m2_instance* */
#ifdef WITH_DEBUG_RENDERING
	struct gx_collisions gx_collisions;
#endif
	gfx_buffer_t particle_uniform_buffer;
	gfx_buffer_t river_uniform_buffer;
	gfx_buffer_t ocean_uniform_buffer;
	gfx_buffer_t magma_uniform_buffer;
	gfx_buffer_t mcnk_uniform_buffer;
	gfx_buffer_t mliq_uniform_buffer;
	gfx_buffer_t wmo_uniform_buffer;
	gfx_buffer_t m2_uniform_buffer;
	pthread_mutex_t gc_mutex;
	struct frustum wdl_frustum;
	struct frustum frustum;
	struct mat4f view_wdl_vp;
	struct mat4f view_wdl_p;
	struct vec3f view_pos;
	struct vec3f view_rot;
	struct mat4f view_vp;
	struct mat4f view_v;
	struct mat4f view_p;
	struct vec4f view_bottom;
	struct vec4f view_right;
	struct mat4f cull_wdl_vp;
	struct mat4f cull_wdl_p;
	struct vec3f cull_pos;
	struct vec3f cull_rot;
	struct mat4f cull_vp;
	struct mat4f cull_v;
	struct mat4f cull_p;
	float view_distance;
	float fov;
};

void init_gx_frame(struct gx_frame *gx_frame);
void destroy_gx_frame(struct gx_frame *gx_frame);
void gx_frame_build_uniform_buffers(struct gx_frame *gx_frame);
void render_copy_cameras(struct gx_frame *gx_frame, struct camera *cull_camera, struct camera *view_camera);
void render_clear_scene(struct gx_frame *gx_frame);
void render_release_obj(struct gx_frame *gx_frame);
void render_gc(struct gx_frame *gx_frame);
bool render_gc_m2(struct gx_m2_instance *m2);
bool render_gc_wmo(struct gx_wmo_instance *wmo);
bool render_gc_text(struct gx_text *text);
bool render_add_mcnk_objects(struct gx_mcnk *mcnk);
bool render_add_mcnk(struct gx_mcnk *mcnk);
bool render_add_mclq(uint8_t type, struct gx_mclq *mclq);
bool render_add_wmo(struct gx_wmo_instance *instance, bool bypass_frustum);
bool render_add_wmo_mliq(struct gx_wmo_mliq *mliq, uint8_t liquid);
bool render_add_m2_particles(struct gx_m2_instance *m2);
bool render_add_m2_transparent(struct gx_m2_instance *m2);
bool render_add_m2_opaque(struct gx_m2 *m2);
bool render_add_m2_instance(struct gx_m2_instance *m2, bool bypass_frustum);
bool render_add_text(struct gx_text *text);

#endif
