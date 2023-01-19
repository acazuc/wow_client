#include "cache.h"

#include "font/model.h"

#include "gx/wmo.h"
#include "gx/m2.h"

#include "map/tile.h"

#include "memory.h"
#include "log.h"
#include "wow.h"
#include "blp.h"
#include "dbc.h"

#include <libwow/mpq.h>
#include <libwow/dbc.h>

#include <inttypes.h>
#include <string.h>
#include <assert.h>

MEMORY_DECL(GENERIC);

struct cache_vtable;

struct cache_entry
{
	void *key;
	uint32_t refs;
};

struct cache_type
{
	const struct cache_vtable *vtable;
	struct jks_hmap keys; /* key, val */
	struct jks_hmap refs; /* val, struct cache_entry* */
	pthread_mutex_t mutex;
	bool mutexed;
};

struct cache
{
	struct cache_type blp; /* char*, struct blp_texture* */
	struct cache_type wmo; /* char*, struct gx_wmo* */
	struct cache_type map_wmo; /* uint32_t, struct map_wmo* */
	struct cache_type m2; /* char*, struct gx_m2* */
	struct cache_type map_m2; /* uint32_t. struct map_m2* */
	struct cache_type font; /* char*, struct font_model* */
	struct cache_type dbc; /* char*, struct dbc* */
	struct cache_type mpq; /* char*, struct wow_mpq_file* */
};

typedef bool (*cache_constructor_t)(const void *key, void **ref);
typedef void (*cache_destructor_t)(void *ref);
typedef bool (*cache_key_dup_t)(const void *key, void **d);
typedef void (*cache_key_free_t)(void *key);

#define CACHE_LOCK(cache) \
do \
{ \
	if ((cache)->mutexed) \
		pthread_mutex_lock(&(cache)->mutex); \
} while (0)

#define CACHE_UNLOCK(cache) \
do \
{ \
	if ((cache)->mutexed) \
		pthread_mutex_unlock(&(cache)->mutex); \
} while (0)

struct cache_vtable
{
	const char *name;
	cache_constructor_t constructor;
	cache_destructor_t destructor;
	cache_key_dup_t key_dup;
	cache_key_free_t key_free;
};

static bool key_string_dup(const void *key, void **d)
{
	*d = mem_strdup(MEM_GENERIC, (char*)key);
	return *d != NULL;
}

static bool blp_constructor(const void *key, void **ref)
{
	*ref = blp_texture_new((const char*)key);
	return *ref != NULL;
}

static void blp_destructor(void *ref)
{
	blp_texture_ask_unload(*(struct blp_texture**)ref);
}

static void blp_hmap_destructor(jks_hmap_key_t key, void *ref)
{
	mem_free(MEM_GENERIC, key.ptr);
	blp_destructor(ref);
}

static bool wmo_constructor(const void *key, void **ref)
{
	*ref = gx_wmo_new((const char*)key);
	return *ref != NULL;
}

static void wmo_destructor(void *ref)
{
	gx_wmo_ask_unload(*(struct gx_wmo**)ref);
}

static void wmo_hmap_destructor(jks_hmap_key_t key, void *ref)
{
	mem_free(MEM_GENERIC, key.ptr);
	wmo_destructor(ref);
}

static bool map_wmo_constructor(const void *key, void **ref)
{
	(void)key;
	*ref = map_wmo_new();
	return *ref != NULL;
}

static void map_wmo_destructor(void *ref)
{
	map_wmo_delete(*(struct map_wmo**)ref);
}

static void map_wmo_hmap_destructor(jks_hmap_key_t key, void *ref)
{
	(void)key;
	map_wmo_destructor(ref);
}

static bool m2_constructor(const void *key, void **ref)
{
	*ref = gx_m2_new((const char*)key);
	return *ref != NULL;
}

static void m2_destructor(void *ref)
{
	gx_m2_ask_unload(*(struct gx_m2**)ref);
}

static void m2_hmap_destructor(jks_hmap_key_t key, void *ref)
{
	mem_free(MEM_GENERIC, key.ptr);
	m2_destructor(ref);
}

static bool map_m2_constructor(const void *key, void **ref)
{
	(void)key;
	*ref = map_m2_new();
	return *ref != NULL;
}

static void map_m2_destructor(void *ref)
{
	map_m2_delete(*(struct map_m2**)ref);
}

static void map_m2_hmap_destructor(jks_hmap_key_t key, void *ref)
{
	(void)key;
	map_m2_destructor(ref);
}

static bool font_constructor(const void *key, void **ref)
{
	wow_mpq_file_t *file;
	if (!cache_ref_by_key_mpq(g_wow->cache, (const char*)key, &file))
		return false;
	*ref = font_model_new((const char*)file->data, file->size);
	cache_unref_by_ref_mpq(g_wow->cache, file);
	return *ref != NULL;
}

static void font_destructor(void *ref)
{
	font_model_delete(*(struct font_model**)ref);
}

static void font_hmap_destructor(jks_hmap_key_t key, void *ref)
{
	mem_free(MEM_GENERIC, key.ptr);
	font_destructor(ref);
}

static bool dbc_constructor(const void *key, void **ref)
{
	wow_mpq_file_t *mpq_file;
	if (!cache_ref_by_key_mpq(g_wow->cache, (const char*)key, &mpq_file))
		return false;
	wow_dbc_file_t *dbc_file = wow_dbc_file_new(mpq_file);
	cache_unref_by_ref_mpq(g_wow->cache, mpq_file);
	if (!dbc_file)
		return false;
	*ref = dbc_new(dbc_file);
	if (!*ref)
		wow_dbc_file_delete(dbc_file);
	return *ref != NULL;
}

static void dbc_destructor(void *ref)
{
	dbc_delete(*(struct dbc**)ref);
}

static void dbc_hmap_destructor(jks_hmap_key_t key, void *ref)
{
	mem_free(MEM_GENERIC, key.ptr);
	dbc_destructor(ref);
}

static bool mpq_constructor(const void *key, void **ref)
{
	*ref = wow_mpq_get_file(g_wow->mpq_compound, (const char*)key);
	return *ref != NULL;
}

static void mpq_destructor(void *ref)
{
	wow_mpq_file_delete(*(wow_mpq_file_t**)ref);
}

static void mpq_hmap_destructor(jks_hmap_key_t key, void *ref)
{
	mem_free(MEM_GENERIC, key.ptr);
	mpq_destructor(ref);
}

static const struct cache_vtable blp_vtable =
{
	.name = "blp",
	.constructor = blp_constructor,
	.destructor  = blp_destructor,
	.key_dup     = key_string_dup,
	.key_free    = mem_free_GENERIC,
};

static const struct cache_vtable wmo_vtable =
{
	.name = "wmo",
	.constructor = wmo_constructor,
	.destructor  = wmo_destructor,
	.key_dup     = key_string_dup,
	.key_free    = mem_free_GENERIC,
};

static const struct cache_vtable map_wmo_vtable =
{
	.name = "map_wmo",
	.constructor = map_wmo_constructor,
	.destructor  = map_wmo_destructor,
	.key_dup     = NULL,
	.key_free    = NULL,
};

static const struct cache_vtable m2_vtable =
{
	.name = "m2",
	.constructor = m2_constructor,
	.destructor  = m2_destructor,
	.key_dup     = key_string_dup,
	.key_free    = mem_free_GENERIC,
};

static const struct cache_vtable map_m2_vtable =
{
	.name = "map_m2",
	.constructor = map_m2_constructor,
	.destructor  = map_m2_destructor,
	.key_dup     = NULL,
	.key_free    = NULL,
};

static const struct cache_vtable font_vtable =
{
	.name = "font",
	.constructor = font_constructor,
	.destructor  = font_destructor,
	.key_dup     = key_string_dup,
	.key_free    = mem_free_GENERIC,
};

static const struct cache_vtable dbc_vtable =
{
	.name = "dbc",
	.constructor = dbc_constructor,
	.destructor  = dbc_destructor,
	.key_dup     = key_string_dup,
	.key_free    = mem_free_GENERIC,
};

static const struct cache_vtable mpq_vtable =
{
	.name = "mpq",
	.constructor = mpq_constructor,
	.destructor  = mpq_destructor,
	.key_dup     = key_string_dup,
	.key_free    = mem_free_GENERIC,
};

static void cache_type_init(struct cache_type *cache, bool mutexed, jks_hmap_destructor_t destructor, jks_hmap_hash_fn_t hash_fn, jks_hmap_cmp_fn_t cmp_fn, const struct cache_vtable *vtable)
{
	jks_hmap_init(&cache->keys, sizeof(void*), destructor, hash_fn, cmp_fn, &jks_hmap_memory_fn_GENERIC);
	jks_hmap_init(&cache->refs, sizeof(void*), NULL, jks_hmap_hash_ptr, jks_hmap_cmp_ptr, &jks_hmap_memory_fn_GENERIC);
	cache->vtable = vtable;
	cache->mutexed = mutexed;
	if (mutexed)
		pthread_mutex_init(&cache->mutex, NULL);
}

struct cache *cache_new(void)
{
	struct cache *cache = mem_malloc(MEM_GENERIC, sizeof(*cache));
	if (!cache)
		return NULL;
	cache_type_init(&cache->blp, true, blp_hmap_destructor, jks_hmap_hash_str, jks_hmap_cmp_str, &blp_vtable);
	cache_type_init(&cache->wmo, true, wmo_hmap_destructor, jks_hmap_hash_str, jks_hmap_cmp_str, &wmo_vtable);
	cache_type_init(&cache->map_wmo, true, map_wmo_hmap_destructor, jks_hmap_hash_u32, jks_hmap_cmp_u32, &map_wmo_vtable);
	cache_type_init(&cache->m2, true, m2_hmap_destructor, jks_hmap_hash_str, jks_hmap_cmp_str, &m2_vtable);
	cache_type_init(&cache->map_m2, true, map_m2_hmap_destructor, jks_hmap_hash_u32, jks_hmap_cmp_u32, &map_m2_vtable);
	cache_type_init(&cache->font, true, font_hmap_destructor, jks_hmap_hash_str, jks_hmap_cmp_str, &font_vtable);
	cache_type_init(&cache->dbc, true, dbc_hmap_destructor, jks_hmap_hash_str, jks_hmap_cmp_str, &dbc_vtable);
	cache_type_init(&cache->mpq, true, mpq_hmap_destructor, jks_hmap_hash_str, jks_hmap_cmp_str, &mpq_vtable);
	return cache;
}

static void cache_type_destroy(struct cache_type *cache)
{
	jks_hmap_destroy(&cache->keys);
	jks_hmap_destroy(&cache->refs);
	if (cache->mutexed)
		pthread_mutex_destroy(&cache->mutex);
}

void cache_delete(struct cache *cache)
{
	if (!cache)
		return;
	cache_type_destroy(&cache->blp);
	cache_type_destroy(&cache->wmo);
	cache_type_destroy(&cache->map_wmo);
	cache_type_destroy(&cache->m2);
	cache_type_destroy(&cache->map_m2);
	cache_type_destroy(&cache->font);
	cache_type_destroy(&cache->dbc);
	cache_type_destroy(&cache->mpq);
	mem_free(MEM_GENERIC, cache);
}

void cache_print(struct cache *cache)
{
#define PRINT(type) \
	JKS_HMAP_FOREACH(iter, &cache->type.refs) \
	{ \
		struct cache_entry *entry = *(struct cache_entry**)jks_hmap_iterator_get_value(&iter); \
		LOG_INFO(#type " %s referenced %" PRIu32 " times", (char*)entry->key, entry->refs); \
	} \
	LOG_INFO(#type "s: %u / %u", cache->type.refs.size, cache->type.keys.size);

	PRINT(blp);
	PRINT(wmo);
	PRINT(map_wmo);
	PRINT(m2);
	PRINT(map_m2);
	PRINT(font);
	PRINT(dbc);
	PRINT(mpq);
#undef PRINT
}

static bool cache_ref_by_ref(struct cache_type *cache, void *ref)
{
	jks_hmap_iterator_t iter = jks_hmap_iterator_find(&cache->refs, JKS_HMAP_KEY_PTR(ref));
	if (jks_hmap_iterator_is_end(&cache->refs, &iter))
		return false;
	struct cache_entry *entry = *(struct cache_entry**)jks_hmap_iterator_get_value(&iter);
	entry->refs++;
	return true;
}

static bool cache_ref_by_key(struct cache_type *cache, const void *key, void **ref)
{
	assert(ref);
	jks_hmap_iterator_t iter = jks_hmap_iterator_find(&cache->keys, JKS_HMAP_KEY_PTR((void*)key));
	if (!jks_hmap_iterator_is_end(&cache->keys, &iter))
	{
		*ref = *(void**)jks_hmap_iterator_get_value(&iter);
		return cache_ref_by_ref(cache, *ref);
	}
	struct cache_entry *entry = mem_malloc(MEM_GENERIC, sizeof(*entry));
	if (!entry)
		return false;
	entry->refs = 1;
	if (cache->vtable->key_dup)
	{
		if (!cache->vtable->key_dup(key, &entry->key))
		{
			mem_free(MEM_GENERIC, entry);
			return false;
		}
	}
	else
	{
		entry->key = (void*)key;
	}
	if (!cache->vtable->constructor(key, ref))
	{
		if (cache->vtable->key_free)
			cache->vtable->key_free(entry->key);
		mem_free(MEM_GENERIC, entry);
		return false;
	}
	if (!jks_hmap_set(&cache->keys, JKS_HMAP_KEY_PTR(entry->key), ref))
	{
		if (cache->vtable->key_free)
			cache->vtable->key_free(entry->key);
		cache->vtable->destructor(*ref);
		mem_free(MEM_GENERIC, entry);
		return false;
	}
	if (!jks_hmap_set(&cache->refs, JKS_HMAP_KEY_PTR(*ref), &entry))
	{
		jks_hmap_erase(&cache->keys, JKS_HMAP_KEY_PTR((void*)key));
		mem_free(MEM_GENERIC, entry);
		return false;
	}
	return true;
}

static void cache_unref_by_ref(struct cache_type *cache, void *ref)
{
	jks_hmap_iterator_t iter = jks_hmap_iterator_find(&cache->refs, JKS_HMAP_KEY_PTR(ref));
	if (jks_hmap_iterator_is_end(&cache->refs, &iter))
	{
		assert("!unref unexisting ref");
		return;
	}
	struct cache_entry *entry = *(struct cache_entry**)jks_hmap_iterator_get_value(&iter);
	if (--entry->refs)
		return;
	if (!jks_hmap_erase(&cache->keys, JKS_HMAP_KEY_PTR(entry->key)))
		assert(!"didn't erased key");
	if (!jks_hmap_erase(&cache->refs, JKS_HMAP_KEY_PTR(ref)))
		assert(!"didn't erased ref");
	mem_free(MEM_GENERIC, entry);
}

static void cache_unref_by_key(struct cache_type *cache, const void *key)
{
	jks_hmap_iterator_t iter = jks_hmap_iterator_find(&cache->keys, JKS_HMAP_KEY_PTR((void*)key));
	if (jks_hmap_iterator_is_end(&cache->keys, &iter))
	{
		assert(!"unref unexisting key");
		return;
	}
	cache_unref_by_ref(cache, (void**)jks_hmap_iterator_get_value(&iter));
}

static size_t cache_refs_count_by_ref(struct cache_type *cache, const void *ref)
{
	jks_hmap_iterator_t iter = jks_hmap_iterator_find(&cache->refs, JKS_HMAP_KEY_PTR((void*)ref));
	if (jks_hmap_iterator_is_end(&cache->refs, &iter))
		return 0;
	struct cache_entry *entry = *(struct cache_entry**)jks_hmap_iterator_get_value(&iter);
	return entry->refs;
}

static size_t cache_refs_count_by_key(struct cache_type *cache, const void *key)
{
	jks_hmap_iterator_t iter = jks_hmap_iterator_find(&cache->keys, JKS_HMAP_KEY_PTR((void*)key));
	if (jks_hmap_iterator_is_end(&cache->keys, &iter))
		return 0;
	return cache_refs_count_by_ref(cache, *(void**)jks_hmap_iterator_get_value(&iter));
}

#define CACHE_FUNCTIONS(key_type, ref_type, name) \
bool cache_ref_by_key_##name(struct cache *cache, const key_type key, ref_type *ref) \
{ \
	CACHE_LOCK(&cache->name); \
	ref_type ret = (ref_type)(intptr_t)cache_ref_by_key(&cache->name, (const void*)(intptr_t)key, (void**)ref); \
	CACHE_UNLOCK(&cache->name); \
	return ret; \
} \
bool cache_ref_by_ref_##name(struct cache *cache, ref_type ref) \
{ \
	CACHE_LOCK(&cache->name); \
	ref_type ret = (ref_type)(intptr_t)cache_ref_by_ref(&cache->name, (void*)(intptr_t)ref); \
	CACHE_UNLOCK(&cache->name); \
	return ret; \
} \
void cache_unref_by_key_##name(struct cache *cache, const key_type key) \
{ \
	CACHE_LOCK(&cache->name); \
	cache_unref_by_key(&cache->name, (const void*)(intptr_t)key); \
	CACHE_UNLOCK(&cache->name); \
} \
void cache_unref_by_ref_##name(struct cache *cache, ref_type ref) \
{ \
	CACHE_LOCK(&cache->name); \
	cache_unref_by_ref(&cache->name, (void*)(intptr_t)ref); \
	CACHE_UNLOCK(&cache->name); \
} \
size_t cache_refs_count_by_key_##name(struct cache *cache, const key_type key) \
{ \
	CACHE_LOCK(&cache->name); \
	size_t ret = cache_refs_count_by_key(&cache->name, (const void*)(intptr_t)key); \
	CACHE_UNLOCK(&cache->name); \
	return ret; \
} \
size_t cache_refs_count_by_ref_##name(struct cache *cache, const ref_type ref) \
{ \
	CACHE_LOCK(&cache->name); \
	size_t ret = cache_refs_count_by_ref(&cache->name, (const void*)(intptr_t)ref); \
	CACHE_UNLOCK(&cache->name); \
	return ret; \
} \
bool cache_ref_by_key_unmutexed_##name(struct cache *cache, const key_type key, ref_type *ref) \
{ \
	return (ref_type)(intptr_t)cache_ref_by_key(&cache->name, (const void*)(intptr_t)key, (void**)ref); \
} \
bool cache_ref_by_ref_unmutexed_##name(struct cache *cache, ref_type ref) \
{ \
	return (ref_type)(intptr_t)cache_ref_by_ref(&cache->name, (void*)(intptr_t)ref); \
} \
void cache_unref_by_key_unmutexed_##name(struct cache *cache, const key_type key) \
{ \
	cache_unref_by_key(&cache->name, (const void*)(intptr_t)key); \
} \
void cache_unref_by_ref_unmutexed_##name(struct cache *cache, ref_type ref) \
{ \
	cache_unref_by_ref(&cache->name, (void*)(intptr_t)ref); \
} \
size_t cache_refs_count_by_key_unmutexed_##name(struct cache *cache, const key_type key) \
{ \
	return cache_refs_count_by_key(&cache->name, (const void*)(intptr_t)key); \
} \
size_t cache_refs_count_by_ref_unmutexed_##name(struct cache *cache, const ref_type ref) \
{ \
	return cache_refs_count_by_ref(&cache->name, (const void*)(intptr_t)ref); \
} \
void cache_lock_##name(struct cache *cache) \
{ \
	CACHE_LOCK(&cache->name); \
} \
void cache_unlock_##name(struct cache *cache) \
{ \
	CACHE_UNLOCK(&cache->name); \
}

CACHE_FUNCTIONS(char*, struct blp_texture*, blp);
CACHE_FUNCTIONS(char*, struct gx_wmo*, wmo);
CACHE_FUNCTIONS(uint32_t, struct map_wmo*, map_wmo);
CACHE_FUNCTIONS(char*, struct gx_m2*, m2);
CACHE_FUNCTIONS(uint32_t, struct map_m2*, map_m2);
CACHE_FUNCTIONS(char*, struct font_model*, font);
CACHE_FUNCTIONS(char*, struct dbc*, dbc);
CACHE_FUNCTIONS(char*, struct wow_mpq_file*, mpq);
