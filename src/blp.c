#include "blp.h"

#include "loader.h"
#include "memory.h"
#include "cache.h"
#include "log.h"
#include "wow.h"
#include "bc.h"

#include <gfx/device.h>

#include <libwow/blp.h>

#include <inttypes.h>
#include <string.h>

struct blp_texture_mipmap
{
	uint8_t *data;
	uint32_t data_len;
	uint32_t width;
	uint32_t height;
};

struct blp_texture *blp_texture_new(const char *filename)
{
	struct blp_texture *texture = mem_malloc(MEM_GX, sizeof(*texture));
	if (!texture)
		return NULL;
	texture->mipmaps = NULL;
	texture->mipmaps_nb = 0;
	texture->load_asked = false;
	texture->loading = false;
	texture->loaded = false;
	texture->initialized = false;
	texture->texture = GFX_TEXTURE_INIT();
	texture->filename = mem_strdup(MEM_GX, filename);
	if (!texture->filename)
		goto err1;
	return texture;

err1:
	mem_free(MEM_GX, texture);
	return NULL;
}

struct blp_texture *blp_texture_from_data(const uint8_t *data, uint32_t width, uint32_t height)
{
	struct blp_texture *texture = mem_malloc(MEM_GX, sizeof(*texture));
	if (!texture)
		return NULL;
	texture->mipmaps = NULL;
	texture->format = GFX_B8G8R8A8;
	texture->mipmaps_nb = 1;
	texture->load_asked = false;
	texture->loading = false;
	texture->loaded = true;
	texture->initialized = true;
	texture->filename = NULL;
	texture->texture = GFX_TEXTURE_INIT();
	gfx_create_texture(g_wow->device, &texture->texture, GFX_TEXTURE_2D, texture->format, 1, width, height, 0);
	gfx_set_texture_levels(&texture->texture, 0, 0);
	gfx_set_texture_data(&texture->texture, 0, 0, width, height, 0, width * height * 4, data);
	gfx_set_texture_anisotropy(&texture->texture, g_wow->anisotropy);
	gfx_set_texture_filtering(&texture->texture, GFX_FILTERING_LINEAR, GFX_FILTERING_LINEAR, GFX_FILTERING_LINEAR);
	gfx_set_texture_addressing(&texture->texture, GFX_TEXTURE_ADDRESSING_REPEAT, GFX_TEXTURE_ADDRESSING_REPEAT, GFX_TEXTURE_ADDRESSING_REPEAT);
	texture->width = width;
	texture->height = height;
	return texture;
}

static void free_mipmaps(struct blp_texture_mipmap *mipmaps, uint32_t nb)
{
	if (!mipmaps)
		return;
	for (uint32_t i = 0; i < nb; ++i)
		mem_free(MEM_GX, mipmaps[i].data);
	mem_free(MEM_GX, mipmaps);
}

void blp_texture_delete(struct blp_texture *texture)
{
	if (!texture)
		return;
	free_mipmaps(texture->mipmaps, texture->mipmaps_nb);
	gfx_delete_texture(g_wow->device, &texture->texture);
	mem_free(MEM_GX, texture->filename);
	mem_free(MEM_GX, texture);
}

void blp_texture_ask_load(struct blp_texture *texture)
{
	if (texture->load_asked)
		return;
	texture->load_asked = true;
	cache_ref_by_ref_blp(g_wow->cache, texture);
	loader_push(g_wow->loader, ASYNC_TASK_BLP_LOAD, texture);
}

void blp_texture_ask_unload(struct blp_texture *texture)
{
	loader_gc_blp(g_wow->loader, texture);
}

static bool load_dummy_mipmap(struct blp_texture *texture)
{
	texture->mipmaps_nb = 1;
	texture->mipmaps = mem_malloc(MEM_GX, sizeof(*texture->mipmaps) * texture->mipmaps_nb);
	if (!texture->mipmaps)
		return false;
	texture->mipmaps[0].width = 1;
	texture->mipmaps[0].height = 1;
	texture->mipmaps[0].data_len = 4;
	texture->mipmaps[0].data = mem_malloc(MEM_GX, texture->mipmaps[0].data_len);
	if (!texture->mipmaps[0].data)
		return false;
	static const uint8_t tmp[4] = {0, 0xff, 0, 0xff};
	memcpy(texture->mipmaps[0].data, tmp, 4);
	return true;
}

bool blp_texture_load(struct blp_texture *texture, struct wow_blp_file *file)
{
	if (texture->loaded)
		return true;
	if (file->header.type != 1)
	{
		LOG_WARN("unsupported BLP type: %u", file->header.type);
		if (!load_dummy_mipmap(texture))
			return false;
		goto end;
	}
	texture->mipmaps_nb = file->mipmaps_nb;
	texture->mipmaps = mem_zalloc(MEM_GX, sizeof(*texture->mipmaps) * texture->mipmaps_nb);
	if (!texture->mipmaps)
		return false;
	texture->width = file->header.width;
	texture->height = file->header.height;
	switch (file->header.compression)
	{
		case 1:
		{
			texture->format = GFX_B8G8R8A8;
			for (uint32_t i = 0; i < texture->mipmaps_nb; ++i)
			{
				struct blp_texture_mipmap *mipmap = &texture->mipmaps[i];
				mipmap->width = file->mipmaps[i].width;
				mipmap->height = file->mipmaps[i].height;
				mipmap->data_len = mipmap->width * mipmap->height * 4;
				mipmap->data = mem_malloc(MEM_GX, mipmap->data_len);
				if (!mipmap->data)
					return false;
				const uint8_t *indexes = file->mipmaps[i].data;
				const uint8_t *alphas = indexes + mipmap->width * mipmap->height;
				uint32_t idx = 0;
				uint32_t n = mipmap->width * mipmap->height;
				for (uint32_t j = 0; j < n; ++j)
				{
					uint32_t p = file->header.palette[indexes[j]];
					mipmap->data[idx++] = p >> 0;
					mipmap->data[idx++] = p >> 8;
					mipmap->data[idx++] = p >> 16;
					switch (file->header.alpha_depth)
					{
						case 0:
							mipmap->data[idx++] = 0xff;
							break;
						case 1:
							mipmap->data[idx++] = ((alphas[j / 8] >> (i % 8)) & 1) * 0xff;
							break;
						case 4:
						{
							uint8_t a = ((alphas[j / 2] >> ((i % 2) * 4)) & 0xf);
							mipmap->data[idx++] = a | (a << 4);
							break;
						}
						case 8:
							mipmap->data[idx++] = alphas[j];
							break;
						default:
							mipmap->data[idx++] = 0xff;
							LOG_WARN("unsupported BLP alpha depth: %u", file->header.alpha_depth);
							break;
					}
				}
			}
			break;
		}
		case 2:
		{
			switch (file->header.alpha_type)
			{
				case 0:
					texture->format = (file->header.alpha_depth > 0) ? GFX_BC1_RGBA : GFX_BC1_RGB;
					break;
				case 1:
					texture->format = GFX_BC2_RGBA;
					break;
				case 7:
					texture->format = GFX_BC3_RGBA;
					break;
				default:
					LOG_WARN("unsupported DXT alpha type: %u", file->header.alpha_type);
					if (!load_dummy_mipmap(texture))
						return false;
					goto end;
			}
			for (uint32_t i = 0; i < texture->mipmaps_nb; ++i)
			{
				struct blp_texture_mipmap *mipmap = &texture->mipmaps[i];
				mipmap->width = file->mipmaps[i].width;
				mipmap->height = file->mipmaps[i].height;
				mipmap->data_len = file->mipmaps[i].data_len;
				mipmap->data = mem_malloc(MEM_GX, mipmap->data_len);
				if (!mipmap->data)
					return false;
				memcpy(mipmap->data, file->mipmaps[i].data, mipmap->data_len);
			}
			break;
		}
		case 3:
		{
			for (uint32_t i = 0; i < texture->mipmaps_nb; ++i)
			{
				struct blp_texture_mipmap *mipmap = &texture->mipmaps[i];
				mipmap->width = file->mipmaps[i].width;
				mipmap->height = file->mipmaps[i].height;
				mipmap->data_len = file->mipmaps[i].data_len;
				mipmap->data = mem_malloc(MEM_GX, mipmap->data_len);
				if (!mipmap->data)
					return false;
				memcpy(mipmap->data, file->mipmaps[i].data, mipmap->data_len);
			}
			break;
		}
		default:
		{
			LOG_WARN("unsupported BLP compression: %u", file->header.compression);
			if (!load_dummy_mipmap(texture))
				return false;
			break;
		}
	}
end:
	texture->loaded = true;
	/* XXX: remove init data
	return blp_texture_initialize(texture) == 1; */
	if (!loader_init_blp(g_wow->loader, texture))
		return false;
	return true;
}

int blp_texture_initialize(struct blp_texture *texture)
{
	if (texture->initialized || !texture->mipmaps)
		return 1;
	gfx_create_texture(g_wow->device, &texture->texture, GFX_TEXTURE_2D, texture->format, texture->mipmaps_nb, texture->mipmaps[0].width, texture->mipmaps[0].height, 0);
	gfx_set_texture_levels(&texture->texture, 0, texture->mipmaps_nb - 1);
	for (size_t i = 0; i < texture->mipmaps_nb; ++i)
	{
		struct blp_texture_mipmap *mipmap = &texture->mipmaps[i];
		gfx_set_texture_data(&texture->texture, i, 0, mipmap->width, mipmap->height, 0, mipmap->data_len, mipmap->data);
	}
	gfx_set_texture_anisotropy(&texture->texture, g_wow->anisotropy);
	gfx_set_texture_filtering(&texture->texture, GFX_FILTERING_LINEAR, GFX_FILTERING_LINEAR, GFX_FILTERING_LINEAR);
	gfx_set_texture_addressing(&texture->texture, GFX_TEXTURE_ADDRESSING_REPEAT, GFX_TEXTURE_ADDRESSING_REPEAT, GFX_TEXTURE_ADDRESSING_REPEAT);
	free_mipmaps(texture->mipmaps, texture->mipmaps_nb);
	texture->mipmaps = NULL;
	texture->initialized = true;
	return 1;
}

void blp_texture_bind(struct blp_texture *texture, uint8_t slot)
{
	const gfx_texture_t *tex;
	if (texture && texture->initialized)
		tex = &texture->texture;
	else
		tex = NULL;
	gfx_bind_samplers(g_wow->device, slot, 1, &tex);
}

bool blp_decode_rgba(const struct wow_blp_file *file, uint8_t mipmap_id, uint32_t *width, uint32_t *height, uint8_t **data)
{
	if (file->header.type != 1)
	{
		LOG_ERROR("unsupported BLP type %" PRIu32, file->header.type);
		return false;
	}
	if (mipmap_id >= file->mipmaps_nb)
	{
		LOG_ERROR("invalid mipmap id");
		return false;
	}
	const struct wow_blp_mipmap *mipmap = &file->mipmaps[mipmap_id];
	*width = mipmap->width;
	*height = mipmap->height;
	switch (file->header.compression)
	{
		case 1:
		{
			*data = mem_malloc(MEM_GENERIC, *width * *height * 4);
			if (!*data)
			{
				LOG_ERROR("failed to malloc data");
				break;
			}
			const uint8_t *indexes = mipmap->data;
			const uint8_t *alphas = indexes + *width * *height;
			uint32_t idx = 0;
			for (uint32_t i = 0; i < *width * *height; ++i)
			{
				uint32_t p = file->header.palette[indexes[i]];
				uint8_t *r = &(*data)[idx++];
				uint8_t *g = &(*data)[idx++];
				uint8_t *b = &(*data)[idx++];
				uint8_t *a = &(*data)[idx++];
				*r = p >> 16;
				*g = p >> 8;
				*b = p >> 0;
				switch (file->header.alpha_depth)
				{
					case 0:
						*a = 0xff;
						break;
					case 1:
						*a = ((alphas[i / 8] >> (i % 8)) & 1) * 0xff;
						break;
					case 4:
						*a = ((alphas[i / 2] >> ((i % 2) * 4)) & 0xf);
						*a |= *a << 4;
						break;
					case 8:
						*a = alphas[i];
						break;
					default:
						*a = 0xff;
						LOG_WARN("unsupported BLP alpha depth: %" PRIu32, file->header.alpha_depth);
						break;
				}
			}
			break;
		}
		case 2:
		{
			size_t size = *width * *height * 4;
			if (*width < 4)
				size += (4 - *width) * *height * 4;
			if (*height < 4)
				size += (4 - *height) * *width * 4;
			switch (file->header.alpha_type)
			{
				case 0:
					*data = mem_malloc(MEM_GENERIC, size);
					if (*data)
						unpack_bc1(*width, *height, mipmap->data, *data);
					else
						LOG_ERROR("failed to malloc BC1");
					break;
				case 1:
					*data = mem_malloc(MEM_GENERIC, size);
					if (data)
						unpack_bc2(*width, *height, mipmap->data, *data);
					else
						LOG_ERROR("failed to malloc BC2");
					break;
				case 7:
					*data = mem_malloc(MEM_GENERIC, size);
					if (*data)
						unpack_bc3(*width, *height, mipmap->data, *data);
					else
						LOG_ERROR("failed to malloc BC3");
					break;
				default:
					return false;
			}
			break;
		}
		case 3:
		{
			*data = mem_malloc(MEM_GENERIC, *width * *height * 4);
			if (!*data)
			{
				LOG_ERROR("failed to malloc data");
				break;
			}
			for (uint32_t i = 0; i < *width * *height * 4; i += 4)
			{
				(*data)[i + 0] = mipmap->data[i + 2];
				(*data)[i + 1] = mipmap->data[i + 1];
				(*data)[i + 2] = mipmap->data[i + 0];
				(*data)[i + 3] = mipmap->data[i + 3];
			}
			break;
		}
		default:
			return false;
	}
	return true;
}
