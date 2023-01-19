#ifndef FONT_ATLAS_H
#define FONT_ATLAS_H

#include "font/glyph.h"

#include <jks/array.h>

#include <gfx/objects.h>

#include <stdint.h>

struct font_atlas_line
{
	uint32_t width;
	uint32_t height;
	uint32_t y;
};

struct font_atlas
{
	struct jks_array lines; /* struct font_atlas_line */
	struct jks_array data; /* uint8_t */
	gfx_texture_t texture;
	uint32_t revision;
	uint32_t height;
	uint32_t width;
	uint32_t bpp;
	bool dirty_size;
	bool dirty;
};

struct font_atlas *font_atlas_new(void);
void font_atlas_delete(struct font_atlas *atlas);
bool font_atlas_find_place(struct font_atlas *atlas, uint32_t width, uint32_t height, uint32_t *x, uint32_t *y);
bool font_atlas_grow(struct font_atlas *atlas);
void font_atlas_add_glyph(struct font_atlas *atlas, uint32_t x, uint32_t y, struct font_glyph *glyph, const uint8_t *bitmap);
void font_atlas_update(struct font_atlas *atlas);
void font_atlas_bind(struct font_atlas *atlas, uint32_t bind);

#endif
