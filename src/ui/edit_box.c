#include "edit_box.h"

#include "itf/interface.h"
#include "itf/font.h"

#include "ui/font_string.h"

#include "xml/edit_box.h"

#include "font/font.h"

#include "wow_lua.h"
#include "shaders.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <gfx/window.h>
#include <gfx/device.h>

#include <jks/utf8.h>

#include <string.h>
#include <stdlib.h>

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_EDIT_BOX() LUA_METHOD(EditBox, edit_box)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_FRAME->region)
#define UI_FRAME (&edit_box->frame)
#define UI_FONT_INSTANCE (&edit_box->font_instance)

MEMORY_DECL(UI);

static void on_selected_position_changed(void *ptr);
static void on_cursor_position_changed(void *ptr);
static void on_enabled_changed(void *ptr);
static void on_changed(void *ptr);
static void load_font_string(struct ui_edit_box *edit_box);
static void on_font_height_changed(struct ui_object *object);
static void on_color_changed(struct ui_object *object);
static void on_shadow_color_changed(struct ui_object *object);
static void on_spacing_changed(struct ui_object *object);
static void on_outline_changed(struct ui_object *object);
static void on_monochrome_changed(struct ui_object *object);
static void on_justify_h_changed(struct ui_object *object);
static void on_justify_v_changed(struct ui_object *object);

static const struct ui_font_instance_callbacks g_edit_box_font_instance_callbacks =
{
	.on_font_height_changed = on_font_height_changed,
	.on_color_changed = on_color_changed,
	.on_shadow_color_changed = on_shadow_color_changed,
	.on_spacing_changed = on_spacing_changed,
	.on_outline_changed = on_outline_changed,
	.on_monochrome_changed = on_monochrome_changed,
	.on_justify_h_changed = on_justify_h_changed,
	.on_justify_v_changed = on_justify_v_changed,
};

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_frame_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_edit_box *edit_box = (struct ui_edit_box*)object;
	UI_OBJECT->mask |= UI_OBJECT_edit_box;
	edit_box->font_string = NULL;
	edit_box->font = NULL;
	edit_box->blink_speed = .5;
	edit_box->password = false;
	edit_box->history_lines = 0;
	edit_box->auto_focus = true;
	edit_box->multi_line = false;
	edit_box->ignore_arrows = false;
	UI_REGION->keyboard_enabled = true;
	UI_REGION->mouse_enabled = true;
	char_input_init(&edit_box->char_input);
	edit_box->char_input.on_selected_position_changed = on_selected_position_changed;
	edit_box->char_input.on_cursor_position_changed = on_cursor_position_changed;
	edit_box->char_input.on_enabled_changed = on_enabled_changed;
	edit_box->char_input.on_changed = on_changed;
	edit_box->char_input.userptr = edit_box;
	ui_font_instance_init(interface, UI_FONT_INSTANCE, object, &g_edit_box_font_instance_callbacks);
	ui_inset_init(&edit_box->text_insets, 0, 0, 0, 0);
	edit_box->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		edit_box->uniform_buffers[i] = GFX_BUFFER_INIT();
	edit_box->vertexes_buffer = GFX_BUFFER_INIT();
	edit_box->indices_buffer = GFX_BUFFER_INIT();
	edit_box->indices = 0;
	edit_box->initialized = false;
	return true;
}

static void dtr(struct ui_object *object)
{
	struct ui_edit_box *edit_box = (struct ui_edit_box*)object;
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_delete_buffer(g_wow->device, &edit_box->uniform_buffers[i]);
	gfx_delete_attributes_state(g_wow->device, &edit_box->attributes_state);
	gfx_delete_buffer(g_wow->device, &edit_box->vertexes_buffer);
	gfx_delete_buffer(g_wow->device, &edit_box->indices_buffer);
	ui_object_delete((struct ui_object*)edit_box->font_string);
	char_input_destroy(&edit_box->char_input);
	ui_font_instance_destroy(UI_FONT_INSTANCE);
	ui_frame_vtable.dtr(object);
}

static void load_xml(struct ui_object *object, const struct xml_layout_frame *layout_frame)
{
	ui_frame_vtable.load_xml(object, layout_frame);
	struct ui_edit_box *edit_box = (struct ui_edit_box*)object;
	const struct xml_edit_box *xml_edit_box = xml_clayout_frame_as_edit_box(layout_frame);
	if (edit_box)
	{
		if (xml_edit_box->font)
			edit_box->font = interface_get_font(UI_OBJECT->interface, xml_edit_box->font);
		if (OPTIONAL_ISSET(xml_edit_box->text_insets))
			ui_inset_init_xml(&edit_box->text_insets, &OPTIONAL_GET(xml_edit_box->text_insets));
		if (OPTIONAL_ISSET(xml_edit_box->letters))
			edit_box->char_input.max_bytes = OPTIONAL_GET(xml_edit_box->letters);
		if (OPTIONAL_ISSET(xml_edit_box->blink_speed))
			edit_box->blink_speed = OPTIONAL_GET(xml_edit_box->blink_speed);
		if (OPTIONAL_ISSET(xml_edit_box->numeric))
		{
			if (OPTIONAL_GET(xml_edit_box->numeric))
				edit_box->char_input.flags |= CHAR_INPUT_NUMERIC;
		}
		if (OPTIONAL_ISSET(xml_edit_box->password))
			edit_box->password = OPTIONAL_GET(xml_edit_box->password);
		if (OPTIONAL_ISSET(xml_edit_box->multi_line))
		{
			if (!OPTIONAL_GET(xml_edit_box->multi_line))
				edit_box->char_input.flags |= CHAR_INPUT_INLINE;
		}
		if (OPTIONAL_ISSET(xml_edit_box->history_lines))
			edit_box->history_lines = OPTIONAL_GET(xml_edit_box->history_lines);
		if (OPTIONAL_ISSET(xml_edit_box->auto_focus))
			edit_box->auto_focus = OPTIONAL_ISSET(xml_edit_box->auto_focus);
		if (OPTIONAL_ISSET(xml_edit_box->ignore_arrows))
			edit_box->ignore_arrows = OPTIONAL_GET(xml_edit_box->ignore_arrows);
		if (OPTIONAL_ISSET(xml_edit_box->font_string))
		{
			if (!edit_box->font_string)
			{
				edit_box->font_string = ui_font_string_new(UI_OBJECT->interface, ((struct xml_layout_frame*)&OPTIONAL_GET(xml_edit_box->font_string))->name, UI_REGION);
				if (edit_box->font_string)
				{
					if (!jks_array_push_back(&UI_FRAME->regions_to_load, &edit_box->font_string))
						LOG_ERROR("failed to push region to load");
				}
			}
			ui_object_load_xml((struct ui_object*)edit_box->font_string, (struct xml_layout_frame*)&OPTIONAL_GET(xml_edit_box->font_string));
		}
	}
	load_font_string(edit_box);
}

static void load_font_string(struct ui_edit_box *edit_box)
{
	if (!edit_box->font_string)
	{
		edit_box->font_string = ui_font_string_new(UI_OBJECT->interface, "", (struct ui_region*)edit_box);
		if (!edit_box->font_string)
			return;
	}
	struct ui_dimension dimension;
	ui_dimension_init(&dimension, edit_box->text_insets.abs.left, edit_box->text_insets.abs.top);
	ui_region_add_anchor((struct ui_region*)edit_box->font_string, ui_anchor_new(UI_OBJECT->interface, &dimension, ANCHOR_TOPLEFT, ANCHOR_TOPLEFT, "$parent", (struct ui_region*)edit_box->font_string));
	ui_dimension_init(&dimension, edit_box->text_insets.abs.right, edit_box->text_insets.abs.bottom);
	ui_region_add_anchor((struct ui_region*)edit_box->font_string, ui_anchor_new(UI_OBJECT->interface, &dimension, ANCHOR_BOTTOMRIGHT, ANCHOR_BOTTOMRIGHT, "$parent", (struct ui_region*)edit_box->font_string));
	edit_box->font_string->bypass_size = true;
	if (edit_box->multi_line)
		OPTIONAL_CTR(edit_box->font_string->font_instance.justify_v, JUSTIFYV_TOP);
	OPTIONAL_CTR(edit_box->font_string->font_instance.justify_h, JUSTIFYH_LEFT);
}

static void render(struct ui_object *object)
{
	struct ui_edit_box *edit_box = (struct ui_edit_box*)object;
	ui_frame_vtable.render(object);
	ui_object_render((struct ui_object*)edit_box->font_string);
	if (!edit_box->initialized)
	{
		for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
			gfx_create_buffer(g_wow->device, &edit_box->uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_ui_model_block), GFX_BUFFER_STREAM);
		edit_box->initialized = true;
	}
	if (edit_box->attributes_state.handle.ptr)
	{
		uint16_t count = edit_box->indices;
		if (edit_box == UI_OBJECT->interface->active_input)
		{
			if (UI_REGION->clicked
			 || !(((g_wow->frametime - edit_box->last_action) / 500000000) & 1))
				count += 6;
		}
		if (count)
		{
			gfx_bind_attributes_state(g_wow->device, &edit_box->attributes_state, &UI_OBJECT->interface->input_layout);
			gfx_bind_pipeline_state(g_wow->device, &UI_OBJECT->interface->pipeline_states[INTERFACE_BLEND_ALPHA]);
			const gfx_texture_t *textures = &UI_OBJECT->interface->white_pixel;
			gfx_bind_samplers(g_wow->device, 0, 1, &textures);
			struct shader_ui_model_block model_block;
			VEC4_SET(model_block.color, 1, 1, 1, 1);
			model_block.alpha_test = 0;
			model_block.use_mask = 0;
			struct vec3f tmp =
			{
				(float)(ui_region_get_left((struct ui_region*)edit_box->font_string) + ui_font_string_get_text_left(edit_box->font_string)),
				(float)(ui_region_get_top((struct ui_region*)edit_box->font_string) + ui_font_string_get_text_top(edit_box->font_string)),
				0
			};
			if (!edit_box->char_input.bytes && ui_font_instance_get_justify_v(&edit_box->font_string->font_instance) == JUSTIFYV_MIDDLE)
			{
				struct interface_font *font = ui_font_instance_get_render_font(&edit_box->font_string->font_instance);
				if (font)
					tmp.y -= font->font->height / 2;
			}
			MAT4_TRANSLATE(model_block.mvp, UI_OBJECT->interface->mat, tmp);
			gfx_set_buffer_data(&edit_box->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
			gfx_bind_constant(g_wow->device, 1, &edit_box->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
			gfx_draw_indexed(g_wow->device, count, 0);
		}
	}
}

static void on_click(struct ui_object *object, enum gfx_mouse_button button)
{
	struct ui_edit_box *edit_box = (struct ui_edit_box*)object;
	interface_set_active_input(UI_OBJECT->interface, edit_box);
	ui_frame_vtable.on_click(object, button);
}

static void text_pos_to_coord(struct ui_edit_box *edit_box, uint32_t pos, int32_t *coord_x, int32_t *coord_y)
{
	*coord_x = 0;
	*coord_y = 0;
	if (!pos)
		return;
	if (!edit_box->font_string->text)
		return;
	struct interface_font *font = ui_font_instance_get_render_font(&edit_box->font_string->font_instance);
	if (!font)
		return;
	float spacing = ui_font_instance_get_spacing(&edit_box->font_string->font_instance);
	const char *iter = edit_box->char_input.text;
	const char *end = iter + edit_box->char_input.bytes;
	while (iter < end)
	{
		if (pos <= iter - edit_box->char_input.text)
			return;
		uint32_t character;
		if (!utf8_next(&iter, end, &character))
		{
			*coord_x = 0;
			*coord_y = 0;
			return;
		}
		if (character == '\n')
		{
			*coord_x = 1;
			*coord_y += font->font->height;
			continue;
		}
		struct font_glyph *glyph = font_get_glyph(font->font, edit_box->password ? '*' : character);
		if (!glyph)
			continue;
		float char_width = glyph->advance;
		if (OPTIONAL_ISSET(((struct ui_region*)edit_box->font_string)->size)
		 && OPTIONAL_GET(((struct ui_region*)edit_box->font_string)->size).abs.x
		 && *coord_x + char_width > OPTIONAL_GET(((struct ui_region*)edit_box->font_string)->size).abs.x + 1)
		{
			*coord_x = 1;
			*coord_y += font->font->height;
		}
		*coord_x += char_width;
		*coord_x += spacing;
	}
}

static uint32_t coord_to_text_pos(struct ui_edit_box *edit_box, int32_t coord_x, int32_t coord_y)
{
	if (!edit_box->font_string->text)
		return 0;
	struct interface_font *font = ui_font_instance_get_render_font(&edit_box->font_string->font_instance);
	if (!font)
		return 0;
	coord_x -= ui_region_get_left((struct ui_region*)edit_box->font_string);
	coord_y -= ui_region_get_top((struct ui_region*)edit_box->font_string);
	if ((edit_box->multi_line && coord_y < 0) || coord_x < 0)
		return 0;
	float spacing = ui_font_instance_get_spacing(&edit_box->font_string->font_instance);
	const char *iter = edit_box->char_input.text;
	const char *end = iter + edit_box->char_input.bytes;
	int32_t x = 0;
	int32_t y = 0;
	uint32_t i = 0;
	while (iter < end)
	{
		if ((edit_box->multi_line && coord_y < y) || coord_x <= x)
			return i;
		uint32_t character;
		if (!utf8_next(&iter, end, &character))
			return 0;
		if (character == '\n')
		{
			x = 1;
			y += font->font->height;
			continue;
		}
		struct font_glyph *glyph = font_get_glyph(font->font, edit_box->password ? '*' : character);
		if (!glyph)
			continue;
		float char_width = glyph->advance;
		if (OPTIONAL_ISSET(((struct ui_region*)edit_box->font_string)->size)
		 && OPTIONAL_GET(((struct ui_region*)edit_box->font_string)->size).abs.x
		 && x + char_width > OPTIONAL_GET(((struct ui_region*)edit_box->font_string)->size).abs.x + 1)
		{
			x = 1;
			y += font->font->height;
		}
		if ((edit_box->multi_line && coord_y < y) || coord_x <= x + char_width / 2)
			return i;
		x += char_width;
		x += spacing;
		i++;
	}
	return i;
}

static void on_mouse_move(struct ui_object *object, gfx_pointer_event_t *event)
{
	struct ui_edit_box *edit_box = (struct ui_edit_box*)object;
	if (UI_REGION->clicked)
	{
		uint32_t pos = coord_to_text_pos(edit_box, event->x, event->y);
		int32_t current_selected = edit_box->char_input.selected_position;
		int32_t current_cursor = edit_box->char_input.cursor_position;
		int32_t next_cursor = -(int32_t)edit_box->char_input.bytes + pos;
		int32_t next_selected = current_selected - (next_cursor - current_cursor);
		LOG_INFO("current_selected: %d, current_cursor: %d, next_cursor: %d, next_selected: %d", current_selected, current_cursor, next_cursor, next_selected);
		char_input_set_selected_position(&edit_box->char_input, next_selected);
		char_input_set_cursor_position(&edit_box->char_input, next_cursor);
	}
	ui_frame_vtable.on_mouse_move(object, event);
}

static void on_mouse_down(struct ui_object *object, gfx_mouse_event_t *event)
{
	struct ui_edit_box *edit_box = (struct ui_edit_box*)object;
	ui_frame_vtable.on_mouse_down(object, event);
	if (UI_REGION->clicked)
	{
		uint32_t pos = coord_to_text_pos(edit_box, event->x, event->y);
		char_input_set_selected_position(&edit_box->char_input, 0);
		char_input_set_cursor_position(&edit_box->char_input, -(int32_t)edit_box->char_input.bytes + pos);
		interface_set_active_input(UI_OBJECT->interface, edit_box);
	}
}

static void on_mouse_up(struct ui_object *object, gfx_mouse_event_t *event)
{
	ui_frame_vtable.on_mouse_up(object, event);
}

static bool on_key_down(struct ui_object *object, gfx_key_event_t *event)
{
	struct ui_edit_box *edit_box = (struct ui_edit_box*)object;
	if (event->key == GFX_KEY_TAB)
	{
		if (ui_frame_execute_script(UI_FRAME, "OnTabPressed", 0))
			return true;
	}
	if (event->key == GFX_KEY_ENTER || event->key == GFX_KEY_KP_ENTER)
	{
		if (ui_frame_execute_script(UI_FRAME, "OnEnterPressed", 0))
			return true;
	}
	if (event->key == GFX_KEY_ESCAPE)
	{
		if (ui_frame_execute_script(UI_FRAME, "OnEscapePressed", 0))
			return true;
	}
	if (event->key == GFX_KEY_SPACE)
	{
		if (ui_frame_execute_script(UI_FRAME, "OnSpacePressed", 0))
			return true;
	}
	if (char_input_on_key_press(&edit_box->char_input, event))
	{
		edit_box->last_action = g_wow->frametime;
		return true;
	}
	struct lua_State *L = ui_object_get_L(UI_OBJECT);
	lua_pushstring(L, "key");
	lua_pushstring(L, lua_gfx_key_to_string(event->key));
	if (ui_frame_execute_script(UI_FRAME, "OnKeyDown", 1))
		return true;
	ui_frame_vtable.on_key_down(object, event);
	return true;
}

static bool on_key_up(struct ui_object *object, gfx_key_event_t *event)
{
	struct ui_edit_box *edit_box = (struct ui_edit_box*)object;
	struct lua_State *L = ui_object_get_L(UI_OBJECT);
	lua_pushstring(L, "key");
	lua_pushstring(L, lua_gfx_key_to_string(event->key));
	if (ui_frame_execute_script(UI_FRAME, "OnKeyUp", 1))
		return true;
	ui_frame_vtable.on_key_up(object, event);
	return true;
}

bool ui_edit_box_on_key_press(struct ui_edit_box *edit_box, gfx_key_event_t *event)
{
	if (char_input_on_key_press(&edit_box->char_input, event))
	{
		edit_box->last_action = g_wow->frametime;
		return true;
	}
	return true;
}

bool ui_edit_box_on_char(struct ui_edit_box *edit_box, gfx_char_event_t *event)
{
	struct lua_State *L = ui_object_get_L(UI_OBJECT);
	char_input_insert_char(&edit_box->char_input, event->utf8);
	lua_pushstring(L, "key");
	lua_pushstring(L, event->utf8);
	ui_frame_execute_script(UI_FRAME, "OnChar", 1);
	return true;
}

static void update_overlay_rect(struct jks_array *vertexes, struct jks_array *indices, int32_t x, int32_t y, int32_t width, int32_t height, struct vec4f color)
{
	uint32_t indice_base = vertexes->size;
	struct shader_ui_input *tmp_vertexes = jks_array_grow(vertexes, 4);
	if (!tmp_vertexes)
	{
		LOG_ERROR("failed to grow vertexes");
		return;
	}
	uint16_t *tmp_indices = jks_array_grow(indices, 6);
	if (!tmp_indices)
	{
		jks_array_resize(vertexes, indice_base);
		LOG_ERROR("failed to grow indices");
		return;
	}
	VEC2_SET(tmp_vertexes[0].position, x        , y);
	VEC2_SET(tmp_vertexes[1].position, x + width, y);
	VEC2_SET(tmp_vertexes[2].position, x + width, y + height);
	VEC2_SET(tmp_vertexes[3].position, x        , y + height);
	VEC4_CPY(tmp_vertexes[0].color, color);
	VEC4_CPY(tmp_vertexes[1].color, color);
	VEC4_CPY(tmp_vertexes[2].color, color);
	VEC4_CPY(tmp_vertexes[3].color, color);
	VEC2_SETV(tmp_vertexes[0].uv, 0);
	VEC2_SETV(tmp_vertexes[1].uv, 0);
	VEC2_SETV(tmp_vertexes[2].uv, 0);
	VEC2_SETV(tmp_vertexes[3].uv, 0);
	tmp_indices[0] = indice_base + 0;
	tmp_indices[1] = indice_base + 1;
	tmp_indices[2] = indice_base + 2;
	tmp_indices[3] = indice_base + 0;
	tmp_indices[4] = indice_base + 2;
	tmp_indices[5] = indice_base + 3;
}

static void update_overlay(struct ui_edit_box *edit_box)
{
#define OVERLAY_COLOR (struct vec4f){1, 1, 1, .4}
	struct interface_font *font = ui_font_instance_get_render_font(&edit_box->font_string->font_instance);
	if (!font)
		return;
	struct jks_array vertexes; /* struct shader_ui_input */
	struct jks_array indices; /* uint16_t */
	jks_array_init(&vertexes, sizeof(struct shader_ui_input), NULL, &jks_array_memory_fn_UI);
	jks_array_init(&indices, sizeof(uint16_t), NULL, &jks_array_memory_fn_UI);
	int32_t width = ui_region_get_width((struct ui_region*)edit_box->font_string);
	uint32_t font_height = font->font->height;
	int32_t cursor_x;
	int32_t cursor_y;
	text_pos_to_coord(edit_box, edit_box->char_input.bytes + edit_box->char_input.cursor_position, &cursor_x, &cursor_y);
	if (edit_box->char_input.selected_position)
	{
		int32_t select_x;
		int32_t select_y;
		text_pos_to_coord(edit_box, edit_box->char_input.bytes + edit_box->char_input.cursor_position + edit_box->char_input.selected_position, &select_x, &select_y);
		if (select_y < cursor_y)
		{
			update_overlay_rect(&vertexes, &indices, select_x, select_y, width - select_x, font_height, OVERLAY_COLOR);
			if (cursor_y - select_y > (int32_t)font_height)
				update_overlay_rect(&vertexes, &indices, 0, select_y + font_height, width, cursor_y - select_y - font_height, OVERLAY_COLOR);
			update_overlay_rect(&vertexes, &indices, 0, cursor_y, cursor_x, font_height, OVERLAY_COLOR);
		}
		else if (select_y > cursor_y)
		{
			update_overlay_rect(&vertexes, &indices, cursor_x, cursor_y, width - cursor_x, font_height, OVERLAY_COLOR);
			if (cursor_y - select_y > (int32_t)font_height)
				update_overlay_rect(&vertexes, &indices, 0, cursor_y + font_height, width, select_y - cursor_y - font_height, OVERLAY_COLOR);
			update_overlay_rect(&vertexes, &indices, 0, select_y, select_x, font_height, OVERLAY_COLOR);
		}
		else
		{
			if (select_x < cursor_x)
				update_overlay_rect(&vertexes, &indices, select_x, select_y, cursor_x - select_x, font_height, OVERLAY_COLOR);
			else if (select_x > cursor_x)
				update_overlay_rect(&vertexes, &indices, cursor_x, select_y, select_x - cursor_x, font_height, OVERLAY_COLOR);
		}
	}
	edit_box->indices = indices.size;
	update_overlay_rect(&vertexes, &indices, cursor_x, cursor_y, 4, font_height, (struct vec4f){1, 1, 1, 1});
	gfx_delete_attributes_state(g_wow->device, &edit_box->attributes_state);
	gfx_delete_buffer(g_wow->device, &edit_box->vertexes_buffer);
	gfx_delete_buffer(g_wow->device, &edit_box->indices_buffer);
	gfx_create_buffer(g_wow->device, &edit_box->vertexes_buffer, GFX_BUFFER_VERTEXES, vertexes.data, vertexes.size * sizeof(struct shader_ui_input), GFX_BUFFER_IMMUTABLE);
	gfx_create_buffer(g_wow->device, &edit_box->indices_buffer, GFX_BUFFER_INDICES, indices.data, indices.size * sizeof(uint16_t), GFX_BUFFER_IMMUTABLE);
	gfx_attribute_bind_t binds[] =
	{
		{&edit_box->vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, position)},
		{&edit_box->vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, color)},
		{&edit_box->vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, uv)},
	};
	gfx_create_attributes_state(g_wow->device, &edit_box->attributes_state, binds, sizeof(binds) / sizeof(*binds), &edit_box->indices_buffer, GFX_INDEX_UINT16);
	jks_array_destroy(&vertexes);
	jks_array_destroy(&indices);
#undef OVERLAY_COLOR
}

static void on_selected_position_changed(void *ptr)
{
	struct ui_edit_box *edit_box = ptr;
	update_overlay(edit_box);
}

static void on_cursor_position_changed(void *ptr)
{
	struct ui_edit_box *edit_box = ptr;
	update_overlay(edit_box);
}

static void on_enabled_changed(void *ptr)
{
	struct ui_edit_box *edit_box = ptr;
	if (edit_box->char_input.flags & CHAR_INPUT_ENABLED)
		ui_frame_execute_script(UI_FRAME, "OnEditFocusGained", 0);
	else
		ui_frame_execute_script(UI_FRAME, "OnEditFocusLost", 0);
}

static void on_changed(void *ptr)
{
	struct ui_edit_box *edit_box = ptr;
	if (edit_box->password)
	{
		size_t utf8len = char_input_utf8length(&edit_box->char_input);
		if (utf8len)
		{
			char *tmp = mem_malloc(MEM_UI, utf8len + 1);
			if (!tmp)
			{
				LOG_ERROR("failed to malloc");
				return;
			}
			memset(tmp, '*', utf8len);
			tmp[utf8len] = '\0';
			ui_font_string_set_text(edit_box->font_string, tmp);
			ui_font_string_update_size(edit_box->font_string, ui_font_instance_get_render_font(&edit_box->font_string->font_instance));
			mem_free(MEM_UI, tmp);
		}
		else
		{
			ui_font_string_set_text(edit_box->font_string, "");
			ui_font_string_update_size(edit_box->font_string, ui_font_instance_get_render_font(&edit_box->font_string->font_instance));
		}
	}
	else
	{
		ui_font_string_set_text(edit_box->font_string, edit_box->char_input.text);
		ui_font_string_update_size(edit_box->font_string, ui_font_instance_get_render_font(&edit_box->font_string->font_instance));
	}
	struct lua_State *L = ui_object_get_L(UI_OBJECT);
	lua_pushstring(L, "isUserInput");
	lua_pushboolean(L, true);
	ui_frame_execute_script(UI_FRAME, "OnTextChanged", 1);
	update_overlay(edit_box);
}

static void set_hidden(struct ui_object *object, bool hidden)
{
	struct ui_edit_box *edit_box = (struct ui_edit_box*)object;
	if (UI_REGION->hidden == hidden)
		return;
	if (hidden)
	{
		if (UI_OBJECT->interface->active_input == edit_box)
			interface_set_active_input(UI_OBJECT->interface, NULL);
	}
	ui_frame_vtable.set_hidden(object, hidden);
}

static struct ui_font_instance *as_font_instance(struct ui_object *object)
{
	struct ui_edit_box *edit_box = (struct ui_edit_box*)object;
	return &edit_box->font_instance;
}

static void on_font_height_changed(struct ui_object *object)
{
	(void)object;
	/* XXX */
}

static void on_color_changed(struct ui_object *object)
{
	(void)object;
	/* XXX */
}

static void on_shadow_color_changed(struct ui_object *object)
{
	(void)object;
	/* XXX */
}

static void on_spacing_changed(struct ui_object *object)
{
	(void)object;
	/* XXX */
}

static void on_outline_changed(struct ui_object *object)
{
	(void)object;
	/* XXX */
}

static void on_monochrome_changed(struct ui_object *object)
{
	(void)object;
	/* XXX */
}

static void on_justify_h_changed(struct ui_object *object)
{
	(void)object;
	/* XXX */
}

static void on_justify_v_changed(struct ui_object *object)
{
	(void)object;
	/* XXX */
}

static int lua_GetText(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (edit_box->char_input.flags & CHAR_INPUT_NUMERIC)
		lua_pushnumber(L, edit_box->char_input.text ? atoi(edit_box->char_input.text) : 0);
	else
		lua_pushstring(L, edit_box->char_input.text ? edit_box->char_input.text : "");
	return 1;
}

static int lua_SetText(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 2)
		return luaL_error(L, "Usage: EditBox:SetText(\"text\")");
	const char *str = lua_tostring(L, 2);
	char_input_set_text(&edit_box->char_input, str ? str : "");
	return 0;
}

static int lua_SetFocus(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 1)
		return luaL_error(L, "Usage: EditBox:SetFocus()");
	interface_set_active_input(UI_OBJECT->interface, edit_box);
	return 0;
}

static int lua_GetMaxBytes(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 1)
		return luaL_error(L, "Usage: EditBox:GetMaxBytes()");
	lua_pushnumber(L, edit_box->char_input.max_bytes);
	return 1;
}

static int lua_SetMaxBytes(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 2)
		return luaL_error(L, "Usage: EditBox:SetMaxBytes(maxBytes)");
	if (!lua_isnumber(L, 2))
		return luaL_argerror(L, 2, "number expected");
	edit_box->char_input.max_bytes = lua_tonumber(L, 2);
	return 0;
}

static int lua_GetTextInsets(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 1)
		return luaL_error(L, "Usage: EditBox:GetTextInsets()");
	lua_pushnumber(L, edit_box->text_insets.abs.left);
	lua_pushnumber(L, edit_box->text_insets.abs.right);
	lua_pushnumber(L, edit_box->text_insets.abs.top);
	lua_pushnumber(L, edit_box->text_insets.abs.bottom);
	return 4;
}

static int lua_SetTextInsets(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 5)
		return luaL_error(L, "Usage: EditBox:SetTextInsets(left, right, top, bottom");
	edit_box->text_insets.abs.left = lua_tointeger(L, 2);
	edit_box->text_insets.abs.right = lua_tointeger(L, 3);
	edit_box->text_insets.abs.top = lua_tointeger(L, 4);
	edit_box->text_insets.abs.bottom = lua_tointeger(L, 5);
	ui_region_clear_anchors((struct ui_region*)edit_box->font_string);
	struct ui_dimension dimension;
	ui_dimension_init(&dimension, edit_box->text_insets.abs.left, edit_box->text_insets.abs.top);
	ui_region_add_anchor((struct ui_region*)edit_box->font_string, ui_anchor_new(UI_OBJECT->interface, &dimension, ANCHOR_TOPLEFT, ANCHOR_TOPLEFT, "$parent", (struct ui_region*)edit_box->font_string));
	ui_dimension_init(&dimension, edit_box->text_insets.abs.right, edit_box->text_insets.abs.bottom);
	ui_region_add_anchor((struct ui_region*)edit_box->font_string, ui_anchor_new(UI_OBJECT->interface, &dimension, ANCHOR_BOTTOMRIGHT, ANCHOR_BOTTOMRIGHT, "$parent", (struct ui_region*)edit_box->font_string));
	return 0;
}

static int lua_IsNumeric(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 2)
		return luaL_error(L, "Usage: EditBox:IsNumeric()");
	lua_pushboolean(L, edit_box->char_input.flags & CHAR_INPUT_NUMERIC);
	return 1;
}

static int lua_SetNumeric(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 2)
		return luaL_error(L, "Usage: EditBox:SetNumeric(numeric)");
	if (lua_tointeger(L, 1))
		edit_box->char_input.flags |= CHAR_INPUT_NUMERIC;
	else
		edit_box->char_input.flags &= CHAR_INPUT_NUMERIC;
	return 0;
}

static int lua_IsPassword(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 2)
		return luaL_error(L, "Usage: EditBox:IsPassword()");
	lua_pushboolean(L, edit_box->password);
	return 1;
}

static int lua_SetPassword(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 2)
		return luaL_error(L, "Usage: EditBox:SetPassword(password");
	edit_box->password = lua_tointeger(L, 1);
	return 0;
}

static int lua_IsMultiLine(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 2)
		return luaL_error(L, "Usage: EditBox:IsMultiLine()");
	lua_pushboolean(L, edit_box->multi_line);
	return 1;
}

static int lua_SetMultiLine(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 2)
		return luaL_error(L, "Usage: EditBox:SetMultiLine(multiLine)");
	edit_box->multi_line = lua_tointeger(L, 1);
	return 0;
}

static int lua_IsAutoFocus(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 2)
		return luaL_error(L, "Usage: EditBox:IsAutoFocus()");
	lua_pushboolean(L, edit_box->multi_line);
	return 1;
}

static int lua_SetAutoFocus(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 2)
		return luaL_error(L, "Usage: EditBox:SetAutoFocus(autoFocus)");
	edit_box->multi_line = lua_tointeger(L, 1);
	return 0;
}

static int lua_GetMaxLetters(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 1)
		return luaL_error(L, "Usage: EditBox:GetMaxLetters()");
	lua_pushnumber(L, edit_box->char_input.max_chars);
	return 1;
}

static int lua_SetMaxLetters(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 2)
		return luaL_error(L, "Usage: EditBox:SetMaxLetters(maxLetters)");
	if (!lua_isnumber(L, 2))
		return luaL_argerror(L, 2, "number expected");
	edit_box->char_input.max_chars = lua_tonumber(L, 2);
	return 0;
}

static int lua_GetNumLetters(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 1)
		return luaL_error(L, "Usage: EditBox:GetNumLetters()");
	lua_pushnumber(L, char_input_utf8length(&edit_box->char_input));
	return 1;
}

static int lua_HighlightText(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 1 && argc != 3)
		return luaL_error(L, "Usage: EditBox:HighlightText()");
	LOG_INFO("highlight text %d", argc);
	if (argc == 3)
	{
		int32_t start = lua_tointeger(L, 2);
		int32_t end = lua_tointeger(L, 3);
		char_input_set_cursor_position(&edit_box->char_input, -(int32_t)edit_box->char_input.bytes + start);
		char_input_set_selected_position(&edit_box->char_input, -(int32_t)edit_box->char_input.bytes + end - edit_box->char_input.cursor_position);
	}
	else
	{
		char_input_set_cursor_position(&edit_box->char_input, 0);
		char_input_set_selected_position(&edit_box->char_input, -(ssize_t)edit_box->char_input.bytes);
	}
	return 0;
}

static int lua_GetCursorPosition(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 1)
		return luaL_error(L, "Usage: EditBox:GetCursorPosition()");
	lua_pushinteger(L, edit_box->char_input.bytes + edit_box->char_input.cursor_position);
	return 1;
}

static int lua_SetCursorPosition(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 2)
		return luaL_error(L, "Usage: EditBox:SetCursorPosition(position)");
	char_input_set_cursor_position(&edit_box->char_input, -(int32_t)edit_box->char_input.bytes + lua_tointeger(L, 2));
	return 0;
}

static int lua_ClearFocus(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 1)
		return luaL_error(L, "Usage: EditBox:ClearFocus()");
	if (UI_OBJECT->interface->active_input == edit_box)
		interface_set_active_input(UI_OBJECT->interface, NULL);
	return 0;
}

static int lua_GetNumber(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 1)
		return luaL_error(L, "Usage: EditBox:GetNumber()");
	lua_pushinteger(L, edit_box->char_input.text ? atoi(edit_box->char_input.text) : 0);
	return 1;
}

static int lua_SetNumber(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 2)
		return luaL_error(L, "Usage: EditBox:SetNumber(number)");
	char buf[64];
	snprintf(buf, sizeof(buf), "%f", lua_tonumber(L, 2));
	char_input_set_text(&edit_box->char_input, buf);
	return 0;
}

static int lua_GetInputLanguage(lua_State *L)
{
	LUA_METHOD_EDIT_BOX();
	if (argc != 1)
		return luaL_error(L, "Usage: EditBox:GetInputLanguage()");
	lua_pushstring(L, "ROMAN"); /* CHINESE, JAPANESE, KOREAN, ROMAN */
	return 1;
}

static bool register_methods(struct jks_array *methods)
{
	/*
	   AddHistoryLine
	   GetAltArrowKeyMode
	   GetBlinkSpeed
	   GetHistoryLines
	   Insert
	   SetAltArrowKeyMode
	   SetBlinkSpeed
	   SetHistoryLines
	   ToggleInputLanguage
	 */
	UI_REGISTER_METHOD(GetText);
	UI_REGISTER_METHOD(SetText);
	UI_REGISTER_METHOD(SetFocus);
	UI_REGISTER_METHOD(GetMaxBytes);
	UI_REGISTER_METHOD(SetMaxBytes);
	UI_REGISTER_METHOD(GetTextInsets);
	UI_REGISTER_METHOD(SetTextInsets);
	UI_REGISTER_METHOD(IsNumeric);
	UI_REGISTER_METHOD(SetNumeric);
	UI_REGISTER_METHOD(IsPassword);
	UI_REGISTER_METHOD(SetPassword);
	UI_REGISTER_METHOD(IsMultiLine);
	UI_REGISTER_METHOD(SetMultiLine);
	UI_REGISTER_METHOD(IsAutoFocus);
	UI_REGISTER_METHOD(SetAutoFocus);
	UI_REGISTER_METHOD(GetMaxLetters);
	UI_REGISTER_METHOD(SetMaxLetters);
	UI_REGISTER_METHOD(GetNumLetters);
	UI_REGISTER_METHOD(HighlightText);
	UI_REGISTER_METHOD(GetCursorPosition);
	UI_REGISTER_METHOD(SetCursorPosition);
	UI_REGISTER_METHOD(ClearFocus);
	UI_REGISTER_METHOD(GetNumber);
	UI_REGISTER_METHOD(SetNumber);
	UI_REGISTER_METHOD(GetInputLanguage);
	if (!ui_font_instance_register_methods(methods)) /* FontInstance only fill methods */
		return false;
	return ui_frame_vtable.register_methods(methods);
}

UI_INH0(frame, void, post_load);
UI_INH0(frame, void, register_in_interface);
UI_INH0(frame, void, unregister_in_interface);
UI_INH0(frame, void, eval_name);
UI_INH0(frame, float, get_alpha);
UI_INH1(frame, void, set_alpha, float, alpha);
UI_INH2(frame, void, get_size, int32_t*, x, int32_t*, y);
UI_INH0(frame, void, set_dirty_coords);
UI_INH1(frame, void, on_mouse_scroll, gfx_scroll_event_t*, event);
UI_INH0(frame, const char*, get_name);

const struct ui_object_vtable ui_edit_box_vtable =
{
	UI_OBJECT_VTABLE("EditBox")
};
