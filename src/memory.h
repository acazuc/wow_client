#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

enum memory_type
{
	MEM_GENERIC,
	MEM_FONT,
	MEM_UI,
	MEM_NET,
	MEM_GFX,
	MEM_DBC,
	MEM_GX,
	MEM_XML,
	MEM_LUA,
	MEM_OBJ,
	MEM_LIBWOW,
	MEM_PPE,
	MEM_SND,
	MEM_MAP,
	MEM_LAST
};

void mem_init(void);
void *mem_malloc(enum memory_type type, size_t size);
void *mem_zalloc(enum memory_type type, size_t size);
void *mem_realloc(enum memory_type type, void *ptr, size_t size);
char *mem_strdup(enum memory_type type, const char *str);
char *mem_strndup(enum memory_type type, const char *str, size_t size);
void mem_free(enum memory_type type, void *ptr);
void mem_dump(void);

#define MEMORY_DECL(mem) \
	void *mem_malloc_##mem(size_t size); \
	void *mem_zalloc_##mem(size_t size); \
	void *mem_realloc_##mem(void *ptr, size_t size); \
	char *mem_strdup_##mem(const char *str); \
	char *mem_strndup_##mem(const char *str, size_t size); \
	void mem_free_##mem(void *ptr); \
	extern const struct jks_array_memory_fn jks_array_memory_fn_##mem; \
	extern const struct jks_hmap_memory_fn jks_hmap_memory_fn_##mem; \
	extern const struct jks_list_memory_fn jks_list_memory_fn_##mem;

#endif
