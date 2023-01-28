#include "ui/font_string.h"
#include "ui/font.h"

#include "itf/interface.h"
#include "itf/font.h"

#include "xml/font_string.h"

#include "lua/functions.h"

#include "font/font.h"

#include "shaders.h"
#include "wow_lua.h"
#include "memory.h"
#include "cache.h"
#include "log.h"
#include "wow.h"

#include <gfx/device.h>

#include <jks/array.h>
#include <jks/utf8.h>

#include <string.h>

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_FONT_STRING() LUA_METHOD(FontString, font_string)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_LAYERED_REGION->region)
#define UI_LAYERED_REGION (&font_string->layered_region)
#define UI_FONT_INSTANCE (&font_string->font_instance)

#define OUTLINE_OFFSET .25

MEMORY_DECL(UI);

static void update_buffers(struct ui_font_string *font_string, struct interface_font *font);
static void load_render_font(struct ui_font_string *font_string);
static void on_font_height_changed(struct ui_object *object);
static void on_color_changed(struct ui_object *object);
static void on_shadow_changed(struct ui_object *object);
static void on_spacing_changed(struct ui_object *object);
static void on_outline_changed(struct ui_object *object);
static void on_monochrome_changed(struct ui_object *object);
static void on_justify_h_changed(struct ui_object *object);
static void on_justify_v_changed(struct ui_object *object);

static const struct ui_font_instance_callbacks g_font_string_font_instance_callbacks =
{
	.on_font_height_changed = on_font_height_changed,
	.on_color_changed = on_color_changed,
	.on_shadow_changed = on_shadow_changed,
	.on_spacing_changed = on_spacing_changed,
	.on_outline_changed = on_outline_changed,
	.on_monochrome_changed = on_monochrome_changed,
	.on_justify_h_changed = on_justify_h_changed,
	.on_justify_v_changed = on_justify_v_changed,
};

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_layered_region_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_font_string *font_string = (struct ui_font_string*)object;
	UI_OBJECT->mask |= UI_OBJECT_font_string;
	ui_font_instance_init(interface, UI_FONT_INSTANCE, object, &g_font_string_font_instance_callbacks);
	font_string->nonspacewrap = false;
	font_string->max_lines = 0;
	font_string->indented = false;
	font_string->indices_nb = 0;
	font_string->text_width = 0;
	font_string->text_height = 0;
	font_string->initialized = false;
	font_string->dirty_buffers = true;
	font_string->dirty_size = true;
	font_string->last_font = NULL;
	font_string->last_font_revision = 0;
	font_string->text = NULL;
	font_string->bypass_size = false;
	load_render_font(font_string);
	ui_font_string_update_size(font_string, ui_font_instance_get_render_font(UI_FONT_INSTANCE));
	font_string->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		font_string->uniform_buffers[i] = GFX_BUFFER_INIT();
	font_string->vertexes_buffer = GFX_BUFFER_INIT();
	font_string->indices_buffer = GFX_BUFFER_INIT();
	return true;
}

static void dtr(struct ui_object *object)
{
	struct ui_font_string *font_string = (struct ui_font_string*)object;
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_delete_buffer(g_wow->device, &font_string->uniform_buffers[i]);
	gfx_delete_buffer(g_wow->device, &font_string->vertexes_buffer);
	gfx_delete_buffer(g_wow->device, &font_string->indices_buffer);
	gfx_delete_attributes_state(g_wow->device, &font_string->attributes_state);
	mem_free(MEM_UI, font_string->text);
	ui_font_instance_destroy(UI_FONT_INSTANCE);
	ui_layered_region_vtable.dtr(object);
}

static void load_xml(struct ui_object *object, const struct xml_layout_frame *layout_frame)
{
	ui_layered_region_vtable.load_xml(object, layout_frame);
	const struct xml_font_string *xml_font_string = xml_clayout_frame_as_font_string(layout_frame);
	struct ui_font_string *font_string = (struct ui_font_string*)object;
	if (xml_font_string)
	{
		xml_layout_frame_resolve_inherits((struct xml_layout_frame*)layout_frame);
		for (size_t i = 0; i < layout_frame->inherits_vector.size; ++i)
		{
			const char *inherit = *JKS_ARRAY_GET(&layout_frame->inherits_vector, i, const char*);
			struct ui_font *parent = interface_get_font(UI_OBJECT->interface, inherit);
			if (parent)
			{
				if (UI_FONT_INSTANCE->font_instance)
					ui_font_remove_child(UI_FONT_INSTANCE->font_instance, UI_FONT_INSTANCE);
				if (ui_font_add_child(parent, UI_FONT_INSTANCE))
					UI_FONT_INSTANCE->font_instance = parent;
				else
					UI_FONT_INSTANCE->font_instance = NULL;
				break;
			}
			else
			{
				if (!interface_get_virtual_layout_frame(UI_OBJECT->interface, inherit))
					LOG_WARN("undefined font_string inherit: %s", inherit);
			}
		}
		if (OPTIONAL_ISSET(xml_font_string->font_height))
		{
			OPTIONAL_SET(UI_FONT_INSTANCE->font_height);
			ui_value_init_xml(&OPTIONAL_GET(UI_FONT_INSTANCE->font_height), &OPTIONAL_GET(xml_font_string->font_height));
		}
		if (OPTIONAL_ISSET(xml_font_string->color))
		{
			OPTIONAL_SET(UI_FONT_INSTANCE->color);
			ui_color_init_xml(&OPTIONAL_GET(UI_FONT_INSTANCE->color), &OPTIONAL_GET(xml_font_string->color));
		}
		if (OPTIONAL_ISSET(xml_font_string->bytes))
			font_string->bytes = OPTIONAL_GET(xml_font_string->bytes);
		if (xml_font_string->text)
		{
			struct lua_State *L = ui_object_get_L(UI_OBJECT);
			lua_getglobal(L, xml_font_string->text);
			const char *s = lua_tostring(L, -1);
			mem_free(MEM_UI, font_string->text);
			if (s)
				font_string->text = mem_strdup(MEM_UI, s);
			else
				font_string->text = mem_strdup(MEM_UI, xml_font_string->text);
			if (!font_string->text)
				LOG_ERROR("failed to duplicate text");
			font_string->dirty_buffers = true;
			font_string->dirty_size = true;
			lua_pop(L, 1);
		}
		if (OPTIONAL_ISSET(xml_font_string->spacing))
			OPTIONAL_CTR(UI_FONT_INSTANCE->spacing, OPTIONAL_GET(xml_font_string->spacing));
		if (xml_font_string->outline)
		{
			enum outline_type outline;
			if (!outline_from_string(xml_font_string->outline, &outline))
				OPTIONAL_CTR(UI_FONT_INSTANCE->outline, outline);
			else
				LOG_ERROR("invalid outline: %s", xml_font_string->outline);
		}
		if (OPTIONAL_ISSET(xml_font_string->monochrome))
			OPTIONAL_CTR(UI_FONT_INSTANCE->monochrome, OPTIONAL_GET(xml_font_string->monochrome));
		if (OPTIONAL_ISSET(xml_font_string->nonspacewrap))
			font_string->nonspacewrap = OPTIONAL_GET(xml_font_string->nonspacewrap);
		if (xml_font_string->justify_v)
		{
			enum justify_v_type type;
			if (justify_v_from_string(xml_font_string->justify_v, &type))
				OPTIONAL_CTR(UI_FONT_INSTANCE->justify_v, type);
		}
		if (xml_font_string->justify_h)
		{
			enum justify_h_type type;
			if (justify_h_from_string(xml_font_string->justify_h, &type))
				OPTIONAL_CTR(UI_FONT_INSTANCE->justify_h, type);
		}
		if (OPTIONAL_ISSET(xml_font_string->max_lines))
			font_string->max_lines = OPTIONAL_GET(xml_font_string->max_lines);
		if (OPTIONAL_ISSET(xml_font_string->indented))
			font_string->indented = OPTIONAL_GET(xml_font_string->indented);
		ui_font_string_update_size(font_string, ui_font_instance_get_render_font(UI_FONT_INSTANCE));
	}
}

static void load_render_font(struct ui_font_string *font_string)
{
	if (!UI_FONT_INSTANCE->font || !UI_FONT_INSTANCE->font[0])
		return;
	int32_t font_height = 12;
	if (OPTIONAL_ISSET(UI_FONT_INSTANCE->font_height))
	{
		font_height = OPTIONAL_GET(UI_FONT_INSTANCE->font_height).abs;
	}
	else if (UI_FONT_INSTANCE->font_instance)
	{
		const struct ui_value *val = ui_font_instance_get_font_height(&UI_FONT_INSTANCE->font_instance->font_instance);
		font_height = val ? val->abs : 0;
	}
	if (UI_FONT_INSTANCE->render_font)
		interface_unref_render_font(UI_OBJECT->interface, UI_FONT_INSTANCE->render_font);
	UI_FONT_INSTANCE->render_font = interface_ref_render_font(UI_OBJECT->interface, UI_FONT_INSTANCE->font, font_height);
}

static void post_load(struct ui_object *object)
{
	struct ui_font_string *font_string = (struct ui_font_string*)object;
	ui_layered_region_vtable.post_load(object);
	ui_font_string_update_size(font_string, ui_font_instance_get_render_font(UI_FONT_INSTANCE));
}

static void render(struct ui_object *object)
{
	struct ui_font_string *font_string = (struct ui_font_string*)object;
	if (!font_string->text || !font_string->text[0])
		return;
	struct interface_font *font = ui_font_instance_get_render_font(UI_FONT_INSTANCE);
	if (!font)
		return;
	if (!font_string->initialized)
	{
		for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
			gfx_create_buffer(g_wow->device, &font_string->uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_ui_model_block), GFX_BUFFER_STREAM);
		font_string->initialized = true;
	}
	struct shader_ui_model_block model_block;
	VEC4_CPY(model_block.color, UI_LAYERED_REGION->vertex_color);
	model_block.color.w *= ui_object_get_alpha(UI_OBJECT);
	if (model_block.color.w == 0)
		return;
	if (font_string->dirty_buffers)
	{
		update_buffers(font_string, font);
		font_string->last_font_revision = font->atlas->revision;
		font_string->last_font = font;
	}
	if (!font_string->indices_nb)
		return;
	VEC4_SET(model_block.uv_transform, 1, 0, 1, 0);
	font_atlas_update(font->atlas);
	if (font_string->last_font != font || font->atlas->revision != font_string->last_font_revision)
	{
		update_buffers(font_string, font);
		font_string->last_font_revision = font->atlas->revision;
		font_string->last_font = font;
	}
	interface_set_render_ctx(UI_OBJECT->interface, true);
	font_atlas_bind(font->atlas, 0);
	gfx_bind_attributes_state(g_wow->device, &font_string->attributes_state, &UI_OBJECT->interface->input_layout);
	gfx_bind_pipeline_state(g_wow->device, &UI_OBJECT->interface->pipeline_states[INTERFACE_BLEND_ALPHA]);
	model_block.alpha_test = 0;
	model_block.use_mask = 0;
	struct vec3f tmp = {(float)ui_region_get_left(UI_REGION), (float)ui_region_get_top(UI_REGION), 0};
	MAT4_TRANSLATE(model_block.mvp, UI_OBJECT->interface->mat, tmp);
	gfx_set_buffer_data(&font_string->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
	gfx_bind_constant(g_wow->device, 1, &font_string->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	gfx_draw_indexed(g_wow->device, font_string->indices_nb, 0);
	ui_layered_region_vtable.render(object);
}

void ui_font_string_update_size(struct ui_font_string *font_string, struct interface_font *font)
{
	if (!font || !font_string->text || !font_string->text[0])
	{
		font_string->text_width = 0;
		font_string->text_height = 0;
		font_string->dirty_size = false;
		ui_object_set_dirty_coords(UI_OBJECT);
		return;
	}
	font_string->text_width = 0;
	font_string->text_height = font->font->height;
	const char *iter = font_string->text;
	const char *end = iter + strlen(font_string->text);
	int32_t tmp_width = 0;
	float spacing = 0;/* ui_font_instance_get_spacing(UI_FONT_INSTANCE); */
	int32_t max_x = OPTIONAL_ISSET(UI_REGION->size) && OPTIONAL_GET(UI_REGION->size).abs.x ? OPTIONAL_GET(UI_REGION->size).abs.x : INT32_MAX - 1;
	while (iter < end)
	{
		uint32_t character;
		if (!utf8_next(&iter, end, &character))
			goto end;
		if (character == '|')
		{
			uint32_t next_character;
			if (!utf8_next(&iter, end, &next_character))
				break;
			if (next_character == 'c')
			{
				for (int i = 0; i < 8 && iter < end; ++i)
				{
					uint32_t tmp;
					if (!utf8_next(&iter, end, &tmp))
						goto end;
				}
				continue;
			}
			else if (next_character == 'r')
			{
				continue;
			}
			else if (next_character == 'H')
			{
				/* TODO */
				continue;
			}
			else if (next_character == '|')
			{
			}
			else
			{
				uint32_t tmp;
				if (!utf8_prev(&iter, (const char*)font_string->text, &tmp))
					goto end;
			}
		}
		if (character == '\n')
		{
			font_string->text_height += font->font->height;
			if (tmp_width > font_string->text_width)
				font_string->text_width = tmp_width;
			tmp_width = 0;
		}
		struct font_glyph *glyph = font_get_glyph(font->font, character);
		if (!glyph)
			continue;
		float char_width = glyph->advance;
		if (tmp_width + char_width > max_x + 1)
		{
			font_string->text_height += font->font->height;
			if (tmp_width > font_string->text_width)
				font_string->text_width = tmp_width;
			tmp_width = 0;
		}
		tmp_width += char_width;
		tmp_width += spacing;
	}
end:
	if (tmp_width > font_string->text_width)
		font_string->text_width = tmp_width;
	font_string->dirty_size = false;
	ui_object_set_dirty_coords(UI_OBJECT);
}

static inline char get_hex(uint32_t t)
{
	if (t >= '0' && t <= '9')
		return t - '0';
	if (t >= 'A' && t <= 'F')
		return 10 + t - 'A';
	if (t >= 'a' && t <= 'f')
		return 10 + t - 'a';
	return 0;
}

static void update_buffers(struct ui_font_string *font_string, struct interface_font *font)
{
	if (font_string->dirty_size)
		ui_font_string_update_size(font_string, font);
	struct jks_array color_stack; /* struct vec4f */
	struct jks_array vertexes; /* struct shader_ui_input */
	struct jks_array indices; /* uint16_t */
	jks_array_init(&color_stack, sizeof(struct vec4f), NULL, &jks_array_memory_fn_UI);
	jks_array_init(&vertexes, sizeof(struct shader_ui_input), NULL, &jks_array_memory_fn_UI);
	jks_array_init(&indices, sizeof(uint16_t), NULL, &jks_array_memory_fn_UI);
	int32_t base_x = ui_font_string_get_text_left(font_string);
	int32_t base_y = ui_font_string_get_text_top(font_string);
	float spacing = 0;/* ui_font_instance_get_spacing(UI_FONT_INSTANCE); */
	const char *iter = font_string->text;
	const char *end = iter + strlen(font_string->text);
	int32_t x = base_x;
	int32_t y = base_y;
	enum outline_type outline = ui_font_instance_get_outline(UI_FONT_INSTANCE);
	int32_t max_x = base_x + (OPTIONAL_ISSET(UI_REGION->size) && OPTIONAL_GET(UI_REGION->size).abs.x ? OPTIONAL_GET(UI_REGION->size).abs.x : ui_region_get_width(UI_REGION));
	max_x++;
	if (outline == OUTLINE_NONE)
		goto outline_end;
	struct font *outline_font = outline == OUTLINE_THICK ? font->outline_thick : font->outline_normal;
	while (iter < end)
	{
		uint32_t character;
		if (!utf8_next(&iter, end, &character))
			goto shadow_end;
		if (character == '|')
		{
			uint32_t next_character;
			if (!utf8_next(&iter, end, &next_character))
				break;
			if (next_character == 'c')
			{
				for (int i = 0; i < 8 && iter < end; ++i)
				{
					uint32_t tmp;
					if (!utf8_next(&iter, end, &tmp))
						goto shadow_end;
				}
				continue;
			}
			else if (next_character == 'r')
			{
				continue;
			}
			else if (next_character == 'H')
			{
				/* TODO */
				continue;
			}
			else if (next_character == '|')
			{
			}
			else
			{
				uint32_t tmp;
				if (!utf8_prev(&iter, (const char*)font_string->text, &tmp))
					goto shadow_end;
			}
		}
		if (character == '\n')
		{
			x = base_x;
			y += font->font->height;
			continue;
		}
		struct font_glyph *glyph = font_get_glyph(outline_font, character);
		if (!glyph)
			continue;
		float char_width = glyph->advance;
		if (x + char_width > max_x)
		{
			x = base_x;
			y += font->font->height;
		}
		float char_render_left = x + glyph->offset_x + OUTLINE_OFFSET;
		float char_render_top = y + glyph->offset_y + OUTLINE_OFFSET;
		float char_render_right = char_render_left + glyph->width;
		float char_render_bottom = char_render_top + glyph->height;
		{
			uint16_t *tmp = jks_array_grow(&indices, 6);
			if (!tmp)
			{
				LOG_ERROR("failed to grow indices");
				goto err;
			}
			tmp[0] = vertexes.size + 0;
			tmp[1] = vertexes.size + 1;
			tmp[2] = vertexes.size + 2;
			tmp[3] = vertexes.size + 0;
			tmp[4] = vertexes.size + 2;
			tmp[5] = vertexes.size + 3;
		}
		struct shader_ui_input *vertex = jks_array_grow(&vertexes, 4);
		if (!vertex)
		{
			LOG_ERROR("failed to grow vertex");
			goto err;
		}
		{
			struct vec2f tmp[4];
			font_glyph_tex_coords(outline_font, glyph, &tmp[0].x);
			VEC2_CPY(vertex[0].uv, tmp[0]);
			VEC2_CPY(vertex[1].uv, tmp[1]);
			VEC2_CPY(vertex[2].uv, tmp[2]);
			VEC2_CPY(vertex[3].uv, tmp[3]);
		}
		VEC2_SET(vertex[0].position, char_render_left , char_render_top);
		VEC2_SET(vertex[1].position, char_render_right, char_render_top);
		VEC2_SET(vertex[2].position, char_render_right, char_render_bottom);
		VEC2_SET(vertex[3].position, char_render_left , char_render_bottom);
		VEC4_SET(vertex[0].color, 0, 0, 0, 1);
		VEC4_SET(vertex[1].color, 0, 0, 0, 1);
		VEC4_SET(vertex[2].color, 0, 0, 0, 1);
		VEC4_SET(vertex[3].color, 0, 0, 0, 1);
		x += char_width;
		x += spacing;
	}

outline_end:;

	const struct ui_shadow *shadow = ui_font_instance_get_shadow(UI_FONT_INSTANCE);
	if (!shadow)
		goto shadow_end;
	const struct ui_color *shadow_color;
	struct ui_color default_shadow_color;
	if (OPTIONAL_ISSET(shadow->color))
	{
		shadow_color = &OPTIONAL_GET(shadow->color);
	}
	else
	{
		VEC4_SET(default_shadow_color, 0, 0, 0, 1);
		shadow_color = &default_shadow_color;
	}
	const struct ui_dimension *shadow_offset;
	struct ui_dimension default_shadow_offset;
	if (OPTIONAL_ISSET(shadow->offset))
	{
		shadow_offset = &OPTIONAL_GET(shadow->offset);
	}
	else
	{
		ui_dimension_init(&default_shadow_offset, 0, 0);
		shadow_offset = &default_shadow_offset;
	}
	iter = font_string->text;
	end = iter + strlen(font_string->text);
	x = base_x;
	y = base_y;
	while (iter < end)
	{
		uint32_t character;
		if (!utf8_next(&iter, end, &character))
			goto shadow_end;
		if (character == '|')
		{
			uint32_t next_character;
			if (!utf8_next(&iter, end, &next_character))
				break;
			if (next_character == 'c')
			{
				for (int i = 0; i < 8 && iter < end; ++i)
				{
					uint32_t tmp;
					if (!utf8_next(&iter, end, &tmp))
						goto shadow_end;
				}
				continue;
			}
			else if (next_character == 'r')
			{
				continue;
			}
			else if (next_character == 'H')
			{
				/* TODO */
				continue;
			}
			else if (next_character == '|')
			{
			}
			else
			{
				uint32_t tmp;
				if (!utf8_prev(&iter, (const char*)font_string->text, &tmp))
					goto shadow_end;
			}
		}
		if (character == '\n')
		{
			x = base_x;
			y += font->font->height;
			continue;
		}
		struct font_glyph *glyph = font_get_glyph(font->font, character);
		if (!glyph)
			continue;
		float char_width = glyph->advance;
		if (x + char_width > max_x)
		{
			x = base_x;
			y += font->font->height;
		}
		float char_render_left = x + glyph->offset_x + shadow_offset->abs.x;
		float char_render_top = y + glyph->offset_y - shadow_offset->abs.y;
		float char_render_right = char_render_left + glyph->width;
		float char_render_bottom = char_render_top + glyph->height;
		{
			uint16_t *tmp = jks_array_grow(&indices, 6);
			if (!tmp)
			{
				LOG_ERROR("failed to grow indices");
				goto err;
			}
			tmp[0] = vertexes.size + 0;
			tmp[1] = vertexes.size + 1;
			tmp[2] = vertexes.size + 2;
			tmp[3] = vertexes.size + 0;
			tmp[4] = vertexes.size + 2;
			tmp[5] = vertexes.size + 3;
		}
		struct shader_ui_input *vertex = jks_array_grow(&vertexes, 4);
		if (!vertex)
		{
			LOG_ERROR("failed to grow vertex");
			goto err;
		}
		{
			struct vec2f tmp[4];
			font_glyph_tex_coords(font->font, glyph, &tmp[0].x);
			VEC2_CPY(vertex[0].uv, tmp[0]);
			VEC2_CPY(vertex[1].uv, tmp[1]);
			VEC2_CPY(vertex[2].uv, tmp[2]);
			VEC2_CPY(vertex[3].uv, tmp[3]);
		}
		VEC2_SET(vertex[0].position, char_render_left , char_render_top);
		VEC2_SET(vertex[1].position, char_render_right, char_render_top);
		VEC2_SET(vertex[2].position, char_render_right, char_render_bottom);
		VEC2_SET(vertex[3].position, char_render_left , char_render_bottom);
		VEC4_CPY(vertex[0].color, *shadow_color);
		VEC4_CPY(vertex[1].color, *shadow_color);
		VEC4_CPY(vertex[2].color, *shadow_color);
		VEC4_CPY(vertex[3].color, *shadow_color);
		x += char_width;
		x += spacing;
	}

shadow_end:

	iter = font_string->text;
	end = iter + strlen(font_string->text);
	x = base_x;
	y = base_y;
	{
		struct vec4f *tmp = jks_array_grow(&color_stack, 1);
		if (!tmp)
		{
			LOG_ERROR("failed to allocated color stack");
			goto err;
		}
		const struct ui_color *color = ui_font_instance_get_color(UI_FONT_INSTANCE);
		if (color)
			VEC4_CPY(*tmp, *color);
		else
			VEC4_SETV(*tmp, 1);
	}
	while (iter < end)
	{
		uint32_t character;
		if (!utf8_next(&iter, end, &character))
			goto text_end;
		if (character == '|')
		{
			uint32_t next_character;
			if (!utf8_next(&iter, end, &next_character))
				goto text_end;
			if (next_character == 'c')
			{
				uint32_t color_code[8];
				int i;
				for (i = 0; i < 8 && iter < end; ++i)
				{
					if (!utf8_next(&iter, end, &color_code[i]))
						goto text_end;
				}
				if (i != 8)
				{
					LOG_ERROR("invalid color code");
					break;
				}
				struct vec4f *tmp = jks_array_grow(&color_stack, 1);
				if (!tmp)
				{
					LOG_ERROR("failed to allocated color stack");
					goto err;
				}
				VEC4_SET(*tmp,
					((get_hex(color_code[2]) << 4) | get_hex(color_code[3])) / 255.,
					((get_hex(color_code[4]) << 4) | get_hex(color_code[5])) / 255.,
					((get_hex(color_code[6]) << 4) | get_hex(color_code[7])) / 255.,
					((get_hex(color_code[0]) << 4) | get_hex(color_code[1])) / 255.);
				continue;
			}
			else if (next_character == 'r')
			{
				jks_array_resize(&color_stack, color_stack.size - 1);
				if (!color_stack.size)
				{
					struct vec4f *tmp = jks_array_grow(&color_stack, 1);
					if (!tmp)
					{
						LOG_ERROR("failed to allocated color stack");
						goto err;
					}
					VEC4_SETV(*tmp, 1);
				}
				continue;
			}
			else if (next_character == 'H')
			{
				/* TODO */
				continue;
			}
			else if (next_character == '|')
			{
				/* do nothing, '|' will be printed and the second | is already skipped */
			}
			else
			{
				uint32_t tmp;
				if (!utf8_prev(&iter, (const char*)font_string->text, &tmp))
					goto text_end;
			}
		}
		if (character == '\n')
		{
			x = base_x;
			y += font->font->height;
			continue;
		}
		struct font_glyph *glyph = font_get_glyph(font->font, character);
		if (!glyph)
			continue;
		float char_width = glyph->advance;
		if (x + char_width > max_x)
		{
			x = base_x;
			y += font->font->height;
		}
		float char_render_left = x + glyph->offset_x;
		float char_render_top = y + glyph->offset_y;
		float char_render_right = char_render_left + glyph->width;
		float char_render_bottom = char_render_top + glyph->height;
		{
			uint16_t *tmp = jks_array_grow(&indices, 6);
			if (!tmp)
			{
				LOG_ERROR("failed to grow indices");
				goto err;
			}
			tmp[0] = vertexes.size + 0;
			tmp[1] = vertexes.size + 1;
			tmp[2] = vertexes.size + 2;
			tmp[3] = vertexes.size + 0;
			tmp[4] = vertexes.size + 2;
			tmp[5] = vertexes.size + 3;
		}
		struct shader_ui_input *vertex = jks_array_grow(&vertexes, 4);
		if (!vertex)
		{
			LOG_ERROR("failed to grow vertexes");
			goto err;
		}
		{
			struct vec2f tmp[4];
			font_glyph_tex_coords(font->font, glyph, &tmp[0].x);
			VEC2_CPY(vertex[0].uv, tmp[0]);
			VEC2_CPY(vertex[1].uv, tmp[1]);
			VEC2_CPY(vertex[2].uv, tmp[2]);
			VEC2_CPY(vertex[3].uv, tmp[3]);
		}
		VEC2_SET(vertex[0].position, char_render_left , char_render_top);
		VEC2_SET(vertex[1].position, char_render_right, char_render_top);
		VEC2_SET(vertex[2].position, char_render_right, char_render_bottom);
		VEC2_SET(vertex[3].position, char_render_left , char_render_bottom);
		struct vec4f *tmp2 = JKS_ARRAY_GET(&color_stack, color_stack.size - 1, struct vec4f);
		VEC4_CPY(vertex[0].color, *tmp2);
		VEC4_CPY(vertex[1].color, *tmp2);
		VEC4_CPY(vertex[2].color, *tmp2);
		VEC4_CPY(vertex[3].color, *tmp2);
		x += char_width;
		x += spacing;
	}
text_end:
	/* XXX: don't recreate if indices nb didn't changed too much (maybe allocate something like npot(indices_nb)) ? */
	if (indices.size != font_string->indices_nb)
	{
		font_string->indices_nb = indices.size;
		gfx_delete_buffer(g_wow->device, &font_string->vertexes_buffer);
		gfx_delete_buffer(g_wow->device, &font_string->indices_buffer);
		gfx_delete_attributes_state(g_wow->device, &font_string->attributes_state);
		gfx_create_buffer(g_wow->device, &font_string->vertexes_buffer, GFX_BUFFER_VERTEXES, vertexes.data, vertexes.size * sizeof(struct shader_ui_input), GFX_BUFFER_STATIC);
		gfx_create_buffer(g_wow->device, &font_string->indices_buffer, GFX_BUFFER_INDICES, indices.data, indices.size * sizeof(uint16_t), GFX_BUFFER_STATIC);
		gfx_attribute_bind_t binds[] =
		{
			{&font_string->vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, position)},
			{&font_string->vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, color)},
			{&font_string->vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, uv)},
		};
		gfx_create_attributes_state(g_wow->device, &font_string->attributes_state, binds, sizeof(binds) / sizeof(*binds), &font_string->indices_buffer, GFX_INDEX_UINT16);
	}
	else
	{
		gfx_set_buffer_data(&font_string->vertexes_buffer, vertexes.data, vertexes.size * sizeof(struct shader_ui_input), 0);
	}
err:
	jks_array_destroy(&color_stack);
	jks_array_destroy(&vertexes);
	jks_array_destroy(&indices);
	font_string->dirty_buffers = false;
}

static void get_size(struct ui_object *object, int32_t *x, int32_t *y)
{
	struct ui_font_string *font_string = (struct ui_font_string*)object;
	if (font_string->bypass_size)
	{
		ui_layered_region_vtable.get_size(object, x, y);
		return;
	}
	if (x)
	{
		if (OPTIONAL_ISSET(UI_REGION->size) && OPTIONAL_GET(UI_REGION->size).abs.x)
			*x = OPTIONAL_GET(UI_REGION->size).abs.x;
		else
			*x = ui_font_string_get_text_width(font_string);
	}
	if (y)
	{
		if (OPTIONAL_ISSET(UI_REGION->size) && OPTIONAL_GET(UI_REGION->size).abs.y)
			*y = OPTIONAL_GET(UI_REGION->size).abs.y;
		else
			*y = ui_font_string_get_text_height(font_string);
	}
}

void ui_font_string_set_font_instance(struct ui_font_string *font_string, struct ui_font *font)
{
	if (UI_FONT_INSTANCE->font_instance == font)
		return;
	if (font)
	{
		if (!ui_font_add_child(font, UI_FONT_INSTANCE))
		{
			LOG_ERROR("can't add font_instance to font childs");
			return;
		}
	}
	if (UI_FONT_INSTANCE->font_instance)
		ui_font_remove_child(UI_FONT_INSTANCE->font_instance, UI_FONT_INSTANCE);
	UI_FONT_INSTANCE->font_instance = font;
	font_string->dirty_buffers = true;
	UI_REGION->dirty_coords = true;
	font_string->dirty_size = true;
}

void ui_font_string_set_text(struct ui_font_string *font_string, const char *text)
{
	if (font_string->text == text || (text && font_string->text && !strcmp(font_string->text, text)))
		return;
	char *tmp = mem_strdup(MEM_UI, text);
	if (!tmp)
	{
		LOG_ERROR("string allocation failed");
		return;
	}
	mem_free(MEM_UI, font_string->text);
	font_string->text = tmp;
	font_string->dirty_buffers = true;
	UI_REGION->dirty_coords = true;
	font_string->dirty_size = true;
}

int32_t ui_font_string_get_text_left(struct ui_font_string *font_string)
{
	enum justify_h_type justify_h = ui_font_instance_get_justify_h(UI_FONT_INSTANCE);
	switch (justify_h)
	{
		case JUSTIFYH_LEFT:
		default:
			return 0;
		case JUSTIFYH_CENTER:
			return (ui_region_get_width(UI_REGION) - font_string->text_width) / 2;
		case JUSTIFYH_RIGHT:
			return ui_region_get_width(UI_REGION) - font_string->text_width;
	}
	return 0;
}

int32_t ui_font_string_get_text_top(struct ui_font_string *font_string)
{
	enum justify_v_type justify_v = ui_font_instance_get_justify_v(UI_FONT_INSTANCE);
	switch (justify_v)
	{
		case JUSTIFYV_TOP:
			return  0;
		case JUSTIFYV_MIDDLE:
		default:
			return (ui_region_get_height(UI_REGION) - font_string->text_height) / 2;
		case JUSTIFYV_BOTTOM:
			return ui_region_get_height(UI_REGION) - font_string->text_height;
	}
}

int32_t ui_font_string_get_text_width(struct ui_font_string *font_string)
{
	if (font_string->dirty_size)
		ui_font_string_update_size((struct ui_font_string*)font_string, ui_font_instance_get_render_font(UI_FONT_INSTANCE));
	return font_string->text_width;
}

int32_t ui_font_string_get_text_height(struct ui_font_string *font_string)
{
	if (font_string->dirty_size)
		ui_font_string_update_size((struct ui_font_string*)font_string, ui_font_instance_get_render_font(UI_FONT_INSTANCE));
	return font_string->text_height;
}

static struct ui_font_instance *as_font_instance(struct ui_object *object)
{
	struct ui_font_string *font_string = (struct ui_font_string*)object;
	return UI_FONT_INSTANCE;
}

static void on_font_height_changed(struct ui_object *object)
{
	struct ui_font_string *font_string = (struct ui_font_string*)object;
	font_string->dirty_size = true;
	font_string->dirty_buffers = true;
}

static void on_color_changed(struct ui_object *object)
{
	struct ui_font_string *font_string = (struct ui_font_string*)object;
	font_string->dirty_buffers = true;
}

static void on_shadow_changed(struct ui_object *object)
{
	struct ui_font_string *font_string = (struct ui_font_string*)object;
	font_string->dirty_buffers = true;
}

static void on_spacing_changed(struct ui_object *object)
{
	struct ui_font_string *font_string = (struct ui_font_string*)object;
	font_string->dirty_size = true;
	font_string->dirty_buffers = true;
}

static void on_outline_changed(struct ui_object *object)
{
	struct ui_font_string *font_string = (struct ui_font_string*)object;
	font_string->dirty_buffers = true;
}

static void on_monochrome_changed(struct ui_object *object)
{
	struct ui_font_string *font_string = (struct ui_font_string*)object;
	font_string->dirty_buffers = true;
}

static void on_justify_h_changed(struct ui_object *object)
{
	struct ui_font_string *font_string = (struct ui_font_string*)object;
	font_string->dirty_size = true;
	font_string->dirty_buffers = true;
}

static void on_justify_v_changed(struct ui_object *object)
{
	struct ui_font_string *font_string = (struct ui_font_string*)object;
	font_string->dirty_size = true;
	font_string->dirty_buffers = true;
}

static int lua_GetText(lua_State *L)
{
	LUA_METHOD_FONT_STRING();
	if (argc != 1)
		return luaL_error(L, "Usage: FontString:GetText()");
	lua_pushstring(L, font_string->text);
	return 1;
}

static int lua_SetText(lua_State *L)
{
	LUA_METHOD_FONT_STRING();
	if (argc == 1)
	{
		ui_font_string_set_text(font_string, "");
		return 0;
	}
	if (argc < 1)
		return luaL_error(L, "Usage: FontString:SetText(\"text\")");
	const char *str = lua_tostring(L, 2);
	ui_font_string_set_text(font_string, str ? str : "");
	return 0;
}

static int lua_SetFormattedText(lua_State *L)
{
	LUA_METHOD_FONT_STRING();
	if (argc < 1)
		return luaL_error(L, "Usage: FontString:SetFormattedText(\"format\", ...)");
	char buffer[4096];
	lua_format(L, 1, buffer, sizeof(buffer));
	ui_font_string_set_text(font_string, buffer);
	return 0;
}

static int lua_GetStringWidth(lua_State *L)
{
	LUA_METHOD_FONT_STRING();
	if (argc != 1)
		return luaL_error(L, "Usage: FontString:GetStringWidth()");
	lua_pushnumber(L, ui_font_string_get_text_width(font_string));
	return 1;
}

static int lua_GetStringHeight(lua_State *L)
{
	LUA_METHOD_FONT_STRING();
	if (argc != 1)
		return luaL_error(L, "Usage: FontString:GetStringHeight()");
	lua_pushnumber(L, ui_font_string_get_text_height(font_string));
	return 1;
}

static int lua_SetTextHeight(lua_State *L)
{
	LUA_METHOD_FONT_STRING();
	if (argc != 2)
		return luaL_error(L, "Usage: FontString:SetTextHeight(height)");
	LUA_UNIMPLEMENTED_METHOD();
	/* may distord text, as if scale = param / font->getLineHeight() */
	return 0;
}

static bool register_methods(struct jks_array *methods)
{
	/*
	   CanNonSpaceWrap
	   SetAlphaGradient
	   SetNonSpaceWrap
	 */
	UI_REGISTER_METHOD(GetText);
	UI_REGISTER_METHOD(SetText);
	UI_REGISTER_METHOD(SetFormattedText);
	UI_REGISTER_METHOD(GetStringWidth);
	UI_REGISTER_METHOD(GetStringHeight);
	UI_REGISTER_METHOD(SetTextHeight);
	if (!ui_font_instance_register_methods(methods)) /* FontInstance only fill methods */
		return false;
	return ui_layered_region_vtable.register_methods(methods);
}

UI_INH0(layered_region, void, register_in_interface);
UI_INH0(layered_region, void, unregister_in_interface);
UI_INH0(layered_region, void, eval_name);
UI_INH1(layered_region, void, on_click, enum gfx_mouse_button, button);
UI_INH0(layered_region, float, get_alpha);
UI_INH1(layered_region, void, set_alpha, float, alpha);
UI_INH1(layered_region, void, set_hidden, bool, hidden);
UI_INH0(layered_region, void, set_dirty_coords);
UI_INH1(layered_region, void, on_mouse_move, gfx_pointer_event_t*, event);
UI_INH1(layered_region, void, on_mouse_down, gfx_mouse_event_t*, event);
UI_INH1(layered_region, void, on_mouse_up, gfx_mouse_event_t*, event);
UI_INH1(layered_region, void, on_mouse_scroll, gfx_scroll_event_t*, event);
UI_INH1(layered_region, bool, on_key_down, gfx_key_event_t*, event);
UI_INH1(layered_region, bool, on_key_up, gfx_key_event_t*, event);
UI_INH0(layered_region, const char*, get_name);

const struct ui_object_vtable ui_font_string_vtable =
{
	UI_OBJECT_VTABLE("FontString")
};
