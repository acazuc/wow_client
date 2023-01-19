#ifndef UI_SCROLLING_MESSAGE_FRAME_H
#define UI_SCROLLING_MESSAGE_FRAME_H

#include "ui/font_instance.h"
#include "ui/frame.h"

#include <gfx/objects.h>

#ifdef interface
# undef interface
#endif

struct ui_scrolling_message
{
	char *text;
	float red;
	float green;
	float blue;
	uint32_t id;
};

struct ui_scrolling_message_frame
{
	struct ui_frame frame;
	struct ui_font_instance font_instance;
	struct jks_array messages; /* struct ui_scrolling_message */
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t vertexes_buffer;
	gfx_buffer_t indices_buffer;
	bool initialized;
	bool dirty_buffers;
	size_t indices_nb;
	struct interface_font *last_font;
	uint32_t last_font_revision;
};

extern const struct ui_object_vtable ui_scrolling_message_frame_vtable;

#endif
