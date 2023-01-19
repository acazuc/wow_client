#ifndef INTERFACE_FONT_H
#define INTERFACE_FONT_H

#include "font/atlas.h"
#include "font/font.h"

#include <jks/hmap.h>

struct interface_font_model
{
	struct jks_hmap fonts; /* uint32_t, struct interface_font */
	struct font_model *model;
	char *name;
};

struct interface_font
{
	struct interface_font_model *model;
	struct font_atlas *atlas;
	struct font *font;
	struct font *outline_normal;
	struct font *outline_thick;
	uint32_t refcount;
	uint32_t size;
};

#endif
