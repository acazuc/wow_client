#include "scrolling_message_frame.h"

#include "itf/interface.h"
#include "itf/font.h"

#include "font/font.h"

#include "wow_lua.h"
#include "shaders.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

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

static void update_buffers(struct ui_scrolling_message_frame *scrolling_message_frame, struct interface_font *font);
static void on_font_height_changed(struct ui_object *object);
static void on_color_changed(struct ui_object *object);
static void on_shadow_color_changed(struct ui_object *object);
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
	.on_shadow_color_changed = on_shadow_color_changed,
	.on_spacing_changed = on_spacing_changed,
	.on_outline_changed = on_outline_changed,
	.on_monochrome_changed = on_monochrome_changed,
	.on_justify_h_changed = on_justify_h_changed,
	.on_justify_v_changed = on_justify_v_changed,
};

static void message_dtr(void *ptr)
{
	mem_free(MEM_UI, ((struct ui_scrolling_message*)ptr)->text);
}

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_frame_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_scrolling_message_frame *scrolling_message_frame = (struct ui_scrolling_message_frame*)object;
	UI_OBJECT->mask |= UI_OBJECT_scrolling_message_frame;
	ui_font_instance_init(interface, UI_FONT_INSTANCE, object, &g_scrolling_message_frame_font_instance_callbacks);
	jks_array_init(&scrolling_message_frame->messages, sizeof(struct ui_scrolling_message), message_dtr, &jks_array_memory_fn_UI);
	scrolling_message_frame->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		scrolling_message_frame->uniform_buffers[i] = GFX_BUFFER_INIT();
	scrolling_message_frame->vertexes_buffer = GFX_BUFFER_INIT();
	scrolling_message_frame->indices_buffer = GFX_BUFFER_INIT();
	scrolling_message_frame->initialized = false;
	scrolling_message_frame->dirty_buffers = true;
	scrolling_message_frame->last_font = NULL;
	scrolling_message_frame->last_font_revision = 0;
	{
		struct ui_scrolling_message *message = jks_array_grow(&scrolling_message_frame->messages, 1);
		if (!message)
		{
			LOG_ERROR("message allocation failed");
			return false;
		}
		message->text = "ouioui";
		message->red = 1;
		message->green = .5;
		message->blue = .5;
		message->id = 1;
	}
	return true;
}

static void dtr(struct ui_object *object)
{
	struct ui_scrolling_message_frame *scrolling_message_frame = (struct ui_scrolling_message_frame*)object;
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_delete_buffer(g_wow->device, &scrolling_message_frame->uniform_buffers[i]);
	gfx_delete_buffer(g_wow->device, &scrolling_message_frame->vertexes_buffer);
	gfx_delete_buffer(g_wow->device, &scrolling_message_frame->indices_buffer);
	gfx_delete_attributes_state(g_wow->device, &scrolling_message_frame->attributes_state);
	jks_array_destroy(&scrolling_message_frame->messages);
	ui_font_instance_destroy(UI_FONT_INSTANCE);
	ui_frame_vtable.dtr(object);
}

static void render(struct ui_object *object)
{
	ui_frame_vtable.render(object);
	struct ui_scrolling_message_frame *scrolling_message_frame = (struct ui_scrolling_message_frame*)object;
	struct interface_font *font = ui_font_instance_get_render_font(UI_FONT_INSTANCE);
	if (!font)
		return;
	if (!scrolling_message_frame->initialized)
	{
		for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
			gfx_create_buffer(g_wow->device, &scrolling_message_frame->uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_ui_model_block), GFX_BUFFER_STREAM);
		scrolling_message_frame->initialized = true;
	}
	if (scrolling_message_frame->dirty_buffers)
	{
		update_buffers(scrolling_message_frame, font);
		scrolling_message_frame->last_font_revision = font->atlas->revision;
		scrolling_message_frame->last_font = font;
	}
	if (!scrolling_message_frame->indices_nb)
		return;
	struct shader_ui_model_block model_block;
	VEC4_SET(model_block.color, 1, 1, 1, 1);
	VEC4_SET(model_block.uv_transform, 1, 0, 1, 0);
	font_atlas_update(font->atlas);
	if (scrolling_message_frame->last_font != font || font->atlas->revision != scrolling_message_frame->last_font_revision)
	{
		update_buffers(scrolling_message_frame, font);
		scrolling_message_frame->last_font_revision = font->atlas->revision;
		scrolling_message_frame->last_font = font;
	}
	interface_set_render_ctx(UI_OBJECT->interface, true);
	font_atlas_bind(font->atlas, 0);
	gfx_bind_attributes_state(g_wow->device, &scrolling_message_frame->attributes_state, &UI_OBJECT->interface->input_layout);
	gfx_bind_pipeline_state(g_wow->device, &UI_OBJECT->interface->pipeline_states[INTERFACE_BLEND_ALPHA]);
	model_block.alpha_test = 0;
	model_block.use_mask = 0;
	struct vec3f tmp = {(float)ui_region_get_left(UI_REGION), (float)ui_region_get_top(UI_REGION), 0};
	MAT4_TRANSLATE(model_block.mvp, UI_OBJECT->interface->mat, tmp);
	gfx_set_buffer_data(&scrolling_message_frame->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
	gfx_bind_constant(g_wow->device, 1, &scrolling_message_frame->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	gfx_draw_indexed(g_wow->device, scrolling_message_frame->indices_nb, 0);
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

static void update_buffers(struct ui_scrolling_message_frame *scrolling_message_frame, struct interface_font *font)
{
	struct jks_array vertexes; /* struct shader_ui_input */
	struct jks_array indices; /* uint16_t */
	jks_array_init(&vertexes, sizeof(struct shader_ui_input), NULL, &jks_array_memory_fn_UI);
	jks_array_init(&indices, sizeof(uint16_t), NULL, &jks_array_memory_fn_UI);
	int32_t x = 0;
	int32_t y = 0;
	for (size_t i = 0; i < scrolling_message_frame->messages.size; ++i)
	{
		y -= font->font->height;
		struct ui_scrolling_message *message = JKS_ARRAY_GET(&scrolling_message_frame->messages, i, struct ui_scrolling_message);
		const char *iter = message->text;
		const char *end = iter + strlen(message->text);
		while (iter < end)
		{
			uint32_t character;
			if (!utf8_next(&iter, end, &character))
				break;
			struct font_glyph *glyph = font_get_glyph(font->font, character);
			if (!glyph)
				continue;
			float char_width = glyph->advance;
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
	}
	scrolling_message_frame->indices_nb = indices.size;
	gfx_delete_buffer(g_wow->device, &scrolling_message_frame->vertexes_buffer);
	gfx_delete_buffer(g_wow->device, &scrolling_message_frame->indices_buffer);
	gfx_delete_attributes_state(g_wow->device, &scrolling_message_frame->attributes_state);
	gfx_create_buffer(g_wow->device, &scrolling_message_frame->vertexes_buffer, GFX_BUFFER_VERTEXES, vertexes.data, vertexes.size * sizeof(struct shader_ui_input), GFX_BUFFER_STATIC);
	gfx_create_buffer(g_wow->device, &scrolling_message_frame->indices_buffer, GFX_BUFFER_INDICES, indices.data, indices.size * sizeof(uint16_t), GFX_BUFFER_STATIC);
	gfx_attribute_bind_t binds[] =
	{
		{&scrolling_message_frame->vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, position)},
		{&scrolling_message_frame->vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, color)},
		{&scrolling_message_frame->vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, uv)},
	};
	gfx_create_attributes_state(g_wow->device, &scrolling_message_frame->attributes_state, binds, sizeof(binds) / sizeof(*binds), &scrolling_message_frame->indices_buffer, GFX_INDEX_UINT16);

err:
	jks_array_destroy(&vertexes);
	jks_array_destroy(&indices);
	scrolling_message_frame->dirty_buffers = false;
}

static void on_font_height_changed(struct ui_object *object)
{
	(void)object;
}

static void on_color_changed(struct ui_object *object)
{
	(void)object;
}

static void on_shadow_color_changed(struct ui_object *object)
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
	char *text = mem_malloc(MEM_UI, strlen(str));
	if (!text)
	{
		LOG_ERROR("text allocation failed");
		return 0;
	}
	struct ui_scrolling_message *message = jks_array_grow(&scrolling_message_frame->messages, 1);
	if (!message)
	{
		LOG_ERROR("message allocation failed");
		mem_free(MEM_UI, text);
		return 0;
	}
	message->text = text;
	message->red = lua_tonumber(L, 3);
	message->green = lua_tonumber(L, 4);
	message->blue = lua_tonumber(L, 5);
	message->id = lua_tointeger(L, 6);
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

UI_INH1(frame, void, load_xml, const struct xml_layout_frame*, layour_frame);
UI_INH0(frame, void, post_load);
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
