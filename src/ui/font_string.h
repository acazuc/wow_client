#ifndef UI_FONT_STRING_H
#define UI_FONT_STRING_H

#include "ui/layered_region.h"
#include "ui/font_instance.h"

#include <gfx/objects.h>

#ifdef interface
# undef interface
#endif

struct ui_font;

struct ui_font_string
{
	struct ui_layered_region layered_region;
	struct ui_font_instance font_instance;
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t vertexes_buffer;
	gfx_buffer_t indices_buffer;
	int bytes;
	char *text;
	bool nonspacewrap;
	int max_lines;
	bool indented;
	size_t indices_nb;
	int32_t text_width;
	int32_t text_height;
	bool bypass_size;
	bool initialized;
	bool dirty_buffers;
	bool dirty_size;
	struct interface_font *last_font;
	uint32_t last_font_revision;
};

extern const struct ui_object_vtable ui_font_string_vtable;

void ui_font_string_set_text(struct ui_font_string *font_string, const char *text);
int32_t ui_font_string_get_text_left(struct ui_font_string *font_string);
int32_t ui_font_string_get_text_top(struct ui_font_string *font_string);
int32_t ui_font_string_get_text_width(struct ui_font_string *font_string);
int32_t ui_font_string_get_text_height(struct ui_font_string *font_string);
void ui_font_string_update_size(struct ui_font_string *font_string, struct interface_font *font);
void ui_font_string_set_font_instance(struct ui_font_string *font_string, struct ui_font *font);

#endif
