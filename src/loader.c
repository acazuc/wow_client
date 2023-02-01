#include "loader.h"

#include "gx/wmo_group.h"
#include "gx/skybox.h"
#include "gx/frame.h"
#include "gx/wmo.h"
#include "gx/m2.h"

#include "map/tile.h"
#include "map/map.h"

#include "memory.h"
#include "cache.h"
#include "log.h"
#include "wow.h"
#include "blp.h"

#include <libwow/wmo_group.h>
#include <libwow/adt.h>
#include <libwow/mpq.h>
#include <libwow/blp.h>
#include <libwow/wmo.h>
#include <libwow/m2.h>

#include <gfx/window.h>

#include <jks/array.h>
#include <jks/list.h>

#include <stdatomic.h>
#include <pthread.h>
#include <unistd.h>

#define ASYNC_THREADS 6
#define CULL_THREADS 1

MEMORY_DECL(GENERIC);

struct async_task
{
	enum async_task_type type;
	union
	{
		struct map_tile *map_tile;
		struct blp_texture *blp;
		struct gx_m2 *m2;
		struct gx_wmo *wmo;
		struct gx_wmo_group *wmo_group;
		void *data;
	};
};

enum loader_object_type
{
	LOADER_MAP_TILE,
	LOADER_BLP,
	LOADER_M2,
	LOADER_WMO,
	LOADER_WMO_GROUP,
	LOADER_LAST,
};

struct loader_object
{
	enum loader_object_type type;
	union
	{
		struct map_tile *map_tile;
		struct blp_texture *blp;
		struct gx_m2 *m2;
		struct gx_wmo *wmo;
		struct gx_wmo_group *wmo_group;
		void *data;
	};
};

struct async_worker
{
	struct wow_mpq_compound *mpq_compound;
	pthread_t thread;
};

struct loader
{
	pthread_mutex_t cull_mutex;
	pthread_mutex_t gc_mutex;
	pthread_mutex_t mutex;
	struct jks_array workers; /* struct async_worker */
	struct jks_list tasks[ASYNC_TASK_LAST]; /* struct async_task */
	struct jks_array cull_threads; /* pthread_t */
	pthread_cond_t cull_run_condition;
	pthread_cond_t cull_end_condition;
	pthread_mutex_t async_mutex;
	struct jks_list objects_to_init[LOADER_LAST]; /* struct loader_object */
	struct jks_list objects_to_init_buffer; /* struct loader_object */
	struct jks_list objects_gc[RENDER_FRAMES_COUNT]; /* struct loader_object */
	atomic_int cull_ended;
	bool cull_ready;
	bool running;
};

void execute_task(struct async_worker *worker, struct async_task *task)
{
	switch (task->type)
	{
		case ASYNC_TASK_WMO_GROUP_LOAD:
		{
			struct wow_mpq_file *file = wow_mpq_get_file(worker->mpq_compound, task->wmo_group->filename);
			if (!file)
			{
				LOG_WARN("WMO group file not found: %s", task->wmo_group->filename);
				task->wmo_group->loading = false;
				break;
			}
			struct wow_wmo_group_file *wmo_group = wow_wmo_group_file_new(file);
			wow_mpq_file_delete(file);
			if (!wmo_group)
			{
				LOG_ERROR("failed to create wmo group from file %s", task->wmo_group->filename);
				break;
			}
			cache_lock_wmo(g_wow->cache);
			if (!gx_wmo_group_load(task->wmo_group, wmo_group))
				LOG_ERROR("failed to load wmo group");
			task->wmo_group->loading = false;
			cache_unlock_wmo(g_wow->cache);
			wow_wmo_group_file_delete(wmo_group);
			break;
		}
		case ASYNC_TASK_WMO_GROUP_UNLOAD:
		{
			gx_wmo_group_delete(task->wmo_group);
			break;
		}
		case ASYNC_TASK_WMO_LOAD:
		{
			struct wow_mpq_file *file = wow_mpq_get_file(worker->mpq_compound, task->wmo->filename);
			if (!file)
			{
				LOG_WARN("WMO file not found: %s", task->wmo->filename);
				task->wmo->loading = false;
				cache_unref_by_ref_wmo(g_wow->cache, task->wmo);
				break;
			}
			struct wow_wmo_file *wmo = wow_wmo_file_new(file);
			wow_mpq_file_delete(file);
			if (!wmo)
			{
				LOG_ERROR("failed to create wmo from file %s", task->wmo->filename);
				cache_unref_by_ref_wmo(g_wow->cache, task->wmo);
				break;
			}
			cache_lock_wmo(g_wow->cache);
			if (!gx_wmo_load(task->wmo, wmo))
			{
				LOG_ERROR("failed to load wmo");
				cache_unref_by_ref_unmutexed_wmo(g_wow->cache, task->wmo);
			}
			task->wmo->loading = false;
			cache_unlock_wmo(g_wow->cache);
			wow_wmo_file_delete(wmo);
			break;
		}
		case ASYNC_TASK_WMO_UNLOAD:
		{
			gx_wmo_delete(task->wmo);
			break;
		}
		case ASYNC_TASK_M2_LOAD:
		{
			struct wow_mpq_file *file = wow_mpq_get_file(worker->mpq_compound, task->m2->filename);
			if (!file)
			{
				LOG_WARN("M2 file not found: %s", task->m2->filename);
				task->m2->invalid = true;
				task->m2->loading = false;
				cache_unref_by_ref_m2(g_wow->cache, task->m2);
				break;
			}
			struct wow_m2_file *m2 = wow_m2_file_new(file);
			wow_mpq_file_delete(file);
			if (!m2)
			{
				LOG_ERROR("failed to create m2 from file %s", task->m2->filename);
				cache_unref_by_ref_m2(g_wow->cache, task->m2);
				break;
			}
			int ret = gx_m2_load(task->m2, m2);
			wow_m2_file_delete(m2);
			switch (ret)
			{
				case -1:
					cache_unref_by_ref_m2(g_wow->cache, task->m2);
					break;
				case 0:
					LOG_ERROR("failed to load m2");
					cache_unref_by_ref_m2(g_wow->cache, task->m2);
					break;
				case 1:
					cache_lock_m2(g_wow->cache);
					if (!gx_m2_post_load(task->m2))
						cache_unref_by_ref_unmutexed_m2(g_wow->cache, task->m2);
					cache_unlock_m2(g_wow->cache);
					break;
			}
			break;
		}
		case ASYNC_TASK_M2_UNLOAD:
		{
			gx_m2_delete(task->m2);
			break;
		}
		case ASYNC_TASK_BLP_LOAD:
		{
			struct wow_mpq_file *file = wow_mpq_get_file(worker->mpq_compound, task->blp->filename);
			if (!file)
			{
				LOG_WARN("BLP file not found: %s", task->blp->filename);
				cache_lock_blp(g_wow->cache);
				task->blp->loading = false;
				cache_unref_by_ref_unmutexed_blp(g_wow->cache, task->blp);
				cache_unlock_blp(g_wow->cache);
				break;
			}
			struct wow_blp_file *blp = wow_blp_file_new(file);
			wow_mpq_file_delete(file);
			if (!blp)
			{
				LOG_ERROR("failed to create blp from file %s", task->blp->filename);
				cache_unref_by_ref_blp(g_wow->cache, task->blp);
				break;
			}
			if (!blp_texture_load(task->blp, blp))
			{
				LOG_ERROR("failed to load blp");
				cache_unref_by_ref_blp(g_wow->cache, task->blp);
			}
			task->blp->loading = false;
			wow_blp_file_delete(blp);
			break;
		}
		case ASYNC_TASK_BLP_UNLOAD:
		{
			blp_texture_delete(task->blp);
			break;
		}
		case ASYNC_TASK_MAP_TILE_LOAD:
		{
			struct wow_mpq_file *file = wow_mpq_get_file(worker->mpq_compound, task->map_tile->filename);
			if (!file)
			{
				LOG_WARN("ADT file not found: %s", task->map_tile->filename);
				task->map_tile->flags &= ~MAP_TILE_LOADING;
				break;
			}
			struct wow_adt_file *adt = wow_adt_file_new(file);
			wow_mpq_file_delete(file);
			if (!adt)
			{
				LOG_ERROR("failed to create adt from file %s", task->map_tile->filename);
				break;
			}
			if (map_tile_load_childs(task->map_tile, adt))
			{
				if (!map_tile_load(task->map_tile, adt))
					LOG_ERROR("failed to load adt");
			}
			else
			{
				LOG_ERROR("failed to load adt childs");
			}
			task->map_tile->flags &= ~MAP_TILE_LOADING;
			wow_adt_file_delete(adt);
			break;
		}
		case ASYNC_TASK_MAP_TILE_UNLOAD:
		{
			map_tile_delete(task->map_tile);
			break;
		}
		case ASYNC_TASK_MINIMAP_TEXTURE:
		{
			generate_minimap_texture(g_wow->map);
			break;
		}
		case ASYNC_TASK_CLOUDS_TEXTURE:
		{
			generate_clouds_texture(g_wow->map->gx_skybox);
			break;
		}
		case ASYNC_TASK_LAST:
			break;
	}
}

bool find_task(struct loader *loader, struct async_task *task)
{
	for (size_t i = 0; i < ASYNC_TASK_LAST; ++i)
	{
		JKS_LIST_FOREACH(iter, &loader->tasks[i])
		{
			struct async_task *t = jks_list_iterator_get(&iter);
			switch (t->type)
			{
				case ASYNC_TASK_WMO_UNLOAD:
					if (t->wmo->loading)
						goto next_iter;
					break;
				case ASYNC_TASK_M2_UNLOAD:
					break;
				case ASYNC_TASK_BLP_UNLOAD:
					break;
				case ASYNC_TASK_MAP_TILE_UNLOAD:
					break;
				case ASYNC_TASK_WMO_GROUP_UNLOAD:
					if (t->wmo_group->loading)
						goto next_iter;
					break;
				case ASYNC_TASK_WMO_LOAD:
					t->wmo->loading = true;
					break;
				case ASYNC_TASK_M2_LOAD:
					t->m2->loading = true;
					break;
				case ASYNC_TASK_BLP_LOAD:
					t->blp->loading = true;
					break;
				case ASYNC_TASK_MAP_TILE_LOAD:
					t->map_tile->flags |= MAP_TILE_LOADING;
					break;
				case ASYNC_TASK_WMO_GROUP_LOAD:
					t->wmo_group->loading = true;
					break;
				case ASYNC_TASK_MINIMAP_TEXTURE:
					break;
				case ASYNC_TASK_CLOUDS_TEXTURE:
					break;
				case ASYNC_TASK_LAST:
					break;
			}
			*task = *t;
			jks_list_iterator_erase(&loader->tasks[i], &iter);
			return true;
next_iter:
			continue;
		}
	}
	return false;
}

void *loader_run(void *data)
{
	struct loader *loader = data;
	size_t id = (size_t)-1;
	for (size_t i = 0; i < loader->workers.size; ++i)
	{
		if (pthread_equal(pthread_self(), JKS_ARRAY_GET(&loader->workers, i, struct async_worker)->thread))
		{
			id = i;
			break;
		}
	}
	if (id == (size_t)-1)
		return NULL;
	struct async_worker *worker = JKS_ARRAY_GET(&loader->workers, id, struct async_worker);
	worker->mpq_compound = wow_mpq_compound_new();
	if (!worker->mpq_compound)
	{
		LOG_ERROR("failed to create mpq compound");
		return NULL;
	}
	if (!wow_load_compound(worker->mpq_compound))
	{
		LOG_ERROR("failed to load mpq compound");
		return NULL;
	}
	while (loader->running)
	{
		struct async_task task = {ASYNC_TASK_LAST, {NULL}};
		bool found;
		{
			pthread_mutex_lock(&loader->async_mutex);
			found = find_task(loader, &task);
			pthread_mutex_unlock(&loader->async_mutex);
		}
		if (found)
			execute_task(worker, &task);
		else
			usleep(5000);
	}
	wow_mpq_compound_delete(worker->mpq_compound);
	return NULL;
}

void *loader_cull_run(void *data)
{
	struct loader *loader = data;
	pthread_mutex_lock(&loader->cull_mutex);
	while (loader->running)
	{
		if (!loader->cull_ready)
		{
			pthread_cond_wait(&loader->cull_run_condition, &loader->cull_mutex);
			continue;
		}
		loader->cull_ready = false;
		pthread_mutex_unlock(&loader->cull_mutex);
		if (g_wow->map)
			map_cull(g_wow->map, g_wow->cull_frame);
		pthread_mutex_lock(&loader->cull_mutex);
		loader->cull_ended++;
		pthread_cond_signal(&loader->cull_end_condition);
	}
	pthread_mutex_unlock(&loader->cull_mutex);
	return NULL;
}

struct loader *loader_new(void)
{
	struct loader *loader = mem_malloc(MEM_GENERIC, sizeof(*loader));
	if (!loader)
		return NULL;
	loader->cull_ready = false;
	loader->cull_ended = true;
	loader->running = true;
	jks_array_init(&loader->cull_threads, sizeof(pthread_t), NULL, &jks_array_memory_fn_GENERIC);
	for (size_t i = 0; i < sizeof(loader->objects_gc) / sizeof(*loader->objects_gc); ++i)
		jks_list_init(&loader->objects_gc[i], sizeof(struct loader_object), NULL, &jks_list_memory_fn_GENERIC);
	jks_list_init(&loader->objects_to_init_buffer, sizeof(struct loader_object), NULL, &jks_list_memory_fn_GENERIC);
	for (size_t i = 0; i < LOADER_LAST; ++i)
		jks_list_init(&loader->objects_to_init[i], sizeof(struct loader_object), NULL, &jks_list_memory_fn_GENERIC);
	for (size_t i = 0; i < ASYNC_TASK_LAST; ++i)
		jks_list_init(&loader->tasks[i], sizeof(struct async_task), NULL, &jks_list_memory_fn_GENERIC);
	jks_array_init(&loader->workers, sizeof(struct async_worker), NULL, &jks_array_memory_fn_GENERIC);
	if (!jks_array_resize(&loader->workers, ASYNC_THREADS))
	{
		LOG_ERROR("failed to resize workers array");
		goto err;
	}
	pthread_mutex_init(&loader->cull_mutex, NULL);
	pthread_mutex_init(&loader->gc_mutex, NULL);
	pthread_mutex_init(&loader->mutex, NULL);
	pthread_mutex_init(&loader->async_mutex, NULL);
	if (pthread_cond_init(&loader->cull_run_condition, NULL))
	{
		LOG_ERROR("failed to init pthread cull run condition");
		goto err;
	}
	if (pthread_cond_init(&loader->cull_end_condition, NULL))
	{
		LOG_ERROR("failed to init pthread cull end condition");
		goto err;
	}
	for (size_t i = 0; i < loader->workers.size; ++i)
	{
		char title[256];
		snprintf(title, sizeof(title), "WoW loader %d", (int)i);
		struct async_worker *worker = JKS_ARRAY_GET(&loader->workers, i, struct async_worker);
		if (pthread_create(&worker->thread, NULL, loader_run, loader))
		{
			LOG_ERROR("failed to create worker pthread");
			goto err;
		}
	}
	if (!jks_array_reserve(&loader->cull_threads, CULL_THREADS))
	{
		LOG_ERROR("failed to resize cull threads");
		goto err;
	}
	for (size_t i = 0; i < CULL_THREADS; ++i)
	{
		if (pthread_create(JKS_ARRAY_GET(&loader->cull_threads, i, pthread_t), NULL, loader_cull_run, loader))
		{
			LOG_ERROR("failed to create cull pthread");
			goto err;
		}
	}
	return loader;

err:
	mem_free(MEM_GENERIC, loader);
	return NULL;
}

void loader_delete(struct loader *loader)
{
	if (!loader)
		return;
	loader->running = false;
	for (size_t i = 0; i < loader->workers.size; ++i)
	{
		struct async_worker *worker = JKS_ARRAY_GET(&loader->workers, i, struct async_worker);
		pthread_join(worker->thread, NULL);
	}
	jks_array_destroy(&loader->workers);
	pthread_cond_broadcast(&loader->cull_run_condition);
	for (size_t i = 0; i < loader->cull_threads.size; ++i)
	{
		pthread_t *thread = JKS_ARRAY_GET(&loader->cull_threads, i, pthread_t);
		pthread_join(*thread, NULL);
	}
	jks_array_destroy(&loader->cull_threads);
	pthread_mutex_destroy(&loader->cull_mutex);
	pthread_mutex_destroy(&loader->gc_mutex);
	pthread_mutex_destroy(&loader->mutex);
	pthread_mutex_destroy(&loader->async_mutex);
	pthread_cond_destroy(&loader->cull_run_condition);
	pthread_cond_destroy(&loader->cull_end_condition);
	for (size_t i = 0; i < sizeof(loader->objects_gc) / sizeof(*loader->objects_gc); ++i)
		jks_list_destroy(&loader->objects_gc[i]);
	jks_list_destroy(&loader->objects_to_init_buffer);
	for (size_t i = 0; i < LOADER_LAST; ++i)
		jks_list_destroy(&loader->objects_to_init[i]);
	for (size_t i = 0; i < ASYNC_TASK_LAST; ++i)
		jks_list_destroy(&loader->tasks[i]);
	mem_free(MEM_GENERIC, loader);
}

bool loader_has_async(struct loader *loader)
{
	for (size_t i = 0; i < ASYNC_TASK_LAST; ++i)
	{
		if (loader->tasks[i].size)
			return true;
	}
	return false;
}

bool loader_has_loading(struct loader *loader)
{
	if (loader->objects_to_init_buffer.size)
		return true;
	for (size_t i = 0; i < LOADER_LAST; ++i)
	{
		if (loader->objects_to_init[i].size)
			return true;
	}
	return false;
}

bool loader_has_gc(struct loader *loader)
{
	for (size_t i = 0; i < sizeof(loader->objects_gc) / sizeof(*loader->objects_gc); ++i)
	{
		if (loader->objects_gc[i].size)
			return true;
	}
	return false;
}

static void queue_buffers(struct loader *loader)
{
	pthread_mutex_lock(&loader->mutex);
	for (; loader->objects_to_init_buffer.size; jks_list_erase_head(&loader->objects_to_init_buffer))
	{
		struct loader_object *object = jks_list_get_head(&loader->objects_to_init_buffer);
		if (!jks_list_push_tail(&loader->objects_to_init[object->type], object))
			LOG_ERROR("failed to add object to init queue");
	}
	pthread_mutex_unlock(&loader->mutex);
}

static void do_init(struct loader *loader)
{
	queue_buffers(loader);
#define MIN_DURATION 5000000
#define FRAME_DURATION 16000000 /* little bit less than frame for 60 fps */
	uint64_t started = nanotime();
	uint64_t tmp = started;
#define DO_INIT(loader_type, obj_type, fn, load_exp) \
	while (loader->objects_to_init[loader_type].size && (tmp - started < MIN_DURATION || tmp - g_wow->frametime < FRAME_DURATION)) \
	{ \
		obj_type *obj = ((struct loader_object*)jks_list_get_head(&loader->objects_to_init[loader_type]))->data; \
		switch (fn(obj)) \
		{ \
			case -1: \
				LOG_ERROR("object loading failed"); \
				/* FALLTHROUGH */ \
			case 1: \
				jks_list_erase_head(&loader->objects_to_init[loader_type]); \
				load_exp; \
			case 0: \
				break; \
		} \
		tmp = nanotime(); \
	}

	DO_INIT(LOADER_MAP_TILE , struct map_tile    , map_tile_initialize    ,);
	DO_INIT(LOADER_BLP      , struct blp_texture , blp_texture_initialize , cache_unref_by_ref_blp(g_wow->cache, obj));
	DO_INIT(LOADER_M2       , struct gx_m2       , gx_m2_initialize       , cache_unref_by_ref_m2(g_wow->cache, obj));
	DO_INIT(LOADER_WMO      , struct gx_wmo      , gx_wmo_initialize      , cache_unref_by_ref_wmo(g_wow->cache, obj));
	DO_INIT(LOADER_WMO_GROUP, struct gx_wmo_group, gx_wmo_group_initialize,);
#undef DO_INIT
}

bool loader_push(struct loader *loader, enum async_task_type type, void *data)
{
	if (!loader->running)
		return false;
	pthread_mutex_lock(&loader->async_mutex);
	struct async_task *task = jks_list_push_tail(&loader->tasks[type], NULL);
	if (!task)
	{
		pthread_mutex_unlock(&loader->async_mutex);
		LOG_ERROR("failed to add task to queue");
		return false;
	}
	task->type = type;
	task->data = data;
	pthread_mutex_unlock(&loader->async_mutex);
	return true;
}

static void do_gc(struct loader *loader)
{
	pthread_mutex_lock(&loader->gc_mutex);
	for (;loader->objects_gc[g_wow->draw_frame_id].size; jks_list_erase_head(&loader->objects_gc[g_wow->draw_frame_id]))
	{
		enum async_task_type async_task;
		struct loader_object *object = jks_list_get_head(&loader->objects_gc[g_wow->draw_frame_id]);
		switch (object->type)
		{
			case LOADER_MAP_TILE:
				async_task = ASYNC_TASK_MAP_TILE_UNLOAD;
				break;
			case LOADER_BLP:
				async_task = ASYNC_TASK_BLP_UNLOAD;
				break;
			case LOADER_M2:
				async_task = ASYNC_TASK_M2_UNLOAD;
				break;
			case LOADER_WMO:
				async_task = ASYNC_TASK_WMO_UNLOAD;
				break;
			case LOADER_WMO_GROUP:
				async_task = ASYNC_TASK_WMO_GROUP_UNLOAD;
				break;
			default:
				LOG_ERROR("unknown loader object type: %d", object->type);
				continue;
		}
		loader_push(loader, async_task, object->data);
	}
	pthread_mutex_unlock(&loader->gc_mutex);
}

void loader_tick(struct loader *loader)
{
	do_init(loader);
	do_gc(loader);
}

static bool loader_init_object(struct loader *loader, enum loader_object_type type, void *ptr)
{
	struct loader_object object;
	object.type = type;
	object.data = ptr;
	pthread_mutex_lock(&loader->mutex);
	if (!jks_list_push_tail(&loader->objects_to_init_buffer, &object))
	{
		pthread_mutex_unlock(&loader->mutex);
		LOG_ERROR("failed to add object to init buffer");
		return false;
	}
	pthread_mutex_unlock(&loader->mutex);
	return true;
}

bool loader_init_wmo_group(struct loader *loader, struct gx_wmo_group *wmo_group)
{
	return loader_init_object(loader, LOADER_WMO_GROUP, wmo_group);
}

bool loader_init_map_tile(struct loader *loader, struct map_tile *map_tile)
{
	return loader_init_object(loader, LOADER_MAP_TILE, map_tile);
}

bool loader_init_wmo(struct loader *loader, struct gx_wmo *wmo)
{
	return loader_init_object(loader, LOADER_WMO, wmo);
}

bool loader_init_blp(struct loader *loader, struct blp_texture *texture)
{
	return loader_init_object(loader, LOADER_BLP, texture);
}

bool loader_init_m2(struct loader *loader, struct gx_m2 *m2)
{
	return loader_init_object(loader, LOADER_M2, m2);
}

static bool loader_gc_object(struct loader *loader, enum loader_object_type type, void *data)
{
	struct loader_object object;
	object.type = type;
	object.data = data;
	pthread_mutex_lock(&loader->gc_mutex);
	if (!jks_list_push_tail(&loader->objects_gc[g_wow->cull_frame_id], &object))
	{
		pthread_mutex_unlock(&loader->gc_mutex);
		LOG_ERROR("failed to push object of type %d to gc", type);
		return false;
	}
	pthread_mutex_unlock(&loader->gc_mutex);
	return true;
}

bool loader_gc_wmo_group(struct loader *loader, struct gx_wmo_group *wmo_group)
{
	return loader_gc_object(loader, LOADER_WMO_GROUP, wmo_group);
}

bool loader_gc_map_tile(struct loader *loader, struct map_tile *map_tile)
{
	return loader_gc_object(loader, LOADER_MAP_TILE, map_tile);
}

bool loader_gc_wmo(struct loader *loader, struct gx_wmo *wmo)
{
	return loader_gc_object(loader, LOADER_WMO, wmo);
}

bool loader_gc_blp(struct loader *loader, struct blp_texture *texture)
{
	return loader_gc_object(loader, LOADER_BLP, texture);
}

bool loader_gc_m2(struct loader *loader, struct gx_m2 *m2)
{
	return loader_gc_object(loader, LOADER_M2, m2);
}

void loader_start_cull(struct loader *loader)
{
	pthread_mutex_lock(&loader->cull_mutex);
	loader->cull_ended = 0;
	loader->cull_ready = true;
	pthread_cond_broadcast(&loader->cull_run_condition);
	pthread_mutex_unlock(&loader->cull_mutex);
}

void loader_wait_cull(struct loader *loader)
{
	pthread_mutex_lock(&loader->cull_mutex);
	while (loader->cull_ended < CULL_THREADS)
		pthread_cond_wait(&loader->cull_end_condition, &loader->cull_mutex);
	pthread_mutex_unlock(&loader->cull_mutex);
}
