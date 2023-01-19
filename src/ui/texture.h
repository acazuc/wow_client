#ifndef UI_TEXTURE_H
#define UI_TEXTURE_H

#include "itf/enum.h"

#include "ui/layered_region.h"
#include "ui/tex_coords.h"
#include "ui/gradient.h"
#include "ui/color.h"

#include <gfx/objects.h>

#ifdef interface
# undef interface
#endif

struct blp_texture;

struct ui_texture
{
	struct ui_layered_region layered_region;
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t vertexes_buffer;
	gfx_buffer_t indices_buffer;
	struct blp_texture *texture;
	struct optional_ui_tex_coords tex_coords;
	struct optional_ui_gradient gradient;
	struct optional_ui_color color;
	char *file;
	enum blend_mode alpha_mode;
	bool update_tex_coords;
	bool initialized;
};

extern const struct ui_object_vtable ui_texture_vtable;

void ui_texture_set_file(struct ui_texture *texture, const char *file);

#endif
