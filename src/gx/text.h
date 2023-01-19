#ifndef GX_TEXT_H
#define GX_TEXT_H

#include <jks/vec3.h>
#include <jks/vec4.h>

#include <gfx/objects.h>

struct gx_text
{
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t vertexes_buffer;
	gfx_buffer_t indices_buffer;
	struct vec3f position;
	struct vec4f color;
	struct vec3f scale;
	char *str;
	uint32_t font_revision;
	uint32_t indices;
	bool in_render_list;
	bool dirty;
};

struct gx_text *gx_text_new(void);
void gx_text_delete(struct gx_text *text);
void gx_text_render(struct gx_text *text);
void gx_text_add_to_render(struct gx_text *text);
void gx_text_set_text(struct gx_text *text, const char *str);
void gx_text_set_color(struct gx_text *text, struct vec4f color);
void gx_text_set_pos(struct gx_text *text, struct vec3f pos);
void gx_text_set_scale(struct gx_text *text, struct vec3f scale);

#endif
