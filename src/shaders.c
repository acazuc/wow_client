#include "shaders.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include "../shaders/compile.h"

#include <gfx/device.h>
#include <gfx/window.h>

#include <string.h>
#include <stdio.h>
#include <errno.h>

#define SHADERS_DIR "shaders"

static uint8_t *read_file(const char *path, size_t *len)
{
	uint8_t *buf = NULL;
	size_t buf_size = 0;
	FILE *file = NULL;
	size_t tmp;

	*len = 0;
	file = fopen(path, "rb");
	if (!file)
	{
		LOG_ERROR("failed to open shader file '%s'", path);
		return NULL;
	}
	do
	{
		if (buf_size + 4096 >= 1024 * 1024)
		{
			LOG_ERROR("shader file too big (> 1MB)");
			mem_free(MEM_GENERIC, buf);
			buf = NULL;
			goto cleanup;
		}
		if (*len == buf_size)
		{
			buf = mem_realloc(MEM_GENERIC, buf, buf_size + 4096);
			if (!buf)
			{
				LOG_ERROR("realloc failed");
				goto cleanup;
			}
			buf_size += 4096;
		}
		tmp = fread(buf + *len, 1, buf_size - *len, file);
		*len += tmp;
	} while (tmp > 0 && !feof(file));

	if (ferror(file))
	{
		LOG_ERROR("read error: %s", strerror(errno));
		mem_free(MEM_GENERIC, buf);
		buf = NULL;
		goto cleanup;
	}

	if (*len == buf_size)
	{
		buf = mem_realloc(MEM_GENERIC, buf, buf_size + 1);
		if (!buf)
		{
			LOG_ERROR("realloc failed");
			goto cleanup;
		}
	}

cleanup:
	fclose(file);
	return buf;
}

static bool load_shader(const char *name, enum gfx_shader_type type, uint8_t **data, size_t *size)
{
	char fn[1024];
	const char *type_str;

	switch (type)
	{
		case GFX_SHADER_VERTEX:
			type_str = "vs";
			break;
		case GFX_SHADER_FRAGMENT:
			type_str = "fs";
			break;
		default:
			LOG_ERROR("invalid shader type: %d", (int)type);
			return false;
	}
	if (snprintf(fn, sizeof(fn), "%s/%s.%s.gfx", SHADERS_DIR, name, type_str) == sizeof(fn))
	{
		LOG_ERROR("shader path is too long");
		return false;
	}
	*data = read_file(fn, size);
	if (!data)
		return false;
	return true;
}

static bool create_shader(gfx_shader_t *shader, enum gfx_shader_type type, uint8_t *data, size_t size)
{
	struct gfx_shader_def *def = (struct gfx_shader_def*)data;
	uint32_t code_length = def->codes_lengths[g_wow->window->properties.device_backend];
	uint32_t code_offset = def->codes_offsets[g_wow->window->properties.device_backend];
	if (!code_length)
	{
		LOG_ERROR("no shader code");
		return false;
	}
	if (code_offset >= size || code_offset + code_length >= size)
	{
		LOG_ERROR("invalid code size");
		return false;
	}
	if (!gfx_create_shader(g_wow->device, shader, type, &data[code_offset], code_length))
	{
		LOG_ERROR("failed to create shader");
		return false;
	}
	return true;
}

static bool load_shader_state(gfx_shader_state_t *shader_state, const char *name)
{
	const gfx_shader_t *shaders[3];
	uint32_t shaders_count = 0;
	gfx_shader_t fragment_shader = GFX_SHADER_INIT();
	gfx_shader_t vertex_shader = GFX_SHADER_INIT();
	uint8_t *vertex_data;
	uint8_t *fragment_data;
	size_t vertex_size;
	size_t fragment_size;
	bool ret = false;

	if (!load_shader(name, GFX_SHADER_VERTEX, &vertex_data, &vertex_size))
		goto cleanup;
	if (!load_shader(name, GFX_SHADER_FRAGMENT, &fragment_data, &fragment_size))
		goto cleanup;
	if (!create_shader(&vertex_shader, GFX_SHADER_VERTEX, vertex_data, vertex_size))
		goto cleanup;
	if (!create_shader(&fragment_shader, GFX_SHADER_FRAGMENT, fragment_data, fragment_size))
		goto cleanup;
	*shader_state = GFX_SHADER_STATE_INIT();
	shaders[shaders_count++] = &vertex_shader;
	shaders[shaders_count++] = &fragment_shader;
	struct gfx_shader_attribute attributes[16];
	struct gfx_shader_constant constants[16];
	struct gfx_shader_sampler samplers[16];
	memset(attributes, 0, sizeof(attributes));
	memset(constants, 0, sizeof(constants));
	memset(samplers, 0, sizeof(samplers));
	ret = gfx_create_shader_state(g_wow->device, shader_state, shaders, shaders_count, attributes, constants, samplers);
	if (!ret)
		LOG_ERROR("failed to create %s shader state", name);

cleanup:
	//gfx_delete_shader(g_wow->device, &fragment_shader);
	//gfx_delete_shader(g_wow->device, &vertex_shader);
	return ret;
}

bool shaders_build(struct shaders *shaders)
{
#define BUILD_SHADER(name) \
	do \
	{ \
		LOG_INFO("building shader " #name); \
		if (!load_shader_state(&shaders->name, #name)) \
			return false; \
	} while (0)

	BUILD_SHADER(wmo_collisions);
	BUILD_SHADER(ssao_denoiser);
	BUILD_SHADER(m2_collisions);
	BUILD_SHADER(wmo_portals);
	BUILD_SHADER(bloom_merge);
	BUILD_SHADER(bloom_blur);
	BUILD_SHADER(mclq_water);
	BUILD_SHADER(mclq_magma);
	BUILD_SHADER(chromaber);
	BUILD_SHADER(m2_lights);
	BUILD_SHADER(m2_bones);
	BUILD_SHADER(particle);
	BUILD_SHADER(sharpen);
	BUILD_SHADER(skybox);
	BUILD_SHADER(ribbon);
	BUILD_SHADER(basic);
	BUILD_SHADER(sobel);
	BUILD_SHADER(bloom);
	BUILD_SHADER(glow);
	BUILD_SHADER(mliq);
	BUILD_SHADER(ssao);
	BUILD_SHADER(fxaa);
	BUILD_SHADER(aabb);
	BUILD_SHADER(mcnk);
	BUILD_SHADER(text);
	BUILD_SHADER(fsaa);
	BUILD_SHADER(gui);
	BUILD_SHADER(wdl);
	BUILD_SHADER(wmo);
	BUILD_SHADER(cel);
	BUILD_SHADER(m2);
	BUILD_SHADER(ui);
	return true;

#undef BUILD_SHADER
}

void shaders_clean(struct shaders *shaders)
{
#define CLEAN_SHADER(name) \
	do \
	{ \
		LOG_INFO("cleaning shader " #name); \
		gfx_delete_shader_state(g_wow->device, &shaders->name); \
	} while (0)

	CLEAN_SHADER(wmo_collisions);
	CLEAN_SHADER(ssao_denoiser);
	CLEAN_SHADER(m2_collisions);
	CLEAN_SHADER(wmo_portals);
	CLEAN_SHADER(bloom_merge);
	CLEAN_SHADER(bloom_blur);
	CLEAN_SHADER(mclq_water);
	CLEAN_SHADER(mclq_magma);
	CLEAN_SHADER(chromaber);
	CLEAN_SHADER(m2_lights);
	CLEAN_SHADER(m2_bones);
	CLEAN_SHADER(particle);
	CLEAN_SHADER(sharpen);
	CLEAN_SHADER(skybox);
	CLEAN_SHADER(ribbon);
	CLEAN_SHADER(basic);
	CLEAN_SHADER(sobel);
	CLEAN_SHADER(bloom);
	CLEAN_SHADER(glow);
	CLEAN_SHADER(mliq);
	CLEAN_SHADER(ssao);
	CLEAN_SHADER(fxaa);
	CLEAN_SHADER(aabb);
	CLEAN_SHADER(mcnk);
	CLEAN_SHADER(text);
	CLEAN_SHADER(fsaa);
	CLEAN_SHADER(gui);
	CLEAN_SHADER(wdl);
	CLEAN_SHADER(wmo);
	CLEAN_SHADER(cel);
	CLEAN_SHADER(m2);
	CLEAN_SHADER(ui);

#undef CLEAN_SHADER
}
