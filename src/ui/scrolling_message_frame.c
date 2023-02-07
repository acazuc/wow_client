#include "ui/scrolling_message_frame.h"
#include "ui/font_string.h"

#include "itf/interface.h"
#include "itf/font.h"

#include "font/font.h"

#include "xml/scrolling_message_frame.h"

#include "wow_lua.h"
#include "shaders.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <gfx/objects.h>
#include <gfx/device.h>

#include <jks/utf8.h>

#include <string.h>

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_SCROLLING_MESSAGE_FRAME() LUA_METHOD(ScrollingMessageFrame, scrolling_message_frame)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_FRAME->region)
#define UI_FRAME (&scrolling_message_frame->frame)
#define UI_FONT_INSTANCE (&scrolling_message_frame->font_instance)

#define OUTLINE_OFFSET .5

struct ui_scrolling_message
{
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t vertexes_buffer;
	gfx_buffer_t indices_buffer;
	char *text;
	float red;
	float green;
	float blue;
	int32_t height;
	size_t indices_nb;
	bool initialized;
	bool dirty_buffers;
	struct interface_font *last_font; /* XXX should be in scrolling_message_frame ? */
	uint32_t last_font_revision;
	uint32_t id;
};

static void update_buffers(struct ui_scrolling_message_frame *scrolling_message_frame, struct ui_scrolling_message *message, struct interface_font *font);
static bool add_message(struct ui_scrolling_message_frame *scrolling_message_frame, const char *text, float r, float g, float b, int32_t id);
static void on_font_height_changed(struct ui_object *object);
static void on_color_changed(struct ui_object *object);
static void on_shadow_changed(struct ui_object *object);
static void on_spacing_changed(struct ui_object *object);
static void on_outline_changed(struct ui_object *object);
static void on_monochrome_changed(struct ui_object *object);
static void on_justify_h_changed(struct ui_object *object);
static void on_justify_v_changed(struct ui_object *object);

MEMORY_DECL(UI);

static const struct ui_font_instance_callbacks g_scrolling_message_frame_font_instance_callbacks =
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

static void message_dtr(void *ptr)
{
	struct ui_scrolling_message *message = *(struct ui_scrolling_message**)ptr;
	gfx_delete_attributes_state(g_wow->device, &message->attributes_state);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_delete_buffer(g_wow->device, &message->uniform_buffers[i]);
	gfx_delete_buffer(g_wow->device, &message->vertexes_buffer);
	gfx_delete_buffer(g_wow->device, &message->indices_buffer);
	mem_free(MEM_UI, message->text);
	mem_free(MEM_UI, message);
}

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_frame_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_scrolling_message_frame *scrolling_message_frame = (struct ui_scrolling_message_frame*)object;
	UI_OBJECT->mask |= UI_OBJECT_scrolling_message_frame;
	ui_font_instance_init(interface, UI_FONT_INSTANCE, object, &g_scrolling_message_frame_font_instance_callbacks);
	jks_array_init(&scrolling_message_frame->messages, sizeof(struct ui_scrolling_message*), message_dtr, &jks_array_memory_fn_UI);
	ui_inset_init(&scrolling_message_frame->text_insets, 0, 0, 0, 0);
	scrolling_message_frame->font_string = NULL;
	add_message(scrolling_message_frame, "ouioui1", 1, .5, .5, 1);
	add_message(scrolling_message_frame, "ouioui2", 1, .3, .8, 1);
	add_message(scrolling_message_frame, "ouioui3", 1, .1, .1, 1);
	add_message(scrolling_message_frame, "ouioui4", 1, .2, .6, 1);
	return true;
}

static void dtr(struct ui_object *object)
{
	struct ui_scrolling_message_frame *scrolling_message_frame = (struct ui_scrolling_message_frame*)object;
	jks_array_destroy(&scrolling_message_frame->messages);
	ui_font_instance_destroy(UI_FONT_INSTANCE);
	ui_object_delete((struct ui_object*)scrolling_message_frame->font_string);
	ui_frame_vtable.dtr(object);
}

static void load_xml(struct ui_object *object, const struct xml_layout_frame *layout_frame)
{
	ui_frame_vtable.load_xml(object, layout_frame);
	struct ui_scrolling_message_frame *scrolling_message_frame = (struct ui_scrolling_message_frame*)object;
	const struct xml_scrolling_message_frame *xml_scrolling_message_frame = xml_clayout_frame_as_scrolling_message_frame(layout_frame);
	if (xml_scrolling_message_frame)
	{
		if (OPTIONAL_ISSET(xml_scrolling_message_frame->text_insets))
			ui_inset_init_xml(&scrolling_message_frame->text_insets, &OPTIONAL_GET(xml_scrolling_message_frame->text_insets));
		if (OPTIONAL_ISSET(xml_scrolling_message_frame->font_string))
		{
			if (!scrolling_message_frame->font_string)
			{
				scrolling_message_frame->font_string = ui_font_string_new(UI_OBJECT->interface, ((struct xml_layout_frame*)&OPTIONAL_GET(xml_scrolling_message_frame->font_string))->name, UI_REGION);
				if (scrolling_message_frame->font_string)
				{
					scrolling_message_frame->font_string->layered_region.draw_layer = DRAWLAYER_OVERLAY;
					if (!jks_array_push_back(&UI_FRAME->regions_to_load, &scrolling_message_frame->font_string))
						LOG_ERROR("failed to add region to load");
					if (!jks_array_push_back(&UI_FRAME->layers[DRAWLAYER_OVERLAY], &scrolling_message_frame->font_string))
						LOG_ERROR("failed to add region to layer");
				}
			}
			ui_object_load_xml((struct ui_object*)scrolling_message_frame->font_string, (struct xml_layout_frame*)&OPTIONAL_GET(xml_scrolling_message_frame->font_string));
		}
	}
}

static void post_load(struct ui_object *object)
{
	struct ui_scrolling_message_frame *scrolling_message_frame = (struct ui_scrolling_message_frame*)object;
	if (scrolling_message_frame->font_string)
		UI_FONT_INSTANCE->font_instance = scrolling_message_frame->font_string->font_instance.font_instance;
	if (!UI_FONT_INSTANCE->font_instance)
	{
		OPTIONAL_SET(UI_FONT_INSTANCE->font_height);
		ui_value_init(&OPTIONAL_GET(UI_FONT_INSTANCE->font_height), 9);
		UI_FONT_INSTANCE->font = strdup("Fonts\\arialn.TTF");
		UI_FONT_INSTANCE->render_font = interface_ref_render_font(UI_OBJECT->interface, UI_FONT_INSTANCE->font, OPTIONAL_GET(UI_FONT_INSTANCE->font_height).abs);
	}
	ui_frame_vtable.post_load(object);
}

static void render(struct ui_object *object)
{
	ui_frame_vtable.render(object);
	struct ui_scrolling_message_frame *scrolling_message_frame = (struct ui_scrolling_message_frame*)object;
	struct interface_font *font = ui_font_instance_get_render_font(UI_FONT_INSTANCE);
	if (!font)
		return;
	int32_t scissor_left = (ui_region_get_left(UI_REGION) + scrolling_message_frame->text_insets.abs.left) * (g_wow->render_width / (float)UI_OBJECT->interface->width);
	int32_t scissor_top = (ui_region_get_top(UI_REGION) + scrolling_message_frame->text_insets.abs.top) * (g_wow->render_height / (float)UI_OBJECT->interface->height);
	int32_t scissor_width = (ui_region_get_width(UI_REGION) - scrolling_message_frame->text_insets.abs.left - scrolling_message_frame->text_insets.abs.right) * (g_wow->render_width / (float)UI_OBJECT->interface->width);
	int32_t scissor_height = (ui_region_get_height(UI_REGION) - scrolling_message_frame->text_insets.abs.top - scrolling_message_frame->text_insets.abs.bottom) * (g_wow->render_height / (float)UI_OBJECT->interface->height);
	interface_enable_scissor(scissor_left, scissor_top, scissor_width, scissor_height);
	int32_t y = 0;
	for (size_t i = scrolling_message_frame->messages.size; i > 0; --i)
	{
		struct ui_scrolling_message *message = *JKS_ARRAY_GET(&scrolling_message_frame->messages, i - 1, struct ui_scrolling_message*);
		if (!message->initialized)
		{
			for (size_t j = 0; j < RENDER_FRAMES_COUNT; ++j)
				gfx_create_buffer(g_wow->device, &message->uniform_buffers[j], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_ui_model_block), GFX_BUFFER_STREAM);
			message->initialized = true;
		}
		if (message->dirty_buffers)
		{
			update_buffers(scrolling_message_frame, message, font);
			message->last_font_revision = font->atlas->revision;
			message->last_font = font;
		}
		if (!message->indices_nb)
			break;
		struct shader_ui_model_block model_block;
		VEC4_SET(model_block.color, 1, 1, 1, 1);
		VEC4_SET(model_block.uv_transform, 1, 0, 1, 0);
		font_atlas_update(font->atlas);
		if (message->last_font != font || font->atlas->revision != message->last_font_revision)
		{
			update_buffers(scrolling_message_frame, message, font);
			message->last_font_revision = font->atlas->revision;
			message->last_font = font;
		}
		interface_set_render_ctx(UI_OBJECT->interface, true);
		font_atlas_bind(font->atlas, 0);
		gfx_bind_attributes_state(g_wow->device, &message->attributes_state, &UI_OBJECT->interface->input_layout);
		gfx_bind_pipeline_state(g_wow->device, &UI_OBJECT->interface->pipeline_states[INTERFACE_BLEND_ALPHA]);
		model_block.alpha_test = 0;
		model_block.use_mask = 0;
		y -= message->height;
		struct vec3f tmp = {(float)ui_region_get_left(UI_REGION), (float)ui_region_get_bottom(UI_REGION) + y, 0};
		MAT4_TRANSLATE(model_block.mvp, UI_OBJECT->interface->mat, tmp);
		gfx_set_buffer_data(&message->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
		gfx_bind_constant(g_wow->device, 1, &message->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
		gfx_draw_indexed(g_wow->device, message->indices_nb, 0);
	}
	interface_disable_scissor();
}

static void on_click(struct ui_object *object, enum gfx_mouse_button button)
{
	ui_frame_vtable.on_click(object, button);
}

static struct ui_font_instance *as_font_instance(struct ui_object *object)
{
	struct ui_scrolling_message_frame *scrolling_message_frame = (struct ui_scrolling_message_frame*)object;
	return &scrolling_message_frame->font_instance;
}

static void update_buffers(struct ui_scrolling_message_frame *scrolling_message_frame, struct ui_scrolling_message *message, struct interface_font *font)
{
	struct jks_array vertexes; /* struct shader_ui_input */
	struct jks_array indices; /* uint16_t */
	jks_array_init(&vertexes, sizeof(struct shader_ui_input), NULL, &jks_array_memory_fn_UI);
	jks_array_init(&indices, sizeof(uint16_t), NULL, &jks_array_memory_fn_UI);
	int32_t x = 0;
	int32_t y = 0;
	const char *iter = message->text;
	const char *end = iter + strlen(message->text);
	int32_t max_x = (OPTIONAL_ISSET(UI_REGION->size) && OPTIONAL_GET(UI_REGION->size).abs.x ? OPTIONAL_GET(UI_REGION->size).abs.x : ui_region_get_width(UI_REGION));
	enum outline_type outline = ui_font_instance_get_outline(UI_FONT_INSTANCE);
	if (outline == OUTLINE_NONE)
		goto outline_end;
	struct font *outline_font = outline == OUTLINE_THICK ? font->outline_thick : font->outline_normal;
	while (iter < end)
	{
		uint32_t character;
		if (!utf8_next(&iter, end, &character))
			break;
		struct font_glyph *glyph = font_get_glyph(outline_font, character);
		if (!glyph)
			continue;
		float char_width = glyph->advance;
		if (x + char_width > max_x)
		{
			x = 0;
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
	iter = message->text;
	end = iter + strlen(message->text);
	x = 0;
	y = 0;
	while (iter < end)
	{
		uint32_t character;
		if (!utf8_next(&iter, end, &character))
			break;
		struct font_glyph *glyph = font_get_glyph(font->font, character);
		if (!glyph)
			continue;
		float char_width = glyph->advance;
		if (x + char_width > max_x)
		{
			x = 0;
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
	}

shadow_end:

	iter = message->text;
	end = iter + strlen(message->text);
	x = 0;
	y = 0;
	while (iter < end)
	{
		uint32_t character;
		if (!utf8_next(&iter, end, &character))
			break;
		struct font_glyph *glyph = font_get_glyph(font->font, character);
		if (!glyph)
			continue;
		float char_width = glyph->advance;
		if (x + char_width > max_x)
		{
			x = 0;
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
		VEC4_SET(vertex[0].color, message->red, message->green, message->blue, 1);
		VEC4_SET(vertex[1].color, message->red, message->green, message->blue, 1);
		VEC4_SET(vertex[2].color, message->red, message->green, message->blue, 1);
		VEC4_SET(vertex[3].color, message->red, message->green, message->blue, 1);
		x += char_width;
	}
	message->height = y + font->font->height;
	message->indices_nb = indices.size;
	gfx_delete_buffer(g_wow->device, &message->vertexes_buffer);
	gfx_delete_buffer(g_wow->device, &message->indices_buffer);
	gfx_delete_attributes_state(g_wow->device, &message->attributes_state);
	gfx_create_buffer(g_wow->device, &message->vertexes_buffer, GFX_BUFFER_VERTEXES, vertexes.data, vertexes.size * sizeof(struct shader_ui_input), GFX_BUFFER_STATIC);
	gfx_create_buffer(g_wow->device, &message->indices_buffer, GFX_BUFFER_INDICES, indices.data, indices.size * sizeof(uint16_t), GFX_BUFFER_STATIC);
	gfx_attribute_bind_t binds[] =
	{
		{&message->vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, position)},
		{&message->vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, color)},
		{&message->vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, uv)},
	};
	gfx_create_attributes_state(g_wow->device, &message->attributes_state, binds, sizeof(binds) / sizeof(*binds), &message->indices_buffer, GFX_INDEX_UINT16);

err:
	jks_array_destroy(&vertexes);
	jks_array_destroy(&indices);
	message->dirty_buffers = false;
}

static bool add_message(struct ui_scrolling_message_frame *scrolling_message_frame, const char *text, float r, float g, float b, int32_t id)
{
	struct ui_scrolling_message **messagep = jks_array_grow(&scrolling_message_frame->messages, 1);
	if (!messagep)
	{
		LOG_ERROR("failed to grow scrolling message frame messages");
		return false;
	}
	struct ui_scrolling_message *message = mem_malloc(MEM_UI, sizeof(*message));
	if (!message)
	{
		LOG_ERROR("scrolling message allocation failed");
		jks_array_resize(&scrolling_message_frame->messages, scrolling_message_frame->messages.size - 1);
		return false;
	}
	message->text = mem_strdup(MEM_UI, text);
	if (!message->text)
	{
		LOG_ERROR("scrolling message text allocation failed");
		mem_free(MEM_UI, message);
		jks_array_resize(&scrolling_message_frame->messages, scrolling_message_frame->messages.size - 1);
		return false;
	}
	message->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		message->uniform_buffers[i] = GFX_BUFFER_INIT();
	message->vertexes_buffer = GFX_BUFFER_INIT();
	message->indices_buffer = GFX_BUFFER_INIT();
	message->red = r;
	message->green = g;
	message->blue = b;
	message->initialized = false;
	message->dirty_buffers = true;
	message->last_font = NULL;
	message->last_font_revision = 0;
	message->id = id;
	*messagep = message;
	return true;
}

static void on_font_height_changed(struct ui_object *object)
{
	(void)object;
}

static void on_color_changed(struct ui_object *object)
{
	(void)object;
}

static void on_shadow_changed(struct ui_object *object)
{
	(void)object;
}

static void on_spacing_changed(struct ui_object *object)
{
	(void)object;
}

static void on_outline_changed(struct ui_object *object)
{
	(void)object;
}

static void on_monochrome_changed(struct ui_object *object)
{
	(void)object;
}

static void on_justify_h_changed(struct ui_object *object)
{
	(void)object;
}

static void on_justify_v_changed(struct ui_object *object)
{
	(void)object;
}

static int lua_AtBottom(lua_State *L)
{
	LUA_METHOD_SCROLLING_MESSAGE_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: ScrollingMessageFrame:AtBottom()");
	LUA_UNIMPLEMENTED_METHOD();
	lua_pushboolean(L, true);
	return 1;
}

static int lua_ScrollDown(lua_State *L)
{
	LUA_METHOD_SCROLLING_MESSAGE_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: ScrollingMessageFrame:ScrollDown()");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_ScrollUp(lua_State *L)
{
	LUA_METHOD_SCROLLING_MESSAGE_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: ScrollingMessageFrame:ScrollUp()");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_ScrollToTop(lua_State *L)
{
	LUA_METHOD_SCROLLING_MESSAGE_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: ScrollingMessageFrame:ScrollToTop()");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_ScrollToBottom(lua_State *L)
{
	LUA_METHOD_SCROLLING_MESSAGE_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: ScrollingMessageFrame:ScrollToBottom()");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_AddMessage(lua_State *L)
{
	LUA_METHOD_SCROLLING_MESSAGE_FRAME();
	if (argc != 6)
		return luaL_error(L, "Usage: ScrollingMessageFrame:AddMessage(text, red, green, blue, id)");
	const char *str = lua_tostring(L, 2);
	if (!str)
		return luaL_error(L, "failed to get text");
	if (!add_message(scrolling_message_frame, str, lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tointeger(L, 6)))
		LOG_ERROR("failed to add message");
	return 0;
}

static bool register_methods(struct jks_array *methods)
{
	/*
	   AtTop
	   Clear
	   GetCurrentLine
	   GetCurrentScroll
	   GetFadeDuration
	   GetFading
	   GetInsertMode
	   GetMaxLines
	   GetNumLinesDisplayed
	   GetNumMessages
	   GetTimeVisible
	   PageDown
	   PageUp
	   SetFadeDuration
	   SetFading
	   SetInsertMode
	   SetMaxLines
	   SetScrollOffset
	   SetTimeVisible
	   UpdateColorByID
	 */
	UI_REGISTER_METHOD(AtBottom);
	UI_REGISTER_METHOD(ScrollUp);
	UI_REGISTER_METHOD(ScrollDown);
	UI_REGISTER_METHOD(ScrollToTop);
	UI_REGISTER_METHOD(ScrollToBottom);
	UI_REGISTER_METHOD(AddMessage);
	if (!ui_font_instance_register_methods(methods)) /* FontInstance only fill methods */
		return false;
	return ui_frame_vtable.register_methods(methods);
}

UI_INH0(frame, void, register_in_interface);
UI_INH0(frame, void, unregister_in_interface);
UI_INH0(frame, void, eval_name);
UI_INH0(frame, float, get_alpha);
UI_INH1(frame, void, set_alpha, float, alpha);
UI_INH1(frame, void, set_hidden, bool, hidden);
UI_INH2(frame, void, get_size, int32_t*, x, int32_t*, y);
UI_INH0(frame, void, set_dirty_coords);
UI_INH1(frame, void, on_mouse_move, gfx_pointer_event_t*, event);
UI_INH1(frame, void, on_mouse_down, gfx_mouse_event_t*, event);
UI_INH1(frame, void, on_mouse_up, gfx_mouse_event_t*, event);
UI_INH1(frame, void, on_mouse_scroll, gfx_scroll_event_t*, event);
UI_INH1(frame, bool, on_key_down, gfx_key_event_t*, event);
UI_INH1(frame, bool, on_key_up, gfx_key_event_t*, event);
UI_INH0(frame, const char*, get_name);

const struct ui_object_vtable ui_scrolling_message_frame_vtable =
{
	UI_OBJECT_VTABLE("ScrollingMessageFrame")
};
