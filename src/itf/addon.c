#include "addon.h"

#include "itf/interface.h"

#include "wow_lua.h"
#include "memory.h"
#include "cache.h"
#include "log.h"
#include "wow.h"

#include <libwow/toc.h>
#include <libwow/mpq.h>

#include <jks/array.h>

#include <string.h>
#include <stdio.h>

#ifdef interface
# undef interface
#endif

MEMORY_DECL(UI);

static bool parse_dependencies(struct addon *addon);

static void get_filepath(struct addon *addon, char *filepath, size_t filepath_size, const char *filename, const char *origin)
{
	const char *org_dir = strrchr(origin, '/');
	int org_len;
	if (org_dir)
		org_len = org_dir - origin;
	else
		org_len = 0;
	snprintf(filepath, filepath_size, "%s%sInterface%s/%s/%.*s%s%s",
	         addon->source == ADDON_EXT ? g_wow->game_path : "",
	         addon->source == ADDON_EXT ? "/" : "",
	         addon->source != ADDON_SRC ? "/AddOns" : "",
	         addon->name, org_len, origin, org_len ? "/" : "", filename);
	for (size_t i = 0; filepath[i]; ++i)
	{
		if (filepath[i] == '\\')
			filepath[i] = '/';
	}
}

static struct addon *addon_new(struct interface *interface, const char *name, const struct addon_vtable *vtable, enum addon_source source)
{
	struct addon *addon = mem_malloc(MEM_UI, sizeof(*addon));
	if (!addon)
	{
		LOG_ERROR("allocation failure");
		return NULL;
	}
	addon->name = mem_strdup(MEM_UI, name);
	if (!addon->name)
	{
		LOG_ERROR("allocation failure");
		mem_free(MEM_UI, addon);
		return NULL;
	}
	addon->source = source;
	addon->interface = interface;
	addon->vtable = vtable;
	addon->toc = addon->vtable->get_toc(addon);
	if (!addon->toc)
	{
		LOG_ERROR("failed to load toc file");
		mem_free(MEM_UI, addon->name);
		mem_free(MEM_UI, addon);
		return NULL;
	}
	jks_array_init(&addon->dependencies, sizeof(char*), mem_free_UI, &jks_array_memory_fn_UI);
	if (addon->toc->deps)
	{
		if (!parse_dependencies(addon))
		{
			jks_array_destroy(&addon->dependencies);
			mem_free(MEM_UI, addon->name);
			mem_free(MEM_UI, addon);
			return NULL;
		}
	}
	addon->enabled = true;
	addon->loaded = false;
	return addon;
}

void addon_delete(struct addon *addon)
{
	if (!addon)
		return;
	wow_toc_file_delete(addon->toc);
	jks_array_destroy(&addon->dependencies);
	mem_free(MEM_UI, addon->name);
	mem_free(MEM_UI, addon);
}

static bool parse_dependencies(struct addon *addon)
{
	char *prv = addon->toc->deps;
	char *nxt;
	while ((nxt = strstr(prv, ", ")))
	{
		char *dep = mem_strndup(MEM_UI, prv, nxt - prv);
		if (!dep)
		{
			LOG_ERROR("failed to strdup addon dependency");
			return false;
		}
		if (!jks_array_push_back(&addon->dependencies, &dep))
		{
			LOG_ERROR("failed to add addon dependency");
			mem_free(MEM_UI, dep);
			return false;
		}
		prv = nxt + 2;
	}
	if (prv[0])
	{
		char *dep = mem_strdup(MEM_UI, prv);
		if (!dep)
		{
			LOG_ERROR("failed to strdup addon dependency");
			return false;
		}
		if (!jks_array_push_back(&addon->dependencies, &dep))
		{
			LOG_ERROR("failed to add addon dependency");
			mem_free(MEM_UI, dep);
			return false;
		}
	}
	return true;
}

bool addon_load(struct addon *addon)
{
	if (addon->loaded)
		return true;
	LOG_INFO("loading addon %s", addon->name);
	bool ret = true;
	for (size_t i = 0; i < addon->toc->files_nb; ++i)
	{
		char filename[256];
		const char *file = addon->toc->files[i];
		size_t file_len = strlen(file);
		for (size_t n = 0; n < file_len; ++n)
		{
			if (file[n] == '\\')
				filename[n] = '/';
			else
				filename[n] = file[n];
		}
		filename[file_len] = '\0';
		if (file_len < 4)
		{
			LOG_ERROR("onvalid file: %s", file);
			continue;
		}
		if (!strncmp(file + file_len - 4, ".xml", 4))
		{
			if (!addon->vtable->load_xml(addon, filename, ""))
			{
				LOG_ERROR("failed to load xml file");
				ret = false;
			}
		}
		else if (!strncmp(file + file_len - 4, ".lua", 4))
		{
			if (!addon->vtable->load_lua(addon, filename, ""))
			{
				LOG_ERROR("failed to load lua file");
				ret = false;
			}
		}
		else
		{
			LOG_ERROR("invalid file: %s", file);
		}
	}
	lua_pushnil(addon->interface->L);
	lua_pushstring(addon->interface->L, addon->name);
	interface_execute_event(addon->interface, EVENT_ADDON_LOADED, 1);
	LOG_INFO("loaded addon %s", addon->name);
	addon->loaded = true;
	return ret;
}

enum addon_state addon_get_state(struct addon *addon)
{
	if (addon->toc->version != 20400)
		return ADDON_INTERFACE_VERSION;
	if (!addon->enabled)
		return ADDON_DISABLED;
	for (size_t i = 0; i < addon->dependencies.size; ++i)
	{
		const char *dependency = *JKS_ARRAY_GET(&addon->dependencies, i, const char*);
		struct addon *dep = interface_get_addon(addon->interface, dependency);
		if (!dep)
			return ADDON_DEP_MISSING;
		enum addon_state dep_state = addon_get_state(dep);
		switch (dep_state)
		{
			case ADDON_ENABLED:
				break;
			case ADDON_BANNED:
			case ADDON_DEP_BANNED:
				return ADDON_DEP_BANNED;
			case ADDON_CORRUPT:
			case ADDON_DEP_CORRUPT:
				return ADDON_DEP_CORRUPT;
			case ADDON_DISABLED:
			case ADDON_DEP_DISABLED:
				return ADDON_DEP_DISABLED;
			case ADDON_INCOMPATIBLE:
			case ADDON_DEP_INCOMPATIBLE:
				return ADDON_DEP_INCOMPATIBLE;
			case ADDON_INSECURE:
			case ADDON_DEP_INSECURE:
				return ADDON_DEP_INSECURE;
			case ADDON_INTERFACE_VERSION:
			case ADDON_DEP_INTERFACE_VERSION:
				return ADDON_DEP_INTERFACE_VERSION;
			case ADDON_DEP_MISSING:
				return ADDON_DEP_MISSING;
		}
	}
	return ADDON_ENABLED;
}

static bool ext_load_file(const char *filepath, struct jks_array *data, size_t limit)
{
	ssize_t readed = 0;
	bool ret = false;
	FILE *file = fopen(filepath, "rb");
	if (!file)
	{
		LOG_ERROR("file not found: %s", filepath);
		return false;
	}
	while (data->size < limit)
	{
		if (!jks_array_reserve(data, data->size + 4096))
		{
			LOG_ERROR("failed to reserve file data");
			goto cleanup;
		}
		readed = fread((char*)data->data + data->size, 1, 4096, file);
		if (!readed)
		{
			if (feof(file))
				break;
			LOG_ERROR("failed to read from file %s", filepath);
			goto cleanup;
		}
		if (!jks_array_resize(data, data->size + readed))
		{
			LOG_ERROR("failed to resize file data");
			goto cleanup;
		}
	}
	if (data->size >= limit)
	{
		LOG_ERROR("ext file too long: %s (%u / %u)", filepath, (unsigned)data->size, (unsigned)limit);
		goto cleanup;
	}
	if (!feof(file))
	{
		LOG_ERROR("error while reading %s", filepath);
		goto cleanup;
	}
	ret = true;

cleanup:
	fclose(file);
	return ret;
}

static wow_toc_file_t *ext_get_toc(struct addon *addon)
{
	char filepath[512];
	snprintf(filepath, sizeof(filepath), "%s/Interface/AddOns/%s/%s.toc", g_wow->game_path, addon->name, addon->name);
	struct jks_array data;
	jks_array_init(&data, sizeof(char), NULL, &jks_array_memory_fn_UI);
	if (!ext_load_file(filepath, &data, 100000000))
	{
		jks_array_destroy(&data);
		return NULL;
	}
	wow_toc_file_t *toc = wow_toc_file_new((const uint8_t*)data.data, data.size);
	jks_array_destroy(&data);
	return toc;
}

static bool ext_load_xml(struct addon *addon, const char *filename, const char *origin)
{
	char filepath[512];
	get_filepath(addon, filepath, sizeof(filepath), filename, origin);
	struct jks_array data;
	jks_array_init(&data, sizeof(char), NULL, &jks_array_memory_fn_UI);
	if (!ext_load_file(filepath, &data, 100000000))
	{
		jks_array_destroy(&data);
		return false;
	}
	LOG_INFO("loading xml file: %s", filename);
	bool ret = interface_load_xml(addon->interface, addon, filename, (const char*)data.data, data.size);
	LOG_INFO("loaded xml file: %s", filename);
	jks_array_destroy(&data);
	return ret;
}

static bool ext_load_lua(struct addon *addon, const char *filename, const char *origin)
{
	char filepath[512];
	get_filepath(addon, filepath, sizeof(filepath), filename, origin);
	struct jks_array data;
	jks_array_init(&data, sizeof(char), NULL, &jks_array_memory_fn_UI);
	if (!ext_load_file(filepath, &data, 100000000))
	{
		jks_array_destroy(&data);
		return false;
	}
	LOG_INFO("loading lua file: %s", filename);
	bool ret = interface_load_lua(addon->interface, (const char*)data.data, data.size, filepath);
	LOG_INFO("loaded lua file: %s", filename);
	jks_array_destroy(&data);
	return ret;
}

static const struct addon_vtable ext_vtable =
{
	.get_toc  = ext_get_toc,
	.load_xml = ext_load_xml,
	.load_lua = ext_load_lua,
};

struct addon *ext_addon_new(struct interface *interface, const char *filename)
{
	return addon_new(interface, filename, &ext_vtable, ADDON_EXT);
}

static wow_toc_file_t *int_get_toc(struct addon *addon)
{
	char filepath[512];
	snprintf(filepath, sizeof(filepath), "Interface/AddOns/%s/%s.toc", addon->name, addon->name);
	normalize_mpq_filename(filepath, sizeof(filepath));
	wow_mpq_file_t *file;
	if (!cache_ref_by_key_mpq(g_wow->cache, filepath, &file))
	{
		LOG_ERROR("failed to get toc file: %s", filepath);
		return NULL;
	}
	wow_toc_file_t *toc = wow_toc_file_new((const uint8_t*)file->data, file->size);
	cache_unref_by_ref_mpq(g_wow->cache, file);
	return toc;
}

static bool int_load_xml(struct addon *addon, const char *filename, const char *origin)
{
	char filepath[512];
	get_filepath(addon, filepath, sizeof(filepath), filename, origin);
	normalize_mpq_filename(filepath, sizeof(filepath));
	wow_mpq_file_t *mpq;
	if (!cache_ref_by_key_mpq(g_wow->cache, filepath, &mpq))
	{
		LOG_ERROR("failed to get xml file: %s", filepath);
		return false;
	}
	LOG_INFO("loading xml file: %s", filename);
	bool ret = interface_load_xml(addon->interface, addon, filename, (const char*)mpq->data, mpq->size);
	LOG_INFO("loaded xml file: %s", filename);
	cache_unref_by_ref_mpq(g_wow->cache, mpq);
	return ret;
}

static bool int_load_lua(struct addon *addon, const char *filename, const char *origin)
{
	char filepath[512];
	get_filepath(addon, filepath, sizeof(filepath), filename, origin);
	normalize_mpq_filename(filepath, sizeof(filepath));
	wow_mpq_file_t *mpq;
	if (!cache_ref_by_key_mpq(g_wow->cache, filepath, &mpq))
	{
		LOG_ERROR("failed to get lua file: %s", filepath);
		return false;
	}
	LOG_INFO("loading lua file: %s", filename);
	bool ret = interface_load_lua(addon->interface, (const char*)mpq->data, mpq->size, filepath);
	LOG_INFO("loaded lua file: %s", filename);
	cache_unref_by_ref_mpq(g_wow->cache, mpq);
	return ret;
}

static const struct addon_vtable int_vtable =
{
	.get_toc  = int_get_toc,
	.load_xml = int_load_xml,
	.load_lua = int_load_lua,
};

struct addon *int_addon_new(struct interface *interface, const char *filename)
{
	return addon_new(interface, filename, &int_vtable, ADDON_INT);
}

static wow_toc_file_t *src_get_toc(struct addon *addon)
{
	char filepath[512];
	snprintf(filepath, sizeof(filepath), "Interface/%s/%s.toc", addon->name, addon->name);
	normalize_mpq_filename(filepath, sizeof(filepath));
	wow_mpq_file_t *file;
	if (!cache_ref_by_key_mpq(g_wow->cache, filepath, &file))
	{
		LOG_ERROR("failed to get toc file: %s", filepath);
		return NULL;
	}
	wow_toc_file_t *toc = wow_toc_file_new((const uint8_t*)file->data, file->size);
	cache_unref_by_ref_mpq(g_wow->cache, file);
	return toc;
}

static bool src_load_xml(struct addon *addon, const char *filename, const char *origin)
{
	char filepath[512];
	get_filepath(addon, filepath, sizeof(filepath), filename, origin);
	normalize_mpq_filename(filepath, sizeof(filepath));
	wow_mpq_file_t *mpq;
	if (!cache_ref_by_key_mpq(g_wow->cache, filepath, &mpq))
	{
		LOG_ERROR("failed to get xml file: %s", filepath);
		return false;
	}
	LOG_INFO("loading xml file: %s", filename);
	bool ret = interface_load_xml(addon->interface, addon, filename, (const char*)mpq->data, mpq->size);
	LOG_INFO("loaded xml file: %s", filename);
	cache_unref_by_ref_mpq(g_wow->cache, mpq);
	return ret;
}

static bool src_load_lua(struct addon *addon, const char *filename, const char *origin)
{
	char filepath[512];
	get_filepath(addon, filepath, sizeof(filepath), filename, origin);
	normalize_mpq_filename(filepath, sizeof(filepath));
	wow_mpq_file_t *mpq;
	if (!cache_ref_by_key_mpq(g_wow->cache, filepath, &mpq))
	{
		LOG_ERROR("failed to get lua file: %s", filepath);
		return false;
	}
	LOG_INFO("loading lua file: %s", filename);
	bool ret = interface_load_lua(addon->interface, (const char*)mpq->data, mpq->size, filepath);
	LOG_INFO("loaded lua file: %s", filename);
	cache_unref_by_ref_mpq(g_wow->cache, mpq);
	return ret;
}

static const struct addon_vtable src_vtable =
{
	.get_toc  = src_get_toc,
	.load_xml = src_load_xml,
	.load_lua = src_load_lua,
};

struct addon *src_addon_new(struct interface *interface, const char *filename)
{
	return addon_new(interface, filename, &src_vtable, ADDON_SRC);
}
