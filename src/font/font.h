#ifndef FONT_H
#define FONT_H

#include "font/atlas.h"

#include <jks/hmap.h>

typedef struct FT_StrokerRec_* FT_Stroker;

struct font_model;

struct font
{
	struct jks_hmap glyphs; /* uint32_t, struct font_glyph */
	FT_Stroker stroker;
	struct font_model *parent;
	struct font_atlas *atlas;
	uint32_t height;
	uint32_t size;
	bool atlas_owned;
};

struct font *font_new(struct font_model *parent, uint32_t size, uint32_t outline, struct font_atlas *atlas);
void font_delete(struct font *font);
struct font_glyph *font_get_glyph(struct font *font, uint32_t codepoint);
void font_glyph_tex_coords(struct font *font, struct font_glyph *glyph, float *tex_coords);

#endif
