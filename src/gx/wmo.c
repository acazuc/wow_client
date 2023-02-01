#include "gx/wmo_group.h"
#include "gx/wmo_mliq.h"
#include "gx/frame.h"
#include "gx/wmo.h"
#include "gx/m2.h"

#include "performance.h"
#include "shaders.h"
#include "camera.h"
#include "loader.h"
#include "memory.h"
#include "cache.h"
#include "log.h"
#include "wow.h"

#include <jks/quaternion.h>

#include <libwow/wmo_group.h>

#include <gfx/device.h>

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

MEMORY_DECL(GX);

#ifdef WITH_DEBUG_RENDERING
static void gx_wmo_group_instance_render_aabb(struct gx_wmo_group *group, struct gx_wmo_group_instance *instance, const struct mat4f *mvp);
#endif

struct gx_wmo *gx_wmo_new(const char *filename)
{
	struct gx_wmo *wmo = mem_zalloc(MEM_GX, sizeof(*wmo));
	if (!wmo)
		return NULL;
	wmo->filename = mem_strdup(MEM_GX, filename);
	if (!wmo->filename)
		goto err;
	for (size_t i = 0; i < sizeof(wmo->render_frames) / sizeof(*wmo->render_frames); ++i)
		jks_array_init(&wmo->render_frames[i].to_render, sizeof(struct gx_wmo_instance*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&wmo->instances, sizeof(struct gx_wmo_instance*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&wmo->groups, sizeof(struct gx_wmo_group*), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&wmo->modd, sizeof(struct wow_modd_data), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&wmo->mods, sizeof(struct wow_mods_data), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&wmo->molt, sizeof(struct wow_molt_data), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&wmo->mopt, sizeof(struct wow_mopt_data), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&wmo->mopr, sizeof(struct wow_mopr_data), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&wmo->momt, sizeof(struct wow_momt_data), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&wmo->mopv, sizeof(struct wow_vec3f), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&wmo->modn, sizeof(char), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&wmo->motx, sizeof(char), NULL, &jks_array_memory_fn_GX);
#ifdef WITH_DEBUG_RENDERING
	gx_wmo_portals_init(&wmo->gx_portals);
	gx_wmo_lights_init(&wmo->gx_lights);
#endif
	return wmo;

err:
	mem_free(MEM_GX, wmo);
	return NULL;
}

void gx_wmo_delete(struct gx_wmo *wmo)
{
	if (!wmo)
		return;
	for (size_t i = 0; i < sizeof(wmo->render_frames) / sizeof(*wmo->render_frames); ++i)
		jks_array_destroy(&wmo->render_frames[i].to_render);
	jks_array_destroy(&wmo->instances);
	jks_array_destroy(&wmo->groups);
	jks_array_destroy(&wmo->modd);
	jks_array_destroy(&wmo->mods);
	jks_array_destroy(&wmo->molt);
	jks_array_destroy(&wmo->mopt);
	jks_array_destroy(&wmo->mopr);
	jks_array_destroy(&wmo->momt);
	jks_array_destroy(&wmo->mopv);
	jks_array_destroy(&wmo->modn);
	jks_array_destroy(&wmo->motx);
#ifdef WITH_DEBUG_RENDERING
	gx_wmo_portals_destroy(&wmo->gx_portals);
	gx_wmo_lights_destroy(&wmo->gx_lights);
#endif
	assert(!wmo->instances.size);
	mem_free(MEM_GX, wmo->filename);
	mem_free(MEM_GX, wmo);
}

void gx_wmo_ask_load(struct gx_wmo *wmo)
{
	if (wmo->load_asked)
		return;
	wmo->load_asked = true;
	cache_ref_by_ref_unmutexed_wmo(g_wow->cache, wmo);
	loader_push(g_wow->loader, ASYNC_TASK_WMO_LOAD, wmo);
}

void gx_wmo_ask_unload(struct gx_wmo *wmo)
{
	loader_gc_wmo(g_wow->loader, wmo);
	for (size_t i = 0; i < wmo->groups.size; ++i)
	{
		struct gx_wmo_group *group = *JKS_ARRAY_GET(&wmo->groups, i, struct gx_wmo_group*);
		loader_gc_wmo_group(g_wow->loader, group);
	}
}

bool gx_wmo_load(struct gx_wmo *wmo, struct wow_wmo_file *file)
{
	if (wmo->loaded)
		return true;
	wmo->flags = file->mohd.flags;
	VEC4_SET(wmo->ambient, file->mohd.ambient.z / 255., file->mohd.ambient.y / 255., file->mohd.ambient.x / 255, file->mohd.ambient.w / 255.);
	if (!jks_array_reserve(&wmo->groups, file->mohd.groups_nb))
		return false;
	if (file->modd.data_nb)
	{
		if (!jks_array_resize(&wmo->modd, file->modd.data_nb))
			return false;
		memcpy(wmo->modd.data, file->modd.data, sizeof(*file->modd.data) * file->modd.data_nb);
		for (size_t i = 0; i < file->modd.data_nb; ++i)
		{
			struct wow_modd_data *modd = JKS_ARRAY_GET(&wmo->modd, i, struct wow_modd_data);
			modd->position = (struct wow_vec3f){modd->position.x, modd->position.z, -modd->position.y};
			modd->rotation = (struct wow_quatf){modd->rotation.x, modd->rotation.z, -modd->rotation.y, modd->rotation.w};
		}
	}
	if (file->mods.data_nb)
	{
		if (!jks_array_resize(&wmo->mods, file->mods.data_nb))
			return false;
		memcpy(jks_array_get(&wmo->mods, 0), file->mods.data, sizeof(*file->mods.data) * file->mods.data_nb);
	}
	if (file->modn.data_len)
	{
		if (!jks_array_resize(&wmo->modn, file->modn.data_len))
			return false;
		memcpy(jks_array_get(&wmo->modn, 0), file->modn.data, sizeof(*file->modn.data) * file->modn.data_len);
	}
	if (file->molt.data_nb)
	{
		if (!jks_array_resize(&wmo->molt, file->molt.data_nb))
			return false;
		memcpy(jks_array_get(&wmo->molt, 0), file->molt.data, sizeof(*file->molt.data) * file->molt.data_nb);
		for (size_t i = 0; i < wmo->molt.size; ++i)
		{
			struct wow_molt_data *molt = jks_array_get(&wmo->molt, i);
			molt->position = (struct wow_vec3f){molt->position.x, molt->position.z, -molt->position.y};
			molt->color = (struct wow_vec4b){molt->color.z, molt->color.y, molt->color.x, molt->color.w};
		}
	}
	if (file->mopt.data_nb)
	{
		if (!jks_array_resize(&wmo->mopt, file->mopt.data_nb))
			return false;
		memcpy(jks_array_get(&wmo->mopt, 0), file->mopt.data, sizeof(*file->mopt.data) * file->mopt.data_nb);
		for (size_t i = 0; i < wmo->mopt.size; ++i)
		{
			struct wow_mopt_data *mopt = jks_array_get(&wmo->mopt, i);
			mopt->normal = (struct wow_vec3f){mopt->normal.x, mopt->normal.z, -mopt->normal.y};
		}
	}
	if (file->momt.data_nb)
	{
		if (!jks_array_resize(&wmo->momt, file->momt.data_nb))
			return false;
		memcpy(jks_array_get(&wmo->momt, 0), file->momt.data, sizeof(*file->momt.data) * file->momt.data_nb);
	}
	if (file->motx.data_len)
	{
		if (!jks_array_resize(&wmo->motx, file->motx.data_len))
			return false;
		memcpy(jks_array_get(&wmo->motx, 0), file->motx.data, sizeof(*file->motx.data) * file->motx.data_len);
	}
	if (file->mopv.vertices_nb)
	{
		if (!jks_array_resize(&wmo->mopv, file->mopv.vertices_nb))
			return false;
		memcpy(wmo->mopv.data, file->mopv.vertices, sizeof(*file->mopv.vertices) * file->mopv.vertices_nb);
		for (size_t i = 0; i < wmo->mopv.size; ++i)
		{
			struct wow_vec3f *tmp = jks_array_get(&wmo->mopv, i);
			*tmp = (struct wow_vec3f){tmp->x, tmp->z, -tmp->y};
		}
	}
	if (file->mopr.data_nb)
	{
		if (!jks_array_resize(&wmo->mopr, file->mopr.data_nb))
			return false;
		memcpy(jks_array_get(&wmo->mopr, 0), file->mopr.data, sizeof(*file->mopr.data) * file->mopr.data_nb);
	}
	struct vec3f p0 = {file->mohd.aabb0.x, file->mohd.aabb0.z, -file->mohd.aabb0.y};
	struct vec3f p1 = {file->mohd.aabb1.x, file->mohd.aabb1.z, -file->mohd.aabb1.y};
	VEC3_MIN(wmo->aabb.p0, p0, p1);
	VEC3_MAX(wmo->aabb.p1, p0, p1);
	for (size_t i = 0; i < file->mohd.groups_nb; ++i)
	{
		struct gx_wmo_group *group = gx_wmo_group_new(wmo, i, file->mogi.data[i].flags);
		if (!group)
		{
			LOG_ERROR("failed to create wmo group");
			return false;
		}
		if (!jks_array_push_back(&wmo->groups, &group))
		{
			LOG_ERROR("failed to push wmo group");
			return false;
		}
		loader_push(g_wow->loader, ASYNC_TASK_WMO_GROUP_LOAD, group);
	}
	for (size_t i = 0; i < wmo->instances.size; ++i)
		gx_wmo_instance_on_load(*(struct gx_wmo_instance**)jks_array_get(&wmo->instances, i));
#ifdef WITH_DEBUG_RENDERING
	if (!gx_wmo_portals_load(&wmo->gx_portals, (struct wow_mopt_data*)wmo->mopt.data, wmo->mopt.size, (struct wow_vec3f*)wmo->mopv.data, wmo->mopv.size))
		return false;
	if (!gx_wmo_lights_load(&wmo->gx_lights, (struct wow_molt_data*)wmo->molt.data, wmo->molt.size))
		return false;
#endif
	wmo->loaded = true;
	if (!loader_init_wmo(g_wow->loader, wmo))
		return false;
	return true;
}

int gx_wmo_initialize(struct gx_wmo *wmo)
{
	if (wmo->initialized)
		return 1;
#ifdef WITH_DEBUG_RENDERING
	gx_wmo_portals_initialize(&wmo->gx_portals);
	gx_wmo_lights_initialize(&wmo->gx_lights);
#endif
	wmo->initialized = true;
	return 1;
}

void gx_wmo_clear_update(struct gx_wmo *wmo)
{
	jks_array_resize(&wmo->render_frames[g_wow->cull_frame_id].to_render, 0);
}

void gx_wmo_cull_portal(struct gx_wmo *wmo)
{
	if (!wmo->initialized)
		return;
	for (size_t i = 0; i < wmo->render_frames[g_wow->draw_frame_id].to_render.size; ++i)
	{
		struct gx_wmo_instance *instance = *JKS_ARRAY_GET(&wmo->render_frames[g_wow->draw_frame_id].to_render, i, struct gx_wmo_instance*);
		gx_wmo_instance_cull_portal(instance);
	}
}

void gx_wmo_render(struct gx_wmo *wmo)
{
	if (!wmo->initialized)
		return;
	for (size_t i = 0; i < wmo->render_frames[g_wow->draw_frame_id].to_render.size; ++i)
	{
		struct gx_wmo_instance *instance = *JKS_ARRAY_GET(&wmo->render_frames[g_wow->draw_frame_id].to_render, i, struct gx_wmo_instance*);
		PERFORMANCE_BEGIN(WMO_RENDER_DATA);
		struct shader_wmo_model_block model_block;
		model_block.v = *(struct mat4f*)&g_wow->draw_frame->view_v;
		model_block.mv = instance->render_frames[g_wow->draw_frame_id].mv;
		model_block.mvp = instance->render_frames[g_wow->draw_frame_id].mvp;
		if (!instance->uniform_buffers[g_wow->draw_frame_id].handle.u64)
			gfx_create_buffer(g_wow->device, &instance->uniform_buffers[g_wow->draw_frame_id], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_wmo_model_block), GFX_BUFFER_STREAM);
		gfx_set_buffer_data(&instance->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
		PERFORMANCE_END(WMO_RENDER_DATA);
	}
	for (size_t i = 0; i < wmo->groups.size; ++i)
		gx_wmo_group_render(*(struct gx_wmo_group**)jks_array_get(&wmo->groups, i), &wmo->render_frames[g_wow->draw_frame_id].to_render);
}

#ifdef WITH_DEBUG_RENDERING
void gx_wmo_render_aabb(struct gx_wmo *wmo)
{
	if (!wmo->initialized)
		return;
	for (size_t i = 0; i < wmo->render_frames[g_wow->draw_frame_id].to_render.size; ++i)
	{
		struct gx_wmo_instance *instance = *JKS_ARRAY_GET(&wmo->render_frames[g_wow->draw_frame_id].to_render, i, struct gx_wmo_instance*);
		gx_aabb_update(&instance->gx_aabb, (struct mat4f*)&g_wow->draw_frame->view_vp);
		gx_aabb_render(&instance->gx_aabb);
		for (size_t j = 0; j < wmo->groups.size; ++j)
		{
			struct gx_wmo_group_instance *group_instance = jks_array_get(&instance->groups, j);
			if (group_instance->render_frames[g_wow->draw_frame_id].culled)
				continue;
			struct gx_wmo_group *group = *(struct gx_wmo_group**)jks_array_get(&wmo->groups, j);
			if (!group || !group->loaded)
				continue;
			gx_wmo_group_instance_render_aabb(group, group_instance, &g_wow->draw_frame->view_vp);
		}
	}
}

void gx_wmo_render_portals(struct gx_wmo *wmo)
{
	if (!wmo->initialized || !wmo->mopt.size)
		return;
	for (size_t i = 0; i < wmo->render_frames[g_wow->draw_frame_id].to_render.size; ++i)
	{
		struct gx_wmo_instance *instance = *JKS_ARRAY_GET(&wmo->render_frames[g_wow->draw_frame_id].to_render, i, struct gx_wmo_instance*);
		gx_wmo_portals_render(&wmo->gx_portals, instance);
	}
}

void gx_wmo_render_lights(struct gx_wmo *wmo)
{
	if (!wmo->initialized || !wmo->molt.size)
		return;
	for (size_t i = 0; i < wmo->render_frames[g_wow->draw_frame_id].to_render.size; ++i)
	{
		struct gx_wmo_instance *instance = *JKS_ARRAY_GET(&wmo->render_frames[g_wow->draw_frame_id].to_render, i, struct gx_wmo_instance*);
		gx_wmo_lights_update(&wmo->gx_lights, &instance->render_frames[g_wow->draw_frame_id].mvp);
		gx_wmo_lights_render(&wmo->gx_lights);
	}
}

void gx_wmo_render_collisions(struct gx_wmo *wmo, bool triangles)
{
	if (!wmo->initialized)
		return;
	for (size_t i = 0; i < wmo->groups.size; ++i)
	{
		struct gx_wmo_group *group = *(struct gx_wmo_group**)jks_array_get(&wmo->groups, i);
		if (!group || !group->loaded)
			continue;
		if (!(group->flags & WOW_MOGP_FLAGS_BSP))
			continue;
		gx_wmo_collisions_render(&group->gx_collisions, wmo->render_frames[g_wow->draw_frame_id].to_render.data, wmo->render_frames[g_wow->draw_frame_id].to_render.size, i, triangles);
	}
}
#endif

static void check_frustum(struct gx_wmo *wmo, struct gx_wmo_instance *instance)
{
	if (!wmo->initialized)
		return;
	struct vec4f tmp;
	VEC3_CPY(tmp, g_wow->cull_frame->cull_pos);
	tmp.w = 1;
	struct vec4f rpos;
	MAT4_VEC4_MUL(rpos, instance->m_inv, tmp);
	struct vec3f p3 = {rpos.x, rpos.y, rpos.z};
	for (size_t i = 0; i < wmo->groups.size; ++i)
	{
		struct gx_wmo_group_instance *group_instance = jks_array_get(&instance->groups, i);
		struct gx_wmo_group *group = *(struct gx_wmo_group**)jks_array_get(&wmo->groups, i);
		if (!group->loaded)
		{
			group_instance->render_frames[g_wow->cull_frame_id].culled = true;
			continue;
		}
		if (group->flags & WOW_MOGP_FLAGS_INDOOR)
		{
			if (!aabb_contains(&group->aabb, p3))
			{
				group_instance->render_frames[g_wow->cull_frame_id].culled = true;
				continue;
			}
		}
		group_instance->render_frames[g_wow->cull_frame_id].culled = !frustum_check_fast(&instance->frustum, &group->aabb);
		if (group_instance->render_frames[g_wow->cull_frame_id].culled)
			continue;
		group_instance->render_frames[g_wow->cull_frame_id].cull_source = true;
		for (size_t j = 0; j < group_instance->batches.size; ++j)
		{
			struct gx_wmo_batch_instance *batch = jks_array_get(&group_instance->batches, j);
			batch->render_frames[g_wow->cull_frame_id].culled = true;
		}
	}
}

void gx_wmo_batch_instance_init(struct gx_wmo_batch_instance *instance)
{
#ifdef WITH_DEBUG_RENDERING
	gx_aabb_init(&instance->gx_aabb, (struct vec4f){1, 0, 0, 1}, 1);
#endif
	for (size_t i = 0; i < sizeof(instance->render_frames) / sizeof(*instance->render_frames); ++i)
		instance->render_frames[i].culled = true;
}

void gx_wmo_batch_instance_destroy(struct gx_wmo_batch_instance *instance)
{
	(void)instance;
#ifdef WITH_DEBUG_RENDERING
	gx_aabb_destroy(&instance->gx_aabb);
#endif
}

#ifdef WITH_DEBUG_RENDERING
void gx_wmo_batch_instance_render_aabb(struct gx_wmo_batch_instance *instance, const struct mat4f *mvp)
{
	gx_aabb_update(&instance->gx_aabb, mvp);
	gx_aabb_render(&instance->gx_aabb);
}
#endif

void gx_wmo_group_instance_init(struct gx_wmo_group_instance *instance)
{
	jks_array_init(&instance->batches, sizeof(struct gx_wmo_batch_instance), (jks_array_destructor_t)gx_wmo_batch_instance_destroy, &jks_array_memory_fn_GX);
	for (size_t i = 0; i < sizeof(instance->render_frames) / sizeof(*instance->render_frames); ++i)
	{
		instance->render_frames[i].culled = true;
		instance->render_frames[i].cull_source = false;
	}
#ifdef WITH_DEBUG_RENDERING
	gx_aabb_init(&instance->gx_aabb, (struct vec4f){0, 0, 0, 0}, 1);
#endif
}

void gx_wmo_group_instance_destroy(struct gx_wmo_group_instance *instance)
{
	jks_array_destroy(&instance->batches);
#ifdef WITH_DEBUG_RENDERING
	gx_aabb_destroy(&instance->gx_aabb);
#endif
}

bool gx_wmo_group_instance_on_load(struct gx_wmo_instance *instance, struct gx_wmo_group *group, struct gx_wmo_group_instance *group_instance)
{
	gx_wmo_group_set_m2_ambient(group, instance);
	gx_wmo_group_instance_init(group_instance);
	if (!jks_array_resize(&group_instance->batches, group->batches.size))
	{
		LOG_ERROR("failed to resize wmo group instance batches array");
		return true;
	}
	for (size_t k = 0; k < group_instance->batches.size; ++k)
		gx_wmo_batch_instance_init(jks_array_get(&group_instance->batches, k));
	group_instance->aabb = group->aabb;
	aabb_transform(&group_instance->aabb, &instance->m);
#ifdef WITH_DEBUG_RENDERING
	group_instance->gx_aabb.aabb = group_instance->aabb;
	group_instance->gx_aabb.flags |= GX_AABB_DIRTY;
#endif
	for (size_t j = 0; j < group_instance->batches.size; ++j)
	{
		struct gx_wmo_batch_instance *batch_instance = jks_array_get(&group_instance->batches, j);
		struct gx_wmo_batch *batch = jks_array_get(&group->batches, j);
		batch_instance->aabb = batch->aabb;
		aabb_transform(&batch_instance->aabb, &instance->m);
#ifdef WITH_DEBUG_RENDERING
		batch_instance->gx_aabb.aabb = batch_instance->aabb;
		batch_instance->gx_aabb.flags |= GX_AABB_DIRTY;
#endif
	}
	return true;
}

#ifdef WITH_DEBUG_RENDERING
static void gx_wmo_group_instance_render_aabb(struct gx_wmo_group *group, struct gx_wmo_group_instance *instance, const struct mat4f *mvp)
{
	for (size_t i = 0; i < instance->batches.size; ++i)
	{
		struct gx_wmo_batch_instance *batch = jks_array_get(&instance->batches, i);
		gx_wmo_batch_instance_render_aabb(batch, mvp);
	}
	if (group->flags & WOW_MOGP_FLAGS_INDOOR)
		VEC4_SET(instance->gx_aabb.color, 0, 1, 1, 1);
	else
		VEC4_SET(instance->gx_aabb.color, 0, 1, 0, 1);
	gx_aabb_update(&instance->gx_aabb, mvp);
	gx_aabb_render(&instance->gx_aabb);
}
#endif

static void delete_m2_instance(struct gx_m2_instance **instance)
{
	gx_m2_instance_gc(*instance);
}

struct gx_wmo_instance *gx_wmo_instance_new(const char *filename)
{
	struct gx_wmo_instance *wmo = mem_zalloc(MEM_GX, sizeof(*wmo));
	if (!wmo)
		return NULL;
	wmo->traversed_portals = NULL;
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		wmo->uniform_buffers[i] = GFX_BUFFER_INIT();
	for (size_t i = 0; i < sizeof(wmo->render_frames) / sizeof(*wmo->render_frames); ++i)
		wmo->render_frames[i].culled = true;
	jks_array_init(&wmo->groups, sizeof(struct gx_wmo_group_instance), (jks_array_destructor_t)gx_wmo_group_instance_destroy, &jks_array_memory_fn_GX);
	jks_array_init(&wmo->m2, sizeof(struct gx_m2_instance*), (jks_array_destructor_t)delete_m2_instance, &jks_array_memory_fn_GX);
	frustum_init(&wmo->frustum);
#ifdef WITH_DEBUG_RENDERING
	gx_aabb_init(&wmo->gx_aabb, (struct vec4f){0, 0, 1, 1}, 2);
#endif
	cache_lock_wmo(g_wow->cache);
	if (!cache_ref_by_key_unmutexed_wmo(g_wow->cache, filename, &wmo->parent))
	{
		cache_unlock_wmo(g_wow->cache);
		goto err;
	}
	if (!jks_array_push_back(&wmo->parent->instances, &wmo))
	{
		cache_unlock_wmo(g_wow->cache);
		goto err;
	}
	cache_unlock_wmo(g_wow->cache);
	return wmo;

err:
	return NULL;
}

void gx_wmo_instance_delete(struct gx_wmo_instance *instance)
{
	if (!instance)
		return;
	frustum_destroy(&instance->frustum);
	{
		cache_lock_wmo(g_wow->cache);
		size_t i;
		for (i = 0; i < instance->parent->instances.size; ++i)
		{
			struct gx_wmo_instance *tmp = *(struct gx_wmo_instance**)jks_array_get(&instance->parent->instances, i);
			if (tmp != instance)
				continue;
			goto found;
		}
		cache_unlock_wmo(g_wow->cache);
		goto unref;
found:
		cache_unlock_wmo(g_wow->cache);
		jks_array_erase(&instance->parent->instances, i);
	}
unref:
	cache_unref_by_ref_wmo(g_wow->cache, instance->parent);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_delete_buffer(g_wow->device, &instance->uniform_buffers[i]);
	jks_array_destroy(&instance->groups);
	jks_array_destroy(&instance->m2);
#ifdef WITH_DEBUG_RENDERING
	gx_aabb_destroy(&instance->gx_aabb);
#endif
	mem_free(MEM_GX, instance->traversed_portals);
	mem_free(MEM_GX, instance);
}

void gx_wmo_instance_gc(struct gx_wmo_instance *instance)
{
	if (!instance)
		return;
	render_gc_wmo(instance);
}

void gx_wmo_instance_on_load(struct gx_wmo_instance *instance)
{
	gx_wmo_instance_load_doodad_set(instance);
	instance->traversed_portals = mem_zalloc(MEM_GX, (instance->parent->mopr.size + 7) / 8);
	if (!instance->traversed_portals)
		LOG_ERROR("failed to allocate traversed portals array");
	if (!jks_array_resize(&instance->groups, instance->parent->groups.size))
		LOG_ERROR("failed to allocate groups array");
	for (size_t i = 0; i < instance->parent->groups.size; ++i)
	{
		struct gx_wmo_group_instance *group_instance = jks_array_get(&instance->groups, i);
		struct gx_wmo_group *group = *(struct gx_wmo_group**)jks_array_get(&instance->parent->groups, i);
		gx_wmo_group_instance_init(group_instance);
		if (group->loaded)
			gx_wmo_group_instance_on_load(instance, group, group_instance);
	}
	gx_wmo_instance_update_aabb(instance);
}

void gx_wmo_instance_load_doodad_set(struct gx_wmo_instance *instance)
{
	if (instance->doodad_set >= instance->parent->mods.size)
	{
		LOG_WARN("invalid doodad set: %u / %u", instance->doodad_set, instance->parent->mods.size);
		return;
	}
	struct wow_mods_data *mods = jks_array_get(&instance->parent->mods, instance->doodad_set);
	instance->doodad_start = mods->start_index;
	instance->doodad_end = mods->start_index + mods->count;
	for (size_t i = mods->start_index; i < mods->start_index + mods->count; ++i)
	{
		struct wow_modd_data *modd = jks_array_get(&instance->parent->modd, i);
		char filename[512];
		snprintf(filename, sizeof(filename), "%s", (char*)jks_array_get(&instance->parent->modn, modd->name_index));
		if (!filename[0])
			continue;
		normalize_m2_filename(filename, sizeof(filename));
		struct gx_m2_instance *m2 = gx_m2_instance_new_filename(filename);
		struct mat4f mat1;
		struct mat4f mat2;
		MAT4_TRANSLATE(mat1, instance->m, modd->position);
		struct mat4f quat;
		QUATERNION_TO_MAT4(float, quat, modd->rotation);
		MAT4_MUL(mat2, mat1, quat);
		MAT4_SCALEV(mat1, mat2, modd->scale);
		{
			cache_lock_m2(g_wow->cache);
			m2->scale = modd->scale;
			gx_m2_instance_set_mat(m2, &mat1);
			struct vec4f tmp1 = {modd->position.x, modd->position.y, modd->position.z, 1};
			struct vec4f tmp3;
			MAT4_VEC4_MUL(tmp3, instance->m, tmp1);
			VEC3_CPY(m2->pos, tmp3);
			if (m2->parent->loaded)
				gx_m2_instance_on_parent_loaded(m2);
			else
				gx_m2_ask_load(m2->parent);
			cache_unlock_m2(g_wow->cache);
		}
		if (!jks_array_push_back(&instance->m2, &m2))
			LOG_ERROR("failed to add m2 to wmo m2 list");
	}
}

void gx_wmo_instance_update(struct gx_wmo_instance *wmo, bool bypass_frustum)
{
	if (!wmo->parent->loaded)
	{
		wmo->render_frames[g_wow->cull_frame_id].culled = true;
		return;
	}
	if (!bypass_frustum)
	{
		if (!frustum_check_fast(&g_wow->cull_frame->frustum, &wmo->aabb))
		{
			wmo->render_frames[g_wow->cull_frame_id].culled = true;
			return;
		}
	}
	wmo->render_frames[g_wow->cull_frame_id].culled = false;
	MAT4_MUL(wmo->render_frames[g_wow->cull_frame_id].mv, g_wow->cull_frame->view_v, wmo->m);
	MAT4_MUL(wmo->render_frames[g_wow->cull_frame_id].mvp, g_wow->cull_frame->view_p, wmo->render_frames[g_wow->cull_frame_id].mv);
	if (g_wow->view_camera != g_wow->frustum_camera)
	{
		struct mat4f tmp1;
		struct mat4f tmp2;
		MAT4_MUL(tmp1, g_wow->cull_frame->cull_v, wmo->m);
		MAT4_MUL(tmp2, g_wow->cull_frame->cull_p, tmp1);
		if (!frustum_update(&wmo->frustum, &tmp2))
			LOG_ERROR("failed to update frustum");
	}
	else
	{
		if (!frustum_update(&wmo->frustum, &wmo->render_frames[g_wow->cull_frame_id].mvp))
			LOG_ERROR("failed to update frustum");
	}
	check_frustum(wmo->parent, wmo);
}

void gx_wmo_instance_cull_portal(struct gx_wmo_instance *instance)
{
	if (instance->render_frames[g_wow->draw_frame_id].culled)
		return;
	struct gx_wmo *wmo = instance->parent;
	memset(instance->traversed_portals, 0, (wmo->mopr.size + 7) / 8);
	struct vec4f tmp;
	VEC3_CPY(tmp, g_wow->cull_frame->cull_pos);
	tmp.w = 1;
	struct vec4f rpos;
	MAT4_VEC4_MUL(rpos, instance->m_inv, tmp);
	for (size_t i = 0; i < wmo->groups.size; ++i)
	{
		struct gx_wmo_group *group = *JKS_ARRAY_GET(&wmo->groups, i, struct gx_wmo_group*);
		struct gx_wmo_group_instance *group_instance = jks_array_get(&instance->groups, i);
		if ((group->flags & WOW_MOGP_FLAGS_INDOOR) && (group_instance->render_frames[g_wow->cull_frame_id].culled || !group_instance->render_frames[g_wow->cull_frame_id].cull_source))
			continue;
		group_instance->render_frames[g_wow->cull_frame_id].cull_source = !(group->flags & WOW_MOGP_FLAGS_INDOOR);
		gx_wmo_group_cull_portal(group, instance, rpos);
	}
}

void gx_wmo_instance_calculate_distance_to_camera(struct gx_wmo_instance *instance)
{
	struct vec3f tmp;
	VEC3_SUB(tmp, instance->pos, g_wow->cull_frame->cull_pos);
	instance->render_frames[g_wow->cull_frame_id].distance_to_camera = VEC3_NORM(tmp);
}

void gx_wmo_instance_add_to_render(struct gx_wmo_instance *instance, bool bypass_frustum)
{
	if (!render_add_wmo(instance, bypass_frustum))
		return;
	if (!jks_array_push_back(&instance->parent->render_frames[g_wow->cull_frame_id].to_render, &instance))
		LOG_ERROR("failed to push wmo 3d instance to render list");
	for (size_t i = 0; i < instance->parent->groups.size; ++i)
	{
		struct gx_wmo_group *group = *JKS_ARRAY_GET(&instance->parent->groups, i, struct gx_wmo_group*);
		if (group->gx_mliq)
			gx_wmo_mliq_add_to_render(group->gx_mliq, instance);
	}
}

void gx_wmo_instance_set_mat(struct gx_wmo_instance *instance, const struct mat4f *m)
{
	instance->m = *m;
	MAT4_INVERSE(float, instance->m_inv, instance->m);
	if (instance->parent->loaded)
		gx_wmo_instance_update_aabb(instance);
}

void gx_wmo_instance_update_aabb(struct gx_wmo_instance *instance)
{
	instance->aabb = instance->parent->aabb;
	aabb_transform(&instance->aabb, &instance->m);
#ifdef WITH_DEBUG_RENDERING
	instance->gx_aabb.aabb = instance->aabb;
	instance->gx_aabb.flags |= GX_AABB_DIRTY;
#endif
	for (size_t i = 0; i < instance->groups.size; ++i)
	{
		struct gx_wmo_group *group = *(struct gx_wmo_group**)jks_array_get(&instance->parent->groups, i);
		if (!group->loaded)
			continue;
		struct gx_wmo_group_instance *group_instance = jks_array_get(&instance->groups, i);
		group_instance->aabb = group->aabb;
		aabb_transform(&group_instance->aabb, &instance->m);
#ifdef WITH_DEBUG_RENDERING
		group_instance->gx_aabb.aabb = group_instance->aabb;
		group_instance->gx_aabb.flags |= GX_AABB_DIRTY;
#endif
		for (size_t j = 0; j < group_instance->batches.size; ++j)
		{
			struct gx_wmo_batch_instance *batch_instance = jks_array_get(&group_instance->batches, j);
			struct gx_wmo_batch *batch = jks_array_get(&group->batches, j);
			batch_instance->aabb = batch->aabb;
			aabb_transform(&batch_instance->aabb, &instance->m);
#ifdef WITH_DEBUG_RENDERING
			batch_instance->gx_aabb.aabb = batch_instance->aabb;
			batch_instance->gx_aabb.flags |= GX_AABB_DIRTY;
#endif
		}
	}
}
