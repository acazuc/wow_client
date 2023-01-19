#ifndef UI_MINIMAP_H
#define UI_MINIMAP_H

#include "ui/frame.h"

#include <gfx/objects.h>

#ifdef interface
# undef interface
#endif

struct blp_texture;

struct ui_minimap
{
	struct ui_frame frame;
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t vertexes_buffer;
	gfx_buffer_t indices_buffer;
	struct blp_texture *mask;
	unsigned zoom;
	bool initialized;
};

extern const struct ui_object_vtable ui_minimap_vtable;

#endif
