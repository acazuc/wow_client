#include "gx/wmo_mliq.h"
#include "gx/skybox.h"
#include "gx/frame.h"
#include "gx/mcnk.h"
#include "gx/mclq.h"
#include "gx/text.h"
#include "gx/wmo.h"
#include "gx/m2.h"

#include "map/map.h"

#include "shaders.h"
#include "camera.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <gfx/device.h>

#include <math.h>

MEMORY_DECL(GX);

static const struct vec4f light_direction = {-1, -1, 1, 0};

void init_gx_frame(struct gx_frame *gx_frame)
{
	for (size_t i = 0; i < sizeof(gx_frame->render_lists.mclq) / sizeof(*gx_frame->render_lists.mclq); ++i)
		jks_array_init(&gx_frame->render_lists.mclq[i], sizeof(struct gx_mclq*), NULL, &jks_array_memory_fn_GX);
	for (size_t i = 0; i < sizeof(gx_frame->render_lists.wmo_mliq) / sizeof(*gx_frame->render_lists.wmo_mliq); ++i)
		jks_array_init(&gx_frame->render_lists.wmo_mliq[i], sizeof(struct gx_wmo_mliq*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&gx_frame->render_lists.m2, sizeof(struct gx_m2*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&gx_frame->render_lists.m2_opaque, sizeof(struct gx_m2*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&gx_frame->render_lists.mcnk, sizeof(struct gx_mcnk*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&gx_frame->render_lists.mcnk_objects, sizeof(struct gx_mcnk*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&gx_frame->render_lists.m2_particles, sizeof(struct gx_m2_instance*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&gx_frame->render_lists.m2_ribbons, sizeof(struct gx_m2_instance*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&gx_frame->render_lists.m2_transparent, sizeof(struct gx_m2_instance*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&gx_frame->render_lists.wmo, sizeof(struct gx_wmo*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&gx_frame->render_lists.text, sizeof(struct gx_text*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&gx_frame->backrefs.m2, sizeof(struct gx_m2_instance*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&gx_frame->backrefs.wmo, sizeof(struct gx_wmo_instance*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&gx_frame->backrefs.wmo_mliq, sizeof(struct gx_wmo_mliq*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&gx_frame->gc_lists.text, sizeof(struct gx_text*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&gx_frame->gc_lists.wmo, sizeof(struct gx_wmo_instance*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&gx_frame->gc_lists.m2, sizeof(struct gx_m2_instance*), NULL, &jks_array_memory_fn_GX);
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&gx_frame->gc_mutex, &attr);
	pthread_mutexattr_destroy(&attr);
	gx_frame->particle_uniform_buffer = GFX_BUFFER_INIT();
	gx_frame->ribbon_uniform_buffer = GFX_BUFFER_INIT();
	gx_frame->river_uniform_buffer = GFX_BUFFER_INIT();
	gx_frame->ocean_uniform_buffer = GFX_BUFFER_INIT();
	gx_frame->magma_uniform_buffer = GFX_BUFFER_INIT();
	gx_frame->mcnk_uniform_buffer = GFX_BUFFER_INIT();
	gx_frame->mliq_uniform_buffer = GFX_BUFFER_INIT();
	gx_frame->wmo_uniform_buffer = GFX_BUFFER_INIT();
	gx_frame->m2_uniform_buffer = GFX_BUFFER_INIT();
	gfx_create_buffer(g_wow->device, &gx_frame->particle_uniform_buffer, GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_particle_scene_block), GFX_BUFFER_STREAM);
	gfx_create_buffer(g_wow->device, &gx_frame->ribbon_uniform_buffer, GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_ribbon_scene_block), GFX_BUFFER_STREAM);
	gfx_create_buffer(g_wow->device, &gx_frame->river_uniform_buffer, GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_mclq_water_scene_block), GFX_BUFFER_STREAM);
	gfx_create_buffer(g_wow->device, &gx_frame->ocean_uniform_buffer, GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_mclq_water_scene_block), GFX_BUFFER_STREAM);
	gfx_create_buffer(g_wow->device, &gx_frame->magma_uniform_buffer, GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_mclq_magma_scene_block), GFX_BUFFER_STREAM);
	gfx_create_buffer(g_wow->device, &gx_frame->mcnk_uniform_buffer, GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_mcnk_scene_block), GFX_BUFFER_STREAM);
	gfx_create_buffer(g_wow->device, &gx_frame->mliq_uniform_buffer, GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_mliq_scene_block), GFX_BUFFER_STREAM);
	gfx_create_buffer(g_wow->device, &gx_frame->wmo_uniform_buffer, GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_wmo_scene_block), GFX_BUFFER_STREAM);
	gfx_create_buffer(g_wow->device, &gx_frame->m2_uniform_buffer, GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_m2_scene_block), GFX_BUFFER_STREAM);
#ifdef WITH_DEBUG_RENDERING
	gx_collisions_init(&gx_frame->gx_collisions);
#endif
}

void destroy_gx_frame(struct gx_frame *gx_frame)
{
#ifdef WITH_DEBUG_RENDERING
	gx_collisions_destroy(&gx_frame->gx_collisions);
#endif
	gfx_delete_buffer(g_wow->device, &gx_frame->particle_uniform_buffer);
	gfx_delete_buffer(g_wow->device, &gx_frame->ribbon_uniform_buffer);
	gfx_delete_buffer(g_wow->device, &gx_frame->river_uniform_buffer);
	gfx_delete_buffer(g_wow->device, &gx_frame->ocean_uniform_buffer);
	gfx_delete_buffer(g_wow->device, &gx_frame->magma_uniform_buffer);
	gfx_delete_buffer(g_wow->device, &gx_frame->mcnk_uniform_buffer);
	gfx_delete_buffer(g_wow->device, &gx_frame->mliq_uniform_buffer);
	gfx_delete_buffer(g_wow->device, &gx_frame->wmo_uniform_buffer);
	gfx_delete_buffer(g_wow->device, &gx_frame->m2_uniform_buffer);
	jks_array_destroy(&gx_frame->render_lists.m2);
	jks_array_destroy(&gx_frame->render_lists.m2_opaque);
	jks_array_destroy(&gx_frame->render_lists.mcnk);
	jks_array_destroy(&gx_frame->render_lists.mcnk_objects);
	jks_array_destroy(&gx_frame->render_lists.m2_particles);
	jks_array_destroy(&gx_frame->render_lists.m2_ribbons);
	jks_array_destroy(&gx_frame->render_lists.m2_transparent);
	jks_array_destroy(&gx_frame->render_lists.wmo);
	jks_array_destroy(&gx_frame->render_lists.text);
	for (size_t i = 0; i < sizeof(gx_frame->render_lists.mclq) / sizeof(*gx_frame->render_lists.mclq); ++i)
		jks_array_destroy(&gx_frame->render_lists.mclq[i]);
	for (size_t i = 0; i < sizeof(gx_frame->render_lists.wmo_mliq) / sizeof(*gx_frame->render_lists.wmo_mliq); ++i)
		jks_array_destroy(&gx_frame->render_lists.wmo_mliq[i]);
	jks_array_destroy(&gx_frame->backrefs.m2);
	jks_array_destroy(&gx_frame->backrefs.wmo);
	jks_array_destroy(&gx_frame->backrefs.wmo_mliq);
	jks_array_destroy(&gx_frame->gc_lists.m2);
	jks_array_destroy(&gx_frame->gc_lists.wmo);
	jks_array_destroy(&gx_frame->gc_lists.text);
	pthread_mutex_destroy(&gx_frame->gc_mutex);
}

#define COPY_VEC3_A1(dst, src) \
do \
{ \
	VEC3_CPY(dst, src); \
	dst.w = 1; \
} while (0)

static void build_particle_uniform_buffer(struct gx_frame *gx_frame)
{
	struct shader_particle_scene_block scene_block;
	scene_block.fog_range.y = g_wow->draw_frame->view_distance * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_END] / 36 / g_wow->map->fog_divisor;
	scene_block.fog_range.x = scene_block.fog_range.y * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_START];
	gfx_set_buffer_data(&gx_frame->particle_uniform_buffer, &scene_block, sizeof(scene_block), 0);
}

static void build_ribbon_uniform_buffer(struct gx_frame *gx_frame)
{
	struct shader_ribbon_scene_block scene_block;
	scene_block.fog_range.y = g_wow->draw_frame->view_distance * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_END] / 36 / g_wow->map->fog_divisor;
	scene_block.fog_range.x = scene_block.fog_range.y * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_START];
	gfx_set_buffer_data(&gx_frame->ribbon_uniform_buffer, &scene_block, sizeof(scene_block), 0);
}

static void build_ocean_uniform_buffer(struct gx_frame *gx_frame)
{
	struct shader_mclq_water_scene_block scene_block;
	VEC4_CPY(scene_block.light_direction, light_direction);
	COPY_VEC3_A1(scene_block.specular_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_SUN]);
	COPY_VEC3_A1(scene_block.diffuse_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_DIFFUSE]);
	COPY_VEC3_A1(scene_block.final_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_OCEAN2]);
	COPY_VEC3_A1(scene_block.base_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_OCEAN1]);
	COPY_VEC3_A1(scene_block.fog_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_FOG]);
	scene_block.alphas.x = g_wow->map->gx_skybox->ocean_min_alpha;
	scene_block.alphas.y = g_wow->map->gx_skybox->ocean_max_alpha;
	scene_block.fog_range.y = g_wow->draw_frame->view_distance * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_END] / 36 / g_wow->map->fog_divisor;
	scene_block.fog_range.x = scene_block.fog_range.y * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_START];
	gfx_set_buffer_data(&gx_frame->ocean_uniform_buffer, &scene_block, sizeof(scene_block), 0);
}

static void build_river_uniform_buffer(struct gx_frame *gx_frame)
{
	struct shader_mclq_water_scene_block scene_block;
	VEC4_CPY(scene_block.light_direction, light_direction);
	COPY_VEC3_A1(scene_block.specular_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_SUN]);
	COPY_VEC3_A1(scene_block.diffuse_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_DIFFUSE]);
	COPY_VEC3_A1(scene_block.base_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_RIVER1]);
	COPY_VEC3_A1(scene_block.final_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_RIVER2]);
	COPY_VEC3_A1(scene_block.fog_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_FOG]);
	scene_block.alphas.x = g_wow->map->gx_skybox->river_min_alpha;
	scene_block.alphas.y = g_wow->map->gx_skybox->river_max_alpha;
	scene_block.fog_range.y = g_wow->draw_frame->view_distance * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_END] / 36 / g_wow->map->fog_divisor;
	scene_block.fog_range.x = scene_block.fog_range.y * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_START];
	gfx_set_buffer_data(&gx_frame->river_uniform_buffer, &scene_block, sizeof(scene_block), 0);
}

static void build_magma_uniform_buffer(struct gx_frame *gx_frame)
{
	struct shader_mclq_magma_scene_block scene_block;
	COPY_VEC3_A1(scene_block.diffuse_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_DIFFUSE]);
	COPY_VEC3_A1(scene_block.fog_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_FOG]);
	scene_block.fog_range.y = g_wow->draw_frame->view_distance * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_END] / 36 / g_wow->map->fog_divisor;
	scene_block.fog_range.x = scene_block.fog_range.y * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_START];
	gfx_set_buffer_data(&gx_frame->magma_uniform_buffer, &scene_block, sizeof(scene_block), 0);
}

static void build_mcnk_uniform_buffer(struct gx_frame *gx_frame)
{
	struct shader_mcnk_scene_block scene_block;
	VEC4_CPY(scene_block.light_direction, light_direction);
	COPY_VEC3_A1(scene_block.specular_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_SUN]);
	COPY_VEC3_A1(scene_block.ambient_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_AMBIENT]);
	COPY_VEC3_A1(scene_block.diffuse_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_DIFFUSE]);
	COPY_VEC3_A1(scene_block.fog_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_FOG]);
	scene_block.fog_range.y = g_wow->draw_frame->view_distance * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_END] / 36 / g_wow->map->fog_divisor;
	scene_block.fog_range.x = scene_block.fog_range.y * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_START];
	gfx_set_buffer_data(&gx_frame->mcnk_uniform_buffer, &scene_block, sizeof(scene_block), 0);
}

static void build_mliq_uniform_buffer(struct gx_frame *gx_frame)
{
	struct shader_mliq_scene_block scene_block;
	VEC4_CPY(scene_block.light_direction, light_direction);
	COPY_VEC3_A1(scene_block.specular_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_SUN]);
	COPY_VEC3_A1(scene_block.diffuse_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_DIFFUSE]);
	COPY_VEC3_A1(scene_block.final_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_OCEAN2]);
	COPY_VEC3_A1(scene_block.base_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_OCEAN1]);
	COPY_VEC3_A1(scene_block.fog_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_FOG]);
	scene_block.fog_range.y = g_wow->draw_frame->view_distance * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_END] / 36 / g_wow->map->fog_divisor;
	scene_block.fog_range.x = scene_block.fog_range.y * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_START];
	gfx_set_buffer_data(&gx_frame->mliq_uniform_buffer, &scene_block, sizeof(scene_block), 0);
}

static void build_wmo_uniform_buffer(struct gx_frame *gx_frame)
{
	struct shader_wmo_scene_block scene_block;
	VEC4_CPY(scene_block.light_direction, light_direction);
	COPY_VEC3_A1(scene_block.specular_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_SUN]);
	COPY_VEC3_A1(scene_block.diffuse_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_DIFFUSE]);
	COPY_VEC3_A1(scene_block.ambient_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_AMBIENT]);
	COPY_VEC3_A1(scene_block.fog_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_FOG]);
	scene_block.fog_range.y = g_wow->draw_frame->view_distance * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_END] / 36 / g_wow->map->fog_divisor;
	scene_block.fog_range.x = scene_block.fog_range.y * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_START];
	gfx_set_buffer_data(&gx_frame->wmo_uniform_buffer, &scene_block, sizeof(scene_block), 0);
}

static void build_m2_uniform_buffer(struct gx_frame *gx_frame)
{
	struct shader_m2_scene_block scene_block;
	VEC4_CPY(scene_block.light_direction, light_direction);
	COPY_VEC3_A1(scene_block.specular_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_SUN]);
	COPY_VEC3_A1(scene_block.diffuse_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_DIFFUSE]);
	COPY_VEC3_A1(scene_block.ambient_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_AMBIENT]);
	scene_block.fog_range.y = g_wow->draw_frame->view_distance * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_END] / 36 / g_wow->map->fog_divisor;
	scene_block.fog_range.x = scene_block.fog_range.y * g_wow->map->gx_skybox->float_values[SKYBOX_FLOAT_FOG_START];
	gfx_set_buffer_data(&gx_frame->m2_uniform_buffer, &scene_block, sizeof(scene_block), 0);
}

#undef COPY_VEC3_A1

void gx_frame_build_uniform_buffers(struct gx_frame *gx_frame)
{
	build_particle_uniform_buffer(gx_frame);
	build_ribbon_uniform_buffer(gx_frame);
	build_ocean_uniform_buffer(gx_frame);
	build_river_uniform_buffer(gx_frame);
	build_magma_uniform_buffer(gx_frame);
	build_mcnk_uniform_buffer(gx_frame);
	build_mliq_uniform_buffer(gx_frame);
	build_wmo_uniform_buffer(gx_frame);
	build_m2_uniform_buffer(gx_frame);
}

void render_copy_cameras(struct gx_frame *gx_frame, struct camera *cull_camera, struct camera *view_camera)
{
	gx_frame->wdl_frustum = cull_camera->wdl_frustum;
	gx_frame->frustum = cull_camera->frustum;
	gx_frame->view_wdl_vp = view_camera->wdl_vp;
	gx_frame->view_wdl_p = view_camera->wdl_p;
	gx_frame->view_pos = view_camera->pos;
	gx_frame->view_rot = view_camera->rot;
	gx_frame->view_vp = view_camera->vp;
	gx_frame->view_v = view_camera->v;
	gx_frame->view_p = view_camera->p;
	gx_frame->cull_wdl_vp = cull_camera->wdl_vp;
	gx_frame->cull_wdl_p = cull_camera->wdl_p;
	gx_frame->cull_pos = cull_camera->pos;
	gx_frame->cull_rot = cull_camera->rot;
	gx_frame->cull_vp = cull_camera->vp;
	gx_frame->cull_v = cull_camera->v;
	gx_frame->cull_p = cull_camera->p;
	gx_frame->view_distance = cull_camera->view_distance;
	gx_frame->fov = view_camera->fov;
	const struct vec4f right = {1, 0, 0, 0};
	const struct vec4f bottom = {0, -1, 0, 0};
	struct mat4f tmp1;
	struct mat4f tmp2;
	MAT4_IDENTITY(tmp1);
	MAT4_ROTATEZ(float, tmp2, tmp1, -view_camera->rot.z);
	MAT4_ROTATEY(float, tmp1, tmp2, -view_camera->rot.y);
	MAT4_ROTATEX(float, tmp2, tmp1, -view_camera->rot.x);
	MAT4_VEC4_MUL(gx_frame->view_right, tmp2, right);
	MAT4_VEC4_MUL(gx_frame->view_bottom, tmp2, bottom);
	gx_frame->m2_params.vp = gx_frame->view_vp;
	gx_frame->m2_params.v = gx_frame->view_v;
	gx_frame->m2_params.p = gx_frame->view_p;
	VEC3_CPY(gx_frame->m2_params.fog_color, g_wow->map->gx_skybox->int_values[SKYBOX_INT_FOG]);
	VEC3_CPY(gx_frame->m2_params.view_right, gx_frame->view_right);
	VEC3_CPY(gx_frame->m2_params.view_bottom, gx_frame->view_bottom);
}

void render_clear_scene(struct gx_frame *gx_frame)
{
	jks_array_resize(&gx_frame->render_lists.m2_particles, 0);
	jks_array_resize(&gx_frame->render_lists.m2_ribbons, 0);
	jks_array_resize(&gx_frame->render_lists.m2_transparent, 0);
	jks_array_resize(&gx_frame->render_lists.m2_opaque, 0);
	for (size_t i = 0; i < gx_frame->render_lists.m2.size; ++i)
		gx_m2_clear_update(*JKS_ARRAY_GET(&gx_frame->render_lists.m2, i, struct gx_m2*));
	jks_array_resize(&gx_frame->render_lists.m2, 0);
	jks_array_resize(&gx_frame->render_lists.mcnk, 0);
	jks_array_resize(&gx_frame->render_lists.mcnk_objects, 0);
	for (size_t type = 0; type < sizeof(gx_frame->render_lists.mclq) / sizeof(*gx_frame->render_lists.mclq); ++type)
		jks_array_resize(&gx_frame->render_lists.mclq[type], 0);
	for (size_t i = 0; i < gx_frame->render_lists.wmo.size; ++i)
		gx_wmo_clear_update(*JKS_ARRAY_GET(&gx_frame->render_lists.wmo, i, struct gx_wmo*));
	jks_array_resize(&gx_frame->render_lists.wmo, 0);
	jks_array_resize(&gx_frame->render_lists.text, 0);
	for (size_t type = 0; type < sizeof(gx_frame->render_lists.wmo_mliq) / sizeof(*gx_frame->render_lists.wmo_mliq); ++type)
	{
		for (size_t i = 0; i < gx_frame->render_lists.wmo_mliq[type].size; ++i)
			gx_wmo_mliq_clear_update(*JKS_ARRAY_GET(&gx_frame->render_lists.wmo_mliq[type], i, struct gx_wmo_mliq*));
		jks_array_resize(&gx_frame->render_lists.wmo_mliq[type], 0);
	}
	jks_array_resize(&gx_frame->backrefs.m2, 0);
	jks_array_resize(&gx_frame->backrefs.wmo, 0);
	jks_array_resize(&gx_frame->backrefs.wmo_mliq, 0);
}

void render_release_obj(struct gx_frame *gx_frame)
{
	for (size_t i = 0; i < gx_frame->backrefs.m2.size; ++i)
	{
		struct gx_m2_instance *instance = *JKS_ARRAY_GET(&gx_frame->backrefs.m2, i, struct gx_m2_instance*);
		instance->update_calculated = false;
		instance->in_render_list = false;
		instance->bones_calculated = false;
	}
	for (size_t i = 0; i < gx_frame->backrefs.wmo_mliq.size; ++i)
		(*JKS_ARRAY_GET(&gx_frame->backrefs.wmo_mliq, i, struct gx_wmo_mliq*))->in_render_list = false;
	for (size_t i = 0; i < gx_frame->backrefs.wmo.size; ++i)
		(*JKS_ARRAY_GET(&gx_frame->backrefs.wmo, i, struct gx_wmo_instance*))->in_render_list = false;
	for (size_t type = 0; type < sizeof(gx_frame->render_lists.wmo_mliq) / sizeof(*gx_frame->render_lists.wmo_mliq); ++type)
	{
		for (size_t i = 0; i < gx_frame->render_lists.wmo_mliq[type].size; ++i)
			(*JKS_ARRAY_GET(&gx_frame->render_lists.wmo_mliq[type], i, struct gx_wmo_mliq*))->in_render_lists[type] = false;
	}
	for (size_t i = 0; i < gx_frame->render_lists.m2.size; ++i)
		(*JKS_ARRAY_GET(&gx_frame->render_lists.m2, i, struct gx_m2*))->in_render_list = false;
	for (size_t i = 0; i < gx_frame->render_lists.wmo.size; ++i)
		(*JKS_ARRAY_GET(&gx_frame->render_lists.wmo, i, struct gx_wmo*))->in_render_list = false;
	for (size_t i = 0; i < gx_frame->render_lists.mcnk.size; ++i)
		(*JKS_ARRAY_GET(&gx_frame->render_lists.mcnk, i, struct gx_mcnk*))->in_render_list = false;
	for (size_t i = 0; i < gx_frame->render_lists.mcnk_objects.size; ++i)
		(*JKS_ARRAY_GET(&gx_frame->render_lists.mcnk_objects, i, struct gx_mcnk*))->in_objects_render_list = false;
	for (size_t type = 0; type < sizeof(gx_frame->render_lists.mclq) / sizeof(*gx_frame->render_lists.mclq); ++type)
	{
		for (size_t i = 0; i < gx_frame->render_lists.mclq[type].size; ++i)
			(*JKS_ARRAY_GET(&gx_frame->render_lists.mclq[type], i, struct gx_mclq*))->liquids[type].in_render_list = false;
	}
	for (size_t i = 0; i < gx_frame->render_lists.text.size; ++i)
		(*JKS_ARRAY_GET(&gx_frame->render_lists.text, i, struct gx_text*))->in_render_list = false;
}

void render_gc(struct gx_frame *gx_frame)
{
	pthread_mutex_lock(&gx_frame->gc_mutex);
	for (size_t i = 0; i < gx_frame->gc_lists.m2.size; ++i)
		gx_m2_instance_delete(*JKS_ARRAY_GET(&gx_frame->gc_lists.m2, i, struct gx_m2_instance*));
	jks_array_resize(&gx_frame->gc_lists.m2, 0);
	for (size_t i = 0; i < gx_frame->gc_lists.wmo.size; ++i)
		gx_wmo_instance_delete(*JKS_ARRAY_GET(&gx_frame->gc_lists.wmo, i, struct gx_wmo_instance*));
	jks_array_resize(&gx_frame->gc_lists.wmo, 0);
	for (size_t i = 0; i < gx_frame->gc_lists.text.size; ++i)
		gx_text_delete(*JKS_ARRAY_GET(&gx_frame->gc_lists.text, i, struct gx_text*));
	jks_array_resize(&gx_frame->gc_lists.text, 0);
	pthread_mutex_unlock(&gx_frame->gc_mutex);
}

bool render_gc_m2(struct gx_m2_instance *m2)
{
	pthread_mutex_lock(&g_wow->gc_frame->gc_mutex);
	if (!jks_array_push_back(&g_wow->gc_frame->gc_lists.m2, &m2))
	{
		pthread_mutex_unlock(&g_wow->gc_frame->gc_mutex);
		LOG_ERROR("failed to add m2 to gc list");
		return false;
	}
	pthread_mutex_unlock(&g_wow->gc_frame->gc_mutex);
	return true;
}

bool render_gc_wmo(struct gx_wmo_instance *wmo)
{
	pthread_mutex_lock(&g_wow->gc_frame->gc_mutex);
	if (!jks_array_push_back(&g_wow->gc_frame->gc_lists.wmo, &wmo))
	{
		pthread_mutex_unlock(&g_wow->gc_frame->gc_mutex);
		LOG_ERROR("failed to add wmo to gc list");
		return false;
	}
	pthread_mutex_unlock(&g_wow->gc_frame->gc_mutex);
	return true;
}

bool render_gc_text(struct gx_text *text)
{
	pthread_mutex_lock(&g_wow->gc_frame->gc_mutex);
	if (!jks_array_push_back(&g_wow->gc_frame->gc_lists.text, &text))
	{
		pthread_mutex_unlock(&g_wow->gc_frame->gc_mutex);
		LOG_ERROR("failed to add text to gc list");
		return false;
	}
	pthread_mutex_unlock(&g_wow->gc_frame->gc_mutex);
	return true;
}

bool render_add_mcnk_objects(struct gx_mcnk *mcnk)
{
	if (mcnk->in_objects_render_list)
		return false;
	if (!jks_array_push_back(&g_wow->cull_frame->render_lists.mcnk_objects, &mcnk))
	{
		LOG_ERROR("failed to add mcnk to objects render list");
		return false;
	}
	mcnk->in_objects_render_list = true;
	return true;
}

bool render_add_mcnk(struct gx_mcnk *mcnk)
{
	if (mcnk->in_render_list)
		return false;
	if (!jks_array_push_back(&g_wow->cull_frame->render_lists.mcnk, &mcnk))
	{
		LOG_ERROR("failed to add mcnk to render list");
		return false;
	}
	mcnk->in_render_list = true;
	return true;
}

bool render_add_mclq(uint8_t type, struct gx_mclq *mclq)
{
	if (mclq->liquids[type].in_render_list)
		return false;
	if (!jks_array_push_back(&g_wow->cull_frame->render_lists.mclq[type], &mclq))
	{
		LOG_ERROR("failed to add mclq to render list");
		return false;
	}
	mclq->liquids[type].in_render_list = true;
	return true;
}

bool render_add_wmo(struct gx_wmo_instance *instance, bool bypass_frustum)
{
	if (instance->in_render_list)
		return false;
	if (!jks_array_push_back(&g_wow->cull_frame->backrefs.wmo, &instance))
	{
		LOG_ERROR("failed to add wmo to backref list");
		return false;
	}
	instance->in_render_list = true;
	gx_wmo_instance_calculate_distance_to_camera(instance);
	gx_wmo_instance_update(instance, bypass_frustum);
	if (instance->render_frames[g_wow->cull_frame_id].culled)
		return false;
	if (!instance->parent->in_render_list)
	{
		if (!jks_array_push_back(&g_wow->cull_frame->render_lists.wmo, &instance->parent))
		{
			LOG_ERROR("failed to add wmo to render list");
			return false;
		}
		instance->parent->in_render_list = true;
	}
	return true;
}

bool render_add_wmo_mliq(struct gx_wmo_mliq *mliq, uint8_t liquid)
{
	if (!mliq->in_render_list)
	{
		if (!jks_array_push_back(&g_wow->cull_frame->backrefs.wmo_mliq, &mliq))
		{
			LOG_ERROR("failed to add wmo mliq to backref list");
			return false;
		}
		mliq->in_render_list = true;
	}
	if (!mliq->in_render_lists[liquid])
	{
		if (!jks_array_push_back(&g_wow->cull_frame->render_lists.wmo_mliq[liquid], &mliq))
		{
			LOG_ERROR("failed to add wmo mliq to render list");
			return false;
		}
		mliq->in_render_lists[liquid] = true;
	}
	return true;
}

bool render_add_m2_particles(struct gx_m2_instance *m2)
{
	if (!jks_array_push_back(&g_wow->cull_frame->render_lists.m2_particles, &m2))
	{
		LOG_ERROR("failed to add m2 to particles render list");
		return false;
	}
	return true;
}

bool render_add_m2_ribbons(struct gx_m2_instance *m2)
{
	if (!jks_array_push_back(&g_wow->cull_frame->render_lists.m2_ribbons, &m2))
	{
		LOG_ERROR("failed to add m2 to ribbons render list");
		return false;
	}
	return true;
}

bool render_add_m2_transparent(struct gx_m2_instance *m2)
{
	if (!jks_array_push_back(&g_wow->cull_frame->render_lists.m2_transparent, &m2))
	{
		LOG_ERROR("failed to add m2 to transparent render list");
		return false;
	}
	return true;
}

bool render_add_m2_opaque(struct gx_m2 *m2)
{
	if (!jks_array_push_back(&g_wow->cull_frame->render_lists.m2_opaque, &m2))
	{
		LOG_ERROR("failed to add m2 to opaque render list");
		return false;
	}
	return true;
}

bool render_add_m2_instance(struct gx_m2_instance *instance, bool bypass_frustum)
{
	if (instance->in_render_list)
		return false;
	if (!jks_array_push_back(&g_wow->cull_frame->backrefs.m2, &instance))
	{
		LOG_ERROR("failed to add m2 to backref list");
		return false;
	}
	instance->in_render_list = true;
	instance->render_frames[g_wow->cull_frame_id].culled = false;
	gx_m2_instance_update(instance, bypass_frustum, &g_wow->cull_frame->m2_params);
	if (instance->render_frames[g_wow->cull_frame_id].culled)
		return false;
	gx_m2_instance_calculate_distance_to_camera(instance);
	if (!instance->parent->in_render_list)
	{
		if (!jks_array_push_back(&g_wow->cull_frame->render_lists.m2, &instance->parent))
		{
			LOG_ERROR("failed to add m2 to render list");
			return false;
		}
		gx_m2_add_to_render(instance->parent);
	}
	return true;
}

bool render_add_text(struct gx_text *text)
{
	if (text->in_render_list)
		return false;
	if (!jks_array_push_back(&g_wow->cull_frame->render_lists.text, &text))
	{
		LOG_ERROR("failed to add text to render list");
		return false;
	}
	text->in_render_list = true;
	return true;
}
