#include "shaders.h"

#include "memory.h"
#include "log.h"
#include "wow.h"

#include <gfx/device.h>
#include <gfx/window.h>

#include <string.h>
#include <stdio.h>
#include <errno.h>

#define SHADERS_DIR "data/shaders"

static uint8_t *read_file(const char *path, uint32_t *len)
{
	uint8_t *buf = NULL;
	size_t buf_size = 0;
	FILE *file = NULL;
	size_t tmp;

	*len = 0;
	file = fopen(path, "rb");
	if (!file)
	{
		LOG_ERROR("failed to open file \"%s\"", path);
		return NULL;
	}
	do
	{
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
	} while (tmp > 0 && buf_size < 1024 * 64 && !feof(file));

	if (ferror(file) || !feof(file))
	{
		LOG_ERROR("read error: %s", strerror(errno));
		mem_free(MEM_GENERIC, buf);
		buf = NULL;
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

static int load_shader(gfx_shader_t *shader, const char *name, enum gfx_shader_type type)
{
	int ret = false;
	char fn[1024];
	uint8_t *data = NULL;
	uint32_t data_len;
	const char *type_str;
	const char *dir_str;
	const char *ext_str;

	switch (type)
	{
		case GFX_SHADER_VERTEX:
			type_str = "vs";
			break;
		case GFX_SHADER_FRAGMENT:
			type_str = "fs";
			break;
		case GFX_SHADER_GEOMETRY:
			type_str = "gs";
			break;
		default:
			LOG_ERROR("invalid shader type: %d", (int)type);
			return 0;
	}
	switch (g_wow->window->properties.device_backend)
	{
		case GFX_DEVICE_GL3:
			dir_str = "gl3";
			ext_str = "glsl";
			break;
		case GFX_DEVICE_GL4:
			dir_str = "gl4";
			ext_str = "glsl";
			break;
		case GFX_DEVICE_D3D9:
			dir_str = "d3d9";
			ext_str = "hlsl";
			break;
		case GFX_DEVICE_D3D11:
			dir_str = "d3d11";
			ext_str = "hlsl";
			break;
		case GFX_DEVICE_VK:
			dir_str = "vk";
			ext_str = "khr";
			break;
	}
	if (snprintf(fn, sizeof(fn), "%s/%s/%s.%s.%s", SHADERS_DIR, dir_str, name, type_str, ext_str) == sizeof(fn))
	{
		LOG_ERROR("shader path is too long");
		return 0;
	}
	data = read_file(fn, &data_len);
	if (!data)
	{
		ret = -1;
		goto cleanup;
	}
	if (!gfx_create_shader(g_wow->device, shader, type, data, data_len))
	{
		ret = 0;
		goto cleanup;
	}

	ret = 1;

cleanup:
	mem_free(MEM_GENERIC, data);
	return ret;
}

static bool load_shader_state(gfx_shader_state_t *shader_state, const char *name, const gfx_shader_attribute_t *attributes, const gfx_shader_constant_t *constants, const gfx_shader_sampler_t *samplers)
{
	const gfx_shader_t *shaders[3];
	uint32_t shaders_count = 0;
	gfx_shader_t geometry_shader = GFX_SHADER_INIT();
	gfx_shader_t fragment_shader = GFX_SHADER_INIT();
	gfx_shader_t vertex_shader = GFX_SHADER_INIT();
	bool geometry = false;
	bool ret = false;

	switch (load_shader(&vertex_shader, name, GFX_SHADER_VERTEX))
	{
		case -1:
		case 0:
			goto cleanup;
		case 1:
			break;
	}
	switch (load_shader(&fragment_shader, name, GFX_SHADER_FRAGMENT))
	{
		case -1:
		case 0:
			goto cleanup;
		case 1:
			break;
	}
	switch (load_shader(&geometry_shader, name, GFX_SHADER_GEOMETRY))
	{
		case -1:
			break;
		case 0:
			goto cleanup;
		case 1:
			geometry = true;
			break;
	}
	*shader_state = GFX_SHADER_STATE_INIT();
	shaders[shaders_count++] = &vertex_shader;
	shaders[shaders_count++] = &fragment_shader;
	if (geometry)
		shaders[shaders_count++] = &geometry_shader;
	ret = gfx_create_shader_state(g_wow->device, shader_state, shaders, shaders_count, attributes, constants, samplers);
	if (!ret)
		LOG_ERROR("failed to create %s shader state", name);

cleanup:
	gfx_delete_shader(g_wow->device, &geometry_shader);
	gfx_delete_shader(g_wow->device, &fragment_shader);
	gfx_delete_shader(g_wow->device, &vertex_shader);
	return ret;
}

static bool build_wmo_collisions(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{"mesh_block", 0},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{NULL, 0}
	};
	return load_shader_state(shader_state, "wmo_collisions", attributes, constants, samplers);
}

static bool build_ssao_denoiser(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_uv", 1},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"position_tex", 2},
		{"normal_tex", 1},
		{"color_tex", 0},
		{"ssao_tex", 3},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "ssao_denoiser", attributes, constants, samplers);
}

static bool build_m2_collisions(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{"mesh_block", 0},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{NULL, 0}
	};
	return load_shader_state(shader_state, "m2_collisions", attributes, constants, samplers);
}

static bool build_wmo_portals(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_color", 1},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{"mesh_block", 0},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{NULL, 0}
	};
	return load_shader_state(shader_state, "wmo_portals", attributes, constants, samplers);
}

static bool build_bloom_merge(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_uv", 1},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"position_tex", 2},
		{"normal_tex", 1},
		{"color_tex", 0},
		{"bloom_tex", 3},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "bloom_merge", attributes, constants, samplers);
}

static bool build_bloom_blur(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_uv", 1},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"color_tex", 0},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "bloom_blur", attributes, constants, samplers);
}

static bool build_mclq_water(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_depth", 1},
		{"vs_uv", 2},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"scene_block", 2},
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"position_tex", 3},
		{"normal_tex", 2},
		{"color_tex", 1},
		{"tex", 0},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "mclq_water", attributes, constants, samplers);
}

static bool build_mclq_magma(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_uv", 1},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"scene_block", 2},
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"tex", 0},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "mclq_magma", attributes, constants, samplers);
}

static bool build_chromaber(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_uv", 1},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"color_tex", 0},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "chromaber", attributes, constants, samplers);
}

static bool build_m2_lights(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_color", 1},
		{"vs_bone", 2},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{NULL, 0}
	};
	return load_shader_state(shader_state, "m2_lights", attributes, constants, samplers);
}

static bool build_m2_bones(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_color", 1},
		{"vs_bone", 2},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{NULL, 0}
	};
	return load_shader_state(shader_state, "m2_bones", attributes, constants, samplers);
}

static bool build_particle(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_color", 1},
		{"vs_uv", 2},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"scene_block", 2},
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"tex", 0},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "particle", attributes, constants, samplers);
}

static bool build_sharpen(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_uv", 1},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"color_tex", 0},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "sharpen", attributes, constants, samplers);
}

static bool build_skybox(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_color0", 1},
		{"vs_color1", 2},
		{"vs_color2", 3},
		{"vs_color3", 4},
		{"vs_color4", 5},
		{"vs_uv", 6},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"clouds1", 0},
		{"clouds2", 1},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "skybox", attributes, constants, samplers);
}

static bool build_ribbon(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_color", 1},
		{"vs_uv", 2},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"scene_block", 2},
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"tex", 0},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "ribbon", attributes, constants, samplers);
}

static bool build_basic(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_color", 1},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{NULL, 0}
	};
	return load_shader_state(shader_state, "basic", attributes, constants, samplers);
}

static bool build_sobel(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_uv", 1},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"position_tex", 2},
		{"normal_tex", 1},
		{"color_tex", 0},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "sobel", attributes, constants, samplers);
}

static bool build_bloom(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_uv", 1},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"color_tex", 0},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "bloom", attributes, constants, samplers);
}

static bool build_glow(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_uv", 1},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"position_tex", 2},
		{"normal_tex", 1},
		{"color_tex", 0},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "glow", attributes, constants, samplers);
}

static bool build_mliq(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_uv", 1},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"scene_block", 2},
		{"model_block", 1},
		{"mesh_block", 0},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"tex", 0},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "mliq", attributes, constants, samplers);
}

static bool build_ssao(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_uv", 1},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"tex_position", 0},
		{"tex_normal", 1},
		{"tex_noise", 2},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "ssao", attributes, constants, samplers);
}

static bool build_fxaa(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_uv", 1},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"position_tex", 2},
		{"normal_tex", 1},
		{"color_tex", 0},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "fxaa", attributes, constants, samplers);
}

static bool build_fsaa(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_uv", 1},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"position_tex", 2},
		{"normal_tex", 1},
		{"color_tex", 0},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "fsaa", attributes, constants, samplers);
}

static bool build_aabb(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"mesh_block", 0},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{NULL, 0}
	};
	return load_shader_state(shader_state, "aabb", attributes, constants, samplers);
}

static bool build_mcnk(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_normal", 0},
		{"vs_xz", 1},
		{"vs_uv", 2},
		{"vs_y", 3},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"scene_block", 2},
		{"model_block", 1},
		{"mesh_block", 0},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"textures[0]", 1},
		{"textures[1]", 2},
		{"textures[2]", 3},
		{"textures[3]", 4},
		{"textures[4]", 5},
		{"textures[5]", 6},
		{"textures[6]", 7},
		{"textures[7]", 8},
		{"alpha_map", 0},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "mcnk", attributes, constants, samplers);
}

static bool build_text(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_uv", 1},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"tex", 0},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "text", attributes, constants, samplers);
}

static bool build_gui(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_color", 1},
		{"vs_uv", 2},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"tex", 0},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "gui", attributes, constants, samplers);
}

static bool build_wdl(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{NULL, 0}
	};
	return load_shader_state(shader_state, "wdl", attributes, constants, samplers);
}

static bool build_wmo(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_normal", 1},
		{"vs_uv", 2},
		{"vs_color", 3},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"scene_block", 2},
		{"model_block", 1},
		{"mesh_block", 0},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"tex1", 0},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "wmo", attributes, constants, samplers);
}

static bool build_cel(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_uv", 1},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"position_tex", 2},
		{"normal_tex", 1},
		{"color_tex", 0},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "cel", attributes, constants, samplers);
}

static bool build_m2(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_bone_weights", 0},
		{"vs_position", 1},
		{"vs_normals", 2},
		{"vs_bones", 3},
		{"vs_uv1", 4},
		{"vs_uv2", 5},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"scene_block", 2},
		{"model_block", 1},
		{"mesh_block", 0},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"tex1", 0},
		{"tex2", 1},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "m2", attributes, constants, samplers);
}

static bool build_ui(gfx_shader_state_t *shader_state)
{
	static const gfx_shader_attribute_t attributes[] =
	{
		{"vs_position", 0},
		{"vs_color", 1},
		{"vs_uv", 2},
		{NULL, 0}
	};
	static const gfx_shader_constant_t constants[] =
	{
		{"model_block", 1},
		{NULL, 0}
	};
	static const gfx_shader_sampler_t samplers[] =
	{
		{"tex", 0},
		{"mask", 1},
		{NULL, 0}
	};
	return load_shader_state(shader_state, "ui", attributes, constants, samplers);
}

bool shaders_build(struct shaders *shaders)
{
#define BUILD_SHADER(name) \
	do \
	{ \
		LOG_INFO("building shader " #name); \
		if (!build_##name(&shaders->name)) \
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
