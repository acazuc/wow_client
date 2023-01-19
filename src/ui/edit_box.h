#ifndef UI_EDIT_BOX_H
#define UI_EDIT_BOX_H

#include "char_input.h"

#include "ui/font_instance.h"
#include "ui/frame.h"
#include "ui/inset.h"

#include <gfx/objects.h>

#ifdef interface
# undef interface
#endif

struct ui_font_string;
struct font;

struct ui_edit_box
{
	struct ui_frame frame;
	struct ui_font_instance font_instance;
	struct char_input char_input;
	struct ui_font_string *font_string;
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t vertexes_buffer;
	gfx_buffer_t indices_buffer;
	struct ui_font *font;
	struct ui_inset text_insets;
	float blink_speed;
	bool password;
	bool multi_line;
	int history_lines;
	bool auto_focus;
	bool ignore_arrows;
	int64_t last_action;
	int32_t clicked_x;
	int32_t clicked_y;
	int32_t indices;
	bool initialized;
};

extern const struct ui_object_vtable ui_edit_box_vtable;

bool ui_edit_box_on_key_press(struct ui_edit_box *edit_box, gfx_key_event_t *event);
bool ui_edit_box_on_char(struct ui_edit_box *edit_box, gfx_char_event_t *event);

#endif
