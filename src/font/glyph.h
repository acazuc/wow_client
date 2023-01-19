#ifndef FONT_GLYPH_H
#define FONT_GLYPH_H

#include <stdint.h>

struct font_glyph
{
	uint16_t advance;
	uint16_t height;
	uint16_t width;
	uint16_t tex_x;
	uint16_t tex_y;
	int16_t offset_x;
	int16_t offset_y;
};

#endif
