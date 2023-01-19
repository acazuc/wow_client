#include "font/atlas.h"

#include "memory.h"
#include "log.h"
#include "wow.h"

#include <gfx/device.h>

#include <string.h>

#define GROW_SIZE 256

MEMORY_DECL(FONT);

struct font_atlas *font_atlas_new(void)
{
	struct font_atlas *atlas = mem_malloc(MEM_FONT, sizeof(*atlas));
	if (!atlas)
	{
		LOG_ERROR("atlas allocation failed");
		return NULL;
	}
	atlas->revision = 1;
	atlas->height = 0;
	atlas->width = 256;
	atlas->bpp = 32;
	atlas->dirty = false;
	atlas->dirty_size = false;
	jks_array_init(&atlas->lines, sizeof(struct font_atlas_line), NULL, &jks_array_memory_fn_FONT);
	jks_array_init(&atlas->data, sizeof(uint8_t), NULL, &jks_array_memory_fn_FONT);
	atlas->texture = GFX_TEXTURE_INIT();
	return atlas;
}

void font_atlas_delete(struct font_atlas *atlas)
{
	if (!atlas)
		return;
	jks_array_destroy(&atlas->lines);
	jks_array_destroy(&atlas->data);
	gfx_delete_texture(g_wow->device, &atlas->texture);
	mem_free(MEM_FONT, atlas);
}

void font_atlas_add_glyph(struct font_atlas *atlas, uint32_t x, uint32_t y, struct font_glyph *glyph, const uint8_t *bitmap)
{
	if (atlas->bpp == 32)
	{
		for (uint32_t tY = 0; tY < glyph->height; ++tY)
		{
			for (uint32_t tX = 0; tX < glyph->width; ++tX)
			{
				*JKS_ARRAY_GET(&atlas->data, (y + tY) * atlas->width + x + tX, uint32_t) = 0xFFFFFF | ((uint32_t)bitmap[tY * glyph->width + tX] << 24);
			}
		}
	}
	else
	{
		for (uint32_t tY = 0; tY < glyph->height; ++tY)
		{
			for (uint32_t tX = 0; tX < glyph->width; ++tX)
			{
				*JKS_ARRAY_GET(&atlas->data, (y + tY) * atlas->width + x + tX, uint8_t) = bitmap[tY * glyph->width + tX];
			}
		}
	}
	atlas->dirty = true;
}

bool font_atlas_find_place(struct font_atlas *atlas, uint32_t width, uint32_t height, uint32_t *x, uint32_t *y)
{
	if (width > atlas->width || height > atlas->height)
		return false;
	for (uint32_t i = 0; i < atlas->lines.size; ++i)
	{
		struct font_atlas_line *line = JKS_ARRAY_GET(&atlas->lines, i, struct font_atlas_line);
		if (line->height >= height)
		{
			if (atlas->width - line->width >= width)
			{
				*x = line->width;
				*y = line->y;
				line->width += width;
				if (height > line->height)
					line->height = height;
				return true;
			}
			continue;
		}
		if (atlas->width - line->width < width)
			continue;
		if (i != atlas->lines.size - 1)
			continue;
		if (atlas->height - line->y < height)
			continue;
		line->height = height;
		*x = line->width;
		*y = line->y;
		line->width += width;
		if (height > line->height)
			line->height = height;
		return true;
	}
	uint32_t new_y = atlas->lines.size ? JKS_ARRAY_GET(&atlas->lines, atlas->lines.size - 1, struct font_atlas_line)->y + JKS_ARRAY_GET(&atlas->lines, atlas->lines.size - 1, struct font_atlas_line)->height : 0;
	if (height > atlas->height - new_y)
		return false;
	struct font_atlas_line *new_line = jks_array_grow(&atlas->lines, 1);
	if (!new_line)
	{
		LOG_ERROR("failed to grow new line");
		return false;
	}
	new_line->y = new_y;
	new_line->width = width;
	new_line->height = height;
	*x = 0;
	*y = new_y;
	return true;
}

bool font_atlas_grow(struct font_atlas *atlas)
{
	atlas->height += GROW_SIZE;
	size_t base = atlas->data.size;
	if (!jks_array_resize(&atlas->data, atlas->width * atlas->height * atlas->bpp / 8))
	{
		LOG_ERROR("failed to grow data");
		return false;
	}
	memset(JKS_ARRAY_GET(&atlas->data, base, uint8_t), 0, atlas->width * GROW_SIZE * atlas->bpp / 8);
	atlas->dirty_size = true;
	return true;
}

void font_atlas_update(struct font_atlas *atlas)
{
	if (atlas->dirty_size)
	{
		atlas->dirty_size = false;
		if (!atlas->height || !atlas->width)
			return;
		gfx_delete_texture(g_wow->device, &atlas->texture);
		if (atlas->bpp == 32)
			gfx_create_texture(g_wow->device, &atlas->texture, GFX_TEXTURE_2D, GFX_B8G8R8A8, 1, atlas->width, atlas->height, 0);
		else
			gfx_create_texture(g_wow->device, &atlas->texture, GFX_TEXTURE_2D, GFX_R8, 1, atlas->width, atlas->height, 0);
		gfx_set_texture_levels(&atlas->texture, 0, 0);
		gfx_set_texture_anisotropy(&atlas->texture, 1);
		gfx_set_texture_filtering(&atlas->texture, GFX_FILTERING_LINEAR, GFX_FILTERING_LINEAR, GFX_FILTERING_NONE);
		gfx_set_texture_addressing(&atlas->texture, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP);
		atlas->revision++;
	}
	else if (!atlas->dirty)
	{
		return;
	}
#if 0
	if (font->atlas->bpp == 32)
	{
		for (size_t y = 0; y < 100; ++y)
		{
			for (size_t x = 0; x < 100; ++x)
			{
				float a = ((uint8_t*)font->atlas->data.data)[3 + 4 * (x + font->atlas->width * y)];
				if (a > 200)
					printf("#");
				else if (a > 150)
					printf("%%");
				else if (a > 100)
					printf("+");
				else if (a > 50)
					printf(".");
				else
					printf(" ");
			}
			printf("\n");
		}
	}
	else
	{
		for (size_t y = 0; y < 100; ++y)
		{
			for (size_t x = 0; x < 100; ++x)
			{
				float a = ((uint8_t*)font->atlas->data.data)[x + font->atlas->width * y];
				if (a > 200)
					printf("#");
				else if (a > 150)
					printf("%%");
				else if (a > 100)
					printf("+");
				else if (a > 50)
					printf(".");
				else
					printf(" ");
			}
			printf("\n");
		}
	}
#endif
	gfx_set_texture_data(&atlas->texture, 0, 0, atlas->width, atlas->height, 0, atlas->data.size, atlas->data.data);
	atlas->dirty = false;
}

void font_atlas_bind(struct font_atlas *atlas, uint32_t bind)
{
	const gfx_texture_t *texture;
	if (atlas->texture.handle.u64)
		texture = &atlas->texture;
	else
		texture = NULL;
	gfx_bind_samplers(g_wow->device, bind, 1, &texture);
}
