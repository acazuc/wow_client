#include "font/model.h"
#include "font/font.h"

#include "memory.h"
#include "log.h"
#include "wow.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H
#include FT_STROKER_H
#include FT_GLYPH_H

#define GLYPH_MARGIN 2

MEMORY_DECL(FONT);

struct font *font_new(struct font_model *parent, uint32_t size, uint32_t outline, struct font_atlas *atlas)
{
	if (!font_model_set_size(parent, size))
	{
		LOG_ERROR("failed to set font size");
		return NULL;
	}
	struct font *font = mem_malloc(MEM_FONT, sizeof(*font));
	if (!font)
	{
		LOG_ERROR("allocation failed");
		return NULL;
	}
	font->parent = parent;
	font->size = size;
	if (outline)
	{
		if (FT_Stroker_New(g_wow->ft_lib, &font->stroker))
		{
			LOG_ERROR("can't create stroker");
			free(font);
			return NULL;
		}
		FT_Stroker_Set(font->stroker, outline, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
	}
	else
	{
		font->stroker = NULL;
	}
	if (atlas)
	{
		font->atlas = atlas;
		font->atlas_owned = false;
	}
	else
	{
		font->atlas = font_atlas_new();
		if (!font->atlas)
		{
			LOG_ERROR("can't allocate font atlas");
			FT_Stroker_Done(font->stroker);
			free(font);
			return NULL;
		}
		font->atlas_owned = true;
	}
	font->height = size + 2;
	jks_hmap_init(&font->glyphs, sizeof(struct font_glyph), NULL, jks_hmap_hash_u32, jks_hmap_cmp_u32, &jks_hmap_memory_fn_FONT);
	return font;
}

void font_delete(struct font *font)
{
	if (!font)
		return;
	FT_Stroker_Done(font->stroker);
	if (font->atlas_owned)
		font_atlas_delete(font->atlas);
	jks_hmap_destroy(&font->glyphs);
	mem_free(MEM_FONT, font);
}

static struct font_glyph *load_glyph(struct font *font, uint32_t codepoint)
{
	struct font_glyph *ret = NULL;
	FT_Glyph ft_glyph = NULL;
	if (!font_model_set_size(font->parent, font->size))
		goto end;
	if (codepoint < 0x1f) /* control code */
		goto end;
	uint32_t index = FT_Get_Char_Index(font->parent->ft_face, codepoint);
	if (!index)
		goto end;
	if (FT_Load_Glyph(font->parent->ft_face, index, FT_LOAD_DEFAULT))
		goto end;
	if (!font->parent->ft_face->glyph->advance.x)
		goto end;
	if (FT_Get_Glyph(font->parent->ft_face->glyph, &ft_glyph))
		goto end;
	if (font->stroker)
		FT_Glyph_StrokeBorder(&ft_glyph, font->stroker, false, true);
	if (FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, NULL, true))
		goto end;
	FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)ft_glyph;
	struct font_glyph glyph;
	glyph.advance = font->parent->ft_face->glyph->advance.x >> 6;
	glyph.width = bitmap_glyph->bitmap.width;
	glyph.height = bitmap_glyph->bitmap.rows;
	glyph.offset_x = bitmap_glyph->left;
	glyph.offset_y = font->size - bitmap_glyph->top;
	uint32_t glyph_x;
	uint32_t glyph_y;
	if (font_atlas_find_place(font->atlas, glyph.width + GLYPH_MARGIN * 2, glyph.height + GLYPH_MARGIN * 2, &glyph_x, &glyph_y))
	{
		glyph.tex_x = glyph_x + GLYPH_MARGIN;
		glyph.tex_y = glyph_y + GLYPH_MARGIN;
		ret = jks_hmap_set(&font->glyphs, JKS_HMAP_KEY_U32(codepoint), &glyph);
		if (ret)
			font_atlas_add_glyph(font->atlas, glyph_x + GLYPH_MARGIN, glyph_y + GLYPH_MARGIN, &glyph, bitmap_glyph->bitmap.buffer);
		goto end;
	}
	if (!font_atlas_grow(font->atlas))
		goto end;
	if (font_atlas_find_place(font->atlas, glyph.width + GLYPH_MARGIN * 2, glyph.height + GLYPH_MARGIN * 2, &glyph_x, &glyph_y))
	{
		glyph.tex_x = glyph_x + GLYPH_MARGIN;
		glyph.tex_y = glyph_y + GLYPH_MARGIN;
		ret = jks_hmap_set(&font->glyphs, JKS_HMAP_KEY_U32(codepoint), &glyph);
		if (ret)
			font_atlas_add_glyph(font->atlas, glyph_x + GLYPH_MARGIN, glyph_y + GLYPH_MARGIN, &glyph, bitmap_glyph->bitmap.buffer);
		goto end;
	}
	glyph.width = 0;
	glyph.height = 0;
	glyph.tex_x = 0;
	glyph.tex_y = 0;
	ret = jks_hmap_set(&font->glyphs, JKS_HMAP_KEY_U32(codepoint), &glyph);
	if (ret)
		font_atlas_add_glyph(font->atlas, 0, 0, &glyph, NULL);
end:
	FT_Done_Glyph(ft_glyph);
	return ret;
}

struct font_glyph *font_get_glyph(struct font *font, uint32_t codepoint)
{
	struct font_glyph *glyph = jks_hmap_get(&font->glyphs, JKS_HMAP_KEY_U32(codepoint));
	if (glyph)
		return glyph;
	glyph = load_glyph(font, codepoint);
	if (glyph)
		return glyph;
	struct font_glyph tmp;
	memset(&tmp, 0, sizeof(tmp));
	return jks_hmap_set(&font->glyphs, JKS_HMAP_KEY_U32(codepoint), &tmp);
}

static void tex_coords(struct font *font, int32_t tex_x, int32_t tex_y, int32_t tex_width, int32_t tex_height, float *tex_coords)
{
	float texture_src_x = (float)tex_x / font->atlas->width;
	float texture_src_y = (float)tex_y / font->atlas->height;
	float render_width = (float)tex_width / font->atlas->width;
	float render_height = (float)tex_height / font->atlas->height;
	tex_coords[0] = texture_src_x;
	tex_coords[1] = texture_src_y;
	tex_coords[2] = texture_src_x + render_width;
	tex_coords[3] = texture_src_y;
	tex_coords[4] = texture_src_x + render_width;
	tex_coords[5] = texture_src_y + render_height;
	tex_coords[6] = texture_src_x;
	tex_coords[7] = texture_src_y + render_height;
}

void font_glyph_tex_coords(struct font *font, struct font_glyph *glyph, float *coords)
{
	if (!glyph)
	{
		for (size_t i = 0; i < 8; ++i)
			coords[i] = 0;
		return;
	}
	tex_coords(font, glyph->tex_x, glyph->tex_y, glyph->width, glyph->height, coords);
}
