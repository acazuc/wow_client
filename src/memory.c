#include "memory.h"

#include "log.h"

#include <jks/array.h>
#include <jks/hmap.h>
#include <jks/list.h>

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#ifdef WITH_MEMORY

static const char *g_memory_str[MEM_LAST] =
{
	"MEM_GENERIC",
	"MEM_FONT",
	"MEM_UI",
	"MEM_NET",
	"MEM_OPENSSL",
	"MEM_GFX",
	"MEM_DBC",
	"MEM_GX",
	"MEM_XML",
	"MEM_LUA",
	"MEM_OBJ",
	"MEM_LIBWOW",
	"MEM_PPE",
	"MEM_SND",
};

static struct
{
	pthread_mutex_t mutex;
	size_t size;
	size_t count;
} g_memory[MEM_LAST] = {0};

struct mem_header
{
	size_t size;
	enum memory_type type;
};

#endif

void mem_init(void)
{
#ifdef WITH_MEMORY
	for (size_t i = 0; i < MEM_LAST; ++i)
		pthread_mutex_init(&g_memory[i].mutex, NULL);
#endif
}

void *mem_malloc(enum memory_type type, size_t size)
{
#ifdef WITH_MEMORY
	struct mem_header *ret = malloc(size + sizeof(struct mem_header));
#else
	void *ret = malloc(size);
#endif
	if (!ret)
	{
		LOG_ERROR("allocation of %zu bytes for type %d failed", size, type);
		return NULL;
	}
#ifdef WITH_MEMORY
	assert(type < MEM_LAST);
	ret->size = size;
	ret->type = type;
	pthread_mutex_lock(&g_memory[type].mutex);
	g_memory[type].size += size;
	g_memory[type].count++;
	pthread_mutex_unlock(&g_memory[type].mutex);
	ret = (struct mem_header*)((char*)ret + sizeof(struct mem_header));
#endif
	return ret;
}

void *mem_zalloc(enum memory_type type, size_t size)
{
	void *ret = mem_malloc(type, size);
	if (!ret)
		return NULL;
	memset(ret, 0, size);
	return ret;
}

void *mem_realloc(enum memory_type type, void *ptr, size_t size)
{
	if (!ptr)
		return mem_malloc(type, size);
	if (!size)
	{
		mem_free(type, ptr);
		return NULL;
	}
#ifdef WITH_MEMORY
	struct mem_header *ret;
	struct mem_header *header = (struct mem_header*)((char*)ptr - sizeof(*header));
	if (header->type != type)
		LOG_WARN("deallocation of invalid type: %d instead of %d", type, header->type);
	ret = realloc(header, size + sizeof(struct mem_header));
#else
	void *ret = realloc(ptr, size);
#endif
	if (!ret)
	{
		LOG_ERROR("reallocation of %zu bytes failed", size);
		return NULL;
	}
#ifdef WITH_MEMORY
	pthread_mutex_lock(&g_memory[ret->type].mutex);
	g_memory[ret->type].size -= ret->size;
	g_memory[ret->type].count--;
	pthread_mutex_unlock(&g_memory[ret->type].mutex);
	pthread_mutex_lock(&g_memory[type].mutex);
	g_memory[type].size += size;
	g_memory[type].count++;
	pthread_mutex_unlock(&g_memory[type].mutex);
	ret->size = size;
	ret->type = type;
	ret = (struct mem_header*)((char*)ret + sizeof(struct mem_header));
#endif
	return ret;
}

char *mem_strdup(enum memory_type type, const char *str)
{
	size_t len = strlen(str);
	char *ret = mem_malloc(type, len + 1);
	if (!ret)
		return NULL;
	memcpy(ret, str, len);
	ret[len] = '\0';
	return ret;
}

char *mem_strndup(enum memory_type type, const char *str, size_t size)
{
	char *ret = mem_malloc(type, size + 1);
	if (!ret)
		return NULL;
	memcpy(ret, str, size);
	ret[size] = '\0';
	return ret;
}

void mem_free(enum memory_type type, void *ptr)
{
#ifdef WITH_MEMORY
	if (!ptr)
		return;
	struct mem_header *header = (struct mem_header*)((char*)ptr - sizeof(*header));
	if (header->type != type)
		LOG_WARN("deallocation of invalid type: %d instead of %d", type, header->type);
	pthread_mutex_lock(&g_memory[header->type].mutex);
	g_memory[header->type].size -= header->size;
	g_memory[header->type].count--;
	pthread_mutex_unlock(&g_memory[header->type].mutex);
	free(header);
#else
	(void)type;
	free(ptr);
#endif
}

void mem_dump(void)
{
#ifdef WITH_MEMORY
	LOG_INFO("%20s | %10s | %10s | %7s", "category", "memory", "bytes", "count");
	LOG_INFO("---------------------+------------+------------+--------");
	for (size_t i = 0; i < sizeof(g_memory) / sizeof(*g_memory); ++i)
	{
		char str[256];
		if (g_memory[i].size > 1000000000)
			snprintf(str, sizeof(str), "%.2f GB", g_memory[i].size / 1000000000.f);
		else if (g_memory[i].size > 1000000)
			snprintf(str, sizeof(str), "%.2f MB", g_memory[i].size / 1000000.f);
		else if (g_memory[i].size > 1000)
			snprintf(str, sizeof(str), "%.2f KB", g_memory[i].size / 1000.f);
		else
			snprintf(str, sizeof(str), "%u  B", (unsigned)g_memory[i].size);
		LOG_INFO("%20s | %10s | %10zu | %7zu", g_memory_str[i], str, g_memory[i].size, g_memory[i].count);
	}
#endif
}

#define MEMORY_DEFINE(mem) \
	void *mem_malloc_##mem(size_t size) \
	{ \
		return mem_malloc(MEM_##mem, size); \
	} \
	void *mem_zalloc_##mem(size_t size) \
	{ \
		return mem_zalloc(MEM_##mem, size); \
	} \
	void *mem_realloc_##mem(void *ptr, size_t size) \
	{ \
		return mem_realloc(MEM_##mem, ptr, size); \
	} \
	char *mem_strdup_##mem(const char *str) \
	{ \
		return mem_strdup(MEM_##mem, str); \
	} \
	char *mem_strndup_##mem(const char *str, size_t size) \
	{ \
		return mem_strndup(MEM_##mem, str, size); \
	} \
	void mem_free_##mem(void *ptr) \
	{ \
		mem_free(MEM_##mem, ptr); \
	} \
	const struct jks_array_memory_fn jks_array_memory_fn_##mem = \
	{ \
		.malloc = mem_malloc_##mem, \
		.realloc = mem_realloc_##mem, \
		.free = mem_free_##mem, \
	}; \
	const struct jks_hmap_memory_fn jks_hmap_memory_fn_##mem = \
	{ \
		.malloc = mem_malloc_##mem, \
		.realloc = mem_realloc_##mem, \
		.free = mem_free_##mem, \
	}; \
	const struct jks_list_memory_fn jks_list_memory_fn_##mem = \
	{ \
		.malloc = mem_malloc_##mem, \
		.realloc = mem_realloc_##mem, \
		.free = mem_free_##mem, \
	};

MEMORY_DEFINE(GENERIC);
MEMORY_DEFINE(FONT);
MEMORY_DEFINE(UI);
MEMORY_DEFINE(NET);
MEMORY_DEFINE(OPENSSL);
MEMORY_DEFINE(GFX);
MEMORY_DEFINE(DBC);
MEMORY_DEFINE(GX);
MEMORY_DEFINE(XML);
MEMORY_DEFINE(LUA);
MEMORY_DEFINE(OBJ);
MEMORY_DEFINE(LIBWOW);
MEMORY_DEFINE(PPE);
MEMORY_DEFINE(SND);
