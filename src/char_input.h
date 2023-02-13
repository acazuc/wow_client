#ifndef CHAR_INPUT_H
#define CHAR_INPUT_H

#include <gfx/events.h>

#include <stdbool.h>

enum char_input_flag
{
	CHAR_INPUT_INLINE = 0x1,
	CHAR_INPUT_EDITABLE = 0x2,
	CHAR_INPUT_NUMERIC = 0x4,
	CHAR_INPUT_ENABLED = 0x8,
	CHAR_INPUT_MOVABLE = 0x10,
};

struct char_input
{
	char *text;
	uint32_t bytes;
	int32_t selected_position;
	int32_t cursor_position;
	uint32_t max_bytes;
	uint32_t max_chars;
	uint8_t flags;
	void *userptr;
	void (*on_enabled_changed)(void *);
	void (*on_editable_changed)(void *);
	void (*on_changed)(void *);
	void (*on_cursor_position_changed)(void *);
	void (*on_selected_position_changed)(void *);
};

void char_input_init(struct char_input *input);
void char_input_destroy(struct char_input *input);
void char_input_set_text(struct char_input *input, const char *text);
void char_input_append(struct char_input *input, const char *text);
void char_input_insert_char(struct char_input *input, const char *text);
bool char_input_on_key_press(struct char_input *input, struct gfx_key_event *event);
bool char_input_on_char(struct char_input *input, struct gfx_char_event *event);
uint32_t char_input_utf8length(struct char_input *input);
void char_input_set_editable(struct char_input *input, bool editable);
void char_input_set_enabled(struct char_input *input, bool enabled);
void char_input_set_cursor_position(struct char_input *input, int32_t cursor_position);
void char_input_set_selected_position(struct char_input *input, int32_t selected_position);

#endif
