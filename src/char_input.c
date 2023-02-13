#include "char_input.h"

#include "memory.h"
#include "log.h"
#include "wow.h"

#include <gfx/window.h>

#include <jks/utf8.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

static void delete_selected(struct char_input *input);
static void delete_bytes_left(struct char_input *input, uint32_t bytes);
static void delete_bytes_right(struct char_input *input, uint32_t bytes);
static void delete_char_left(struct char_input *input);
static void delete_char_right(struct char_input *input);
static void delete_word_left(struct char_input *input);
static void delete_word_right(struct char_input *input);
static void select_bytes_left(struct char_input *input, uint32_t bytes);
static void select_bytes_right(struct char_input *input, uint32_t bytes);
static void select_char_left(struct char_input *input);
static void select_char_right(struct char_input *input);
static void select_word_left(struct char_input *input);
static void select_word_right(struct char_input *input);
static void move_selected_left(struct char_input *input);
static void move_selected_right(struct char_input *input);
static void move_char_left(struct char_input *input);
static void move_char_right(struct char_input *input);
static void move_word_left(struct char_input *input);
static void move_word_right(struct char_input *input);
static int32_t get_char_left_size(struct char_input *input);
static int32_t get_char_right_size(struct char_input *input);
static int32_t get_word_left_size(struct char_input *input);
static int32_t get_word_right_size(struct char_input *input);
static bool is_word_delimiter(uint32_t c);
static void update_text(struct char_input *input, char *text, int32_t cursor_position, int32_t selected_position);

void char_input_init(struct char_input *input)
{
	input->text = NULL;
	input->bytes = 0;
	input->selected_position = 0;
	input->cursor_position = 0;
	input->max_bytes = 0;
	input->max_chars = 0;
	input->flags = CHAR_INPUT_INLINE | CHAR_INPUT_EDITABLE | CHAR_INPUT_MOVABLE;
	input->on_enabled_changed = NULL;
	input->on_editable_changed = NULL;
	input->on_changed = NULL;
	input->on_cursor_position_changed = NULL;
	input->on_selected_position_changed = NULL;
	input->userptr = NULL;
}

void char_input_destroy(struct char_input *input)
{
	mem_free(MEM_GENERIC, input->text);
}

bool char_input_on_key_press(struct char_input *input, struct gfx_key_event *event)
{
	if (input->flags & CHAR_INPUT_EDITABLE)
	{
		if (event->key == GFX_KEY_V && event->mods & GFX_KEY_MOD_CONTROL)
		{
			if (!(input->flags & CHAR_INPUT_NUMERIC))
			{
				char *clipboard = gfx_window_get_clipboard(g_wow->window);
				if (input->flags & CHAR_INPUT_INLINE)
				{
					char *pos;
					while ((pos = strchr(clipboard, '\n')))
						strcpy(pos, pos + 1);
				}
				char_input_insert_char(input, clipboard);
				free(clipboard);
			}
			return true;
		}
		if (event->key == GFX_KEY_BACKSPACE)
		{
			if (event->mods & GFX_KEY_MOD_CONTROL)
				delete_word_left(input);
			else
				delete_char_left(input);
			return true;
		}
		if (event->key == GFX_KEY_DELETE)
		{
			if (event->mods & GFX_KEY_MOD_CONTROL)
				delete_word_right(input);
			else
				delete_char_right(input);
			return true;
		}
		if (!(input->flags & CHAR_INPUT_INLINE))
		{
			if (event->key == GFX_KEY_ENTER || event->key == GFX_KEY_KP_ENTER)
			{
				char_input_insert_char(input, "\n");
				return true;
			}
		}
	}
	if (event->key == GFX_KEY_ESCAPE)
	{
		char_input_set_enabled(input, false);
		return true;
	}
	if (event->key == GFX_KEY_C && event->mods & GFX_KEY_MOD_CONTROL)
	{
		if (input->selected_position > 0)
		{
			char *sub = mem_malloc(MEM_GENERIC, input->selected_position + 1);
			if (sub)
			{
				memcpy(sub, input->text + input->bytes + input->cursor_position, input->selected_position);
				sub[input->selected_position] = '\0';
				gfx_window_set_clipboard(g_wow->window, sub);
				mem_free(MEM_GENERIC, sub);
			}
			else
			{
				LOG_ERROR("malloc failed");
			}
		}
		else if (input->selected_position < 0)
		{
			char *sub = mem_malloc(MEM_GENERIC, -input->selected_position + 1);
			if (sub)
			{
				memcpy(sub, input->text + input->bytes + input->cursor_position, -input->selected_position);
				sub[-input->selected_position] = '\0';
				gfx_window_set_clipboard(g_wow->window, sub);
				mem_free(MEM_GENERIC, sub);
			}
			else
			{
				LOG_ERROR("malloc failed");
			}
		}
		return true;
	}
	if (event->key == GFX_KEY_A && event->mods & GFX_KEY_MOD_CONTROL)
	{
		char_input_set_cursor_position(input, 0);
		char_input_set_selected_position(input, input->selected_position - char_input_utf8length(input));
		return true;
	}
	if (input->flags & CHAR_INPUT_MOVABLE)
	{
		if (event->key == GFX_KEY_LEFT)
		{
			if (event->mods & GFX_KEY_MOD_SHIFT)
			{
				if (event->mods & GFX_KEY_MOD_CONTROL)
					select_word_left(input);
				else
					select_char_left(input);
			}
			else if (input->flags & CHAR_INPUT_EDITABLE)
			{
				if (event->mods & GFX_KEY_MOD_CONTROL)
					move_word_left(input);
				else
					move_char_left(input);
			}
			return true;
		}
		if (event->key == GFX_KEY_RIGHT)
		{
			if (event->mods & GFX_KEY_MOD_SHIFT)
			{
				if (event->mods & GFX_KEY_MOD_CONTROL)
					select_word_right(input);
				else
					select_char_right(input);
			}
			else if (input->flags & CHAR_INPUT_EDITABLE)
			{
				if (event->mods & GFX_KEY_MOD_CONTROL)
					move_word_right(input);
				else
					move_char_right(input);
			}
			return true;
		}
		if (event->key == GFX_KEY_END)
		{
			if (event->mods & GFX_KEY_MOD_SHIFT)
				char_input_set_selected_position(input, input->selected_position + input->cursor_position);
			else
				char_input_set_selected_position(input, 0);
			char_input_set_cursor_position(input, 0);
			return true;
		}
	}
	return false;
}

bool char_input_on_char(struct char_input *input, struct gfx_char_event *event)
{
	if (!(input->flags & CHAR_INPUT_EDITABLE))
		return true;
	if (input->flags & CHAR_INPUT_NUMERIC)
	{
		if (isdigit(event->utf8[0]))
		{
			char_input_insert_char(input, event->utf8);
			return true;
		}
	}
	else
	{
		char_input_insert_char(input, event->utf8);
		return true;
	}
	return false;
}

static int32_t get_char_left_size(struct char_input *input)
{
	if (!input->bytes)
		return 0;
	if (input->cursor_position == -(int32_t)input->bytes)
		return 0;
	const char *orig = input->text + input->bytes + input->cursor_position;
	const char *iter = orig;
	uint32_t cp;
	if (!utf8_prev(&iter, input->text, &cp))
		return 0;
	return orig - iter;
}

static int32_t get_char_right_size(struct char_input *input)
{
	if (!input->bytes)
		return 0;
	if (!input->cursor_position)
		return 0;
	const char *orig = input->text + input->bytes + input->cursor_position;
	const char *iter = orig;
	uint32_t cp;
	if (!utf8_next(&iter, input->text + input->bytes, &cp))
		return 0;
	return iter - orig;
}

static int32_t get_word_left_size(struct char_input *input)
{
	if (!input->bytes)
		return 0;
	if (input->cursor_position == -(int32_t)input->bytes)
		return 0;
	const char *orig = input->text + input->bytes + input->cursor_position;
	const char *iter = orig;
	while (iter != input->text)
	{
		uint32_t current_char;
		if (!utf8_prev(&iter, input->text, &current_char))
			return 0;
		if (iter == input->text || is_word_delimiter(current_char))
			return orig - iter;
	}
	return 0;
}

static int32_t get_word_right_size(struct char_input *input)
{
	if (!input->bytes)
		return 0;
	if (!input->cursor_position)
		return 0;
	const char *orig = input->text + input->bytes + input->cursor_position;
	const char *iter = orig;
	const char *end = input->text + input->bytes;
	while (iter != end)
	{
		uint32_t current_char;
		if (!utf8_next(&iter, input->text + input->bytes, &current_char))
			return 0;
		if (iter == end || is_word_delimiter(current_char))
			return iter - orig;
	}
	return 0;
}

static bool is_word_delimiter(uint32_t c)
{
	if (isspace(c))
		return true;
	if (c >= '!' && c <= '/')
		return true;
	if (c >= ':' && c <= '@')
		return true;
	if (c >= '[' && c <= '`')
		return true;
	if (c >= '{' && c <= '}')
		return true;
	return false;
}

static void update_text(struct char_input *input, char *text, int32_t cursor_position, int32_t selected_position)
{
	mem_free(MEM_GENERIC, input->text);
	input->text = text;
	input->bytes = input->text ? strlen(input->text) : 0;
	char_input_set_selected_position(input, selected_position);
	char_input_set_cursor_position(input, cursor_position);
	if (input->on_changed)
		input->on_changed(input->userptr);
}

void char_input_insert_char(struct char_input *input, const char *c)
{
	if (!(input->flags & CHAR_INPUT_EDITABLE))
	{
		LOG_DEBUG("not editable");
		return;
	}
	if (input->flags & CHAR_INPUT_NUMERIC)
	{
		if (c[0] == '0')
		{
			if (char_input_utf8length(input) == 0 || input->cursor_position == -(int32_t)input->bytes)
			{
				LOG_DEBUG("input too long");
				return;
			}
		}
	}
	int c_len = strlen(c);
	if (!utf8_is_valid(c, c + c_len))
	{
		LOG_ERROR("invalid utf8");
		return;
	}
	if (input->selected_position)
		delete_selected(input);
	size_t str_distance;
	if (!utf8_distance(c, c + c_len, &str_distance))
	{
		LOG_ERROR("can't get utf8 length");
		return;
	}
	uint32_t string_length = char_input_utf8length(input);
	if ((input->max_chars && string_length >= input->max_chars) || (input->max_bytes && input->bytes >= input->max_bytes))
	{
		LOG_DEBUG("input too long");
		return;
	}
	if ((input->max_chars && string_length + str_distance > input->max_chars) || (input->max_bytes && input->bytes + c_len > input->max_bytes))
	{
		int32_t iter_dist = 0;
		const char *iter = c;
		const char *orig = iter;
		while (1)
		{
			++iter_dist;
			const char *old_iter = iter;
			uint32_t tmp;
			if (!utf8_next(&old_iter, c + c_len, &tmp))
				break;
			if ((input->max_chars && string_length + iter_dist > input->max_chars) || (input->max_bytes && input->bytes + old_iter - iter > input->max_bytes))
				break;
			iter = old_iter;
			if (old_iter == c + c_len)
				break;
		}
		if (iter == orig)
			return;
		c_len = iter - orig;
	}
	if (input->cursor_position == -(int32_t)input->bytes)
	{
		char *tmp;
		if (wow_asprintf(MEM_GENERIC, &tmp, "%.*s%s", c_len, c, input->text ? input->text : "") == -1)
			tmp = NULL;
		update_text(input, tmp, input->cursor_position, input->selected_position);
	}
	else if (!input->cursor_position)
	{
		char *tmp;
		if (wow_asprintf(MEM_GENERIC, &tmp, "%s%.*s", input->text ? input->text : "", c_len, c) == -1)
			tmp = NULL;
		update_text(input, tmp, input->cursor_position, input->selected_position);
	}
	else
	{
		char *tmp;
		if (wow_asprintf(MEM_GENERIC, &tmp, "%.*s%.*s%.*s", input->bytes + input->cursor_position, input->text ? input->text : "", c_len, c, -input->cursor_position, input->text ? input->text + input->bytes + input->cursor_position : 0) == -1)
			tmp = NULL;
		update_text(input, tmp, input->cursor_position, input->selected_position);
	}
}

static void move_selected_left(struct char_input *input)
{
	if (!input->selected_position)
		return;
	if (input->selected_position < 0)
		char_input_set_cursor_position(input, input->cursor_position + input->selected_position);
	char_input_set_selected_position(input, 0);
}

static void move_selected_right(struct char_input *input)
{
	if (!input->selected_position)
		return;
	if (input->selected_position > 0)
		char_input_set_cursor_position(input, input->cursor_position + input->selected_position);
	char_input_set_selected_position(input, 0);
}

static void move_char_left(struct char_input *input)
{
	if (input->selected_position)
		move_selected_left(input);
	else if (input->bytes && input->cursor_position > -(int32_t)input->bytes)
		char_input_set_cursor_position(input, input->cursor_position - get_char_left_size(input));
}

static void move_char_right(struct char_input *input)
{
	if (input->selected_position)
		move_selected_right(input);
	else if (input->bytes > 0 && input->cursor_position < 0)
		char_input_set_cursor_position(input, input->cursor_position + get_char_right_size(input));
}

static void move_word_left(struct char_input *input)
{
	if (input->selected_position)
		move_selected_left(input);
	else if (input->bytes > 0 && input->cursor_position > -(int32_t)input->bytes)
		char_input_set_cursor_position(input, input->cursor_position - get_word_left_size(input));
}

static void move_word_right(struct char_input *input)
{
	if (input->selected_position)
		move_selected_right(input);
	else if (input->bytes > 0 && input->cursor_position < 0)
		char_input_set_cursor_position(input, input->cursor_position + get_word_right_size(input));
}

static void delete_selected(struct char_input *input)
{
	if (!input->selected_position)
		return;
	const char *tmp1;
	int tmp1_len;
	const char *tmp2;
	int tmp2_len;
	if (input->selected_position < 0)
	{
		tmp1 = input->text;
		tmp1_len = input->bytes + input->cursor_position + input->selected_position;
		tmp2 = input->text + input->bytes + input->cursor_position;
		tmp2_len = -input->cursor_position;
	}
	else
	{
		tmp1 = input->text;
		tmp1_len = input->bytes + input->cursor_position;
		tmp2 = input->text + input->bytes + input->cursor_position + input->selected_position;
		tmp2_len = -input->cursor_position - input->selected_position;
		char_input_set_cursor_position(input, input->cursor_position + input->selected_position);
	}
	char *tmp;
	if (wow_asprintf(MEM_GENERIC, &tmp, "%.*s%.*s", tmp1_len, tmp1, tmp2_len, tmp2) == -1)
		tmp = NULL;
	update_text(input, tmp, input->cursor_position, 0);
}

static void delete_bytes_left(struct char_input *input, uint32_t bytes)
{
	if (!input->bytes)
		return;
	if (input->selected_position)
	{
		delete_selected(input);
		return;
	}
	if (!input->cursor_position)
	{
		char *tmp;
		if (wow_asprintf(MEM_GENERIC, &tmp, "%.*s", input->bytes - bytes, input->text) == -1)
			tmp = NULL;
		update_text(input, tmp, input->cursor_position, input->selected_position);
	}
	else if (input->cursor_position > -(int32_t)input->bytes)
	{
		char *tmp;
		if (wow_asprintf(MEM_GENERIC, &tmp, "%.*s%.*s", input->bytes + input->cursor_position - bytes, input->text, -input->cursor_position, input->text + input->bytes + input->cursor_position) == -1)
			tmp = NULL;
		update_text(input, tmp, input->cursor_position, input->selected_position);
	}
}

static void delete_bytes_right(struct char_input *input, uint32_t bytes)
{
	if (!input->bytes)
		return;
	if (input->selected_position)
	{
		delete_selected(input);
		return;
	}
	if (input->cursor_position == -(int32_t)input->bytes)
	{
		char *tmp;
		if (wow_asprintf(MEM_GENERIC, &tmp, "%.*s", input->bytes - bytes, input->text + bytes) == -1)
			tmp = NULL;
		update_text(input, tmp, input->cursor_position + bytes, 0);
	}
	else if (input->cursor_position < 0)
	{
		char *tmp;
		if (wow_asprintf(MEM_GENERIC, &tmp, "%.*s%.*s", input->bytes + input->cursor_position - bytes, input->text, -input->cursor_position - bytes, input->text + input->bytes + input->cursor_position + bytes) == -1)
			tmp = NULL;
		update_text(input, tmp, input->cursor_position + bytes, 0);
	}
}

static void delete_char_left(struct char_input *input)
{
	delete_bytes_left(input, get_char_left_size(input));
}

static void delete_char_right(struct char_input *input)
{
	delete_bytes_right(input, get_char_right_size(input));
}

static void delete_word_left(struct char_input *input)
{
	delete_bytes_left(input, get_word_left_size(input));
}

static void delete_word_right(struct char_input *input)
{
	delete_bytes_right(input, get_word_right_size(input));
}

static void select_bytes_left(struct char_input *input, uint32_t bytes)
{
	if (!bytes)
		return;
	if (input->cursor_position > -(int32_t)input->bytes)
	{
		char_input_set_selected_position(input, input->selected_position + bytes);
		char_input_set_cursor_position(input, input->cursor_position - bytes);
	}
}

static void select_bytes_right(struct char_input *input, uint32_t bytes)
{
	if (!bytes)
		return;
	if (input->cursor_position < 0)
	{
		int32_t diff = get_char_right_size(input);
		char_input_set_selected_position(input, input->selected_position - diff);
		char_input_set_cursor_position(input, input->cursor_position + diff);
	}
}

static void select_char_left(struct char_input *input)
{
	select_bytes_left(input, get_char_left_size(input));
}

static void select_char_right(struct char_input *input)
{
	select_bytes_right(input, get_char_right_size(input));
}

static void select_word_left(struct char_input *input)
{
	select_bytes_left(input, get_word_left_size(input));
}

static void select_word_right(struct char_input *input)
{
	select_bytes_right(input, get_word_right_size(input));
}

uint32_t char_input_length(struct char_input *input)
{
	if (!input->text)
		return 0;
	return input->bytes;
}

uint32_t char_input_utf8length(struct char_input *input)
{
	if (!input->text)
		return 0;
	size_t distance;
	if (!utf8_distance(input->text, input->text + input->bytes, &distance))
		return 0;
	return distance;
}

void char_input_set_text(struct char_input *input, const char *str)
{
	mem_free(MEM_GENERIC, input->text);
	input->text = mem_strdup(MEM_GENERIC, str);
	input->bytes = input->text ? strlen(input->text) : 0;
	char_input_set_cursor_position(input, 0);
	char_input_set_selected_position(input, 0);
	if (input->on_changed)
		input->on_changed(input->userptr);
}

void char_input_append(struct char_input *input, const char *text)
{
	char_input_insert_char(input, text);
}

void char_input_set_cursor_position(struct char_input *input, int32_t position)
{
	if (input->cursor_position == position)
		return;
	input->cursor_position = position;
	if (input->on_cursor_position_changed)
		input->on_cursor_position_changed(input->userptr);
}

void char_input_set_selected_position(struct char_input *input, int32_t position)
{
	if (input->selected_position == position)
		return;
	input->selected_position = position;
	if (input->on_selected_position_changed)
		input->on_selected_position_changed(input->userptr);
}

void char_input_set_enabled(struct char_input *input, bool enabled)
{
	if (!(input->flags & CHAR_INPUT_EDITABLE))
		enabled = false;
	if (!!(input->flags & CHAR_INPUT_ENABLED) == !!enabled)
		return;
	if (enabled)
		input->flags |= CHAR_INPUT_ENABLED;
	else
		input->flags &= ~CHAR_INPUT_ENABLED;
	if (input->on_enabled_changed)
		input->on_enabled_changed(input->userptr);
}

void char_input_set_editable(struct char_input *input, bool editable)
{
	if (!!(input->flags & CHAR_INPUT_EDITABLE) == !!editable)
		return;
	if (editable)
		input->flags |= CHAR_INPUT_EDITABLE;
	else
		input->flags &= ~CHAR_INPUT_EDITABLE;
	if (!editable)
		char_input_set_enabled(input, false);
	if (input->on_editable_changed)
		input->on_editable_changed(input->userptr);
}
