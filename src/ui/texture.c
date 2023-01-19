#include "texture.h"

#include "itf/interface.h"

#include "xml/texture.h"

#include "shaders.h"
#include "wow_lua.h"
#include "memory.h"
#include "cache.h"
#include "log.h"
#include "blp.h"
#include "wow.h"

#include <gfx/device.h>

#include <string.h>

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_TEXTURE() LUA_METHOD(Texture, texture)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_LAYERED_REGION->region)
#define UI_LAYERED_REGION (&texture->layered_region)

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_layered_region_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_texture *texture = (struct ui_texture*)object;
	UI_OBJECT->mask |= UI_OBJECT_texture;
	OPTIONAL_UNSET(texture->tex_coords);
	OPTIONAL_UNSET(texture->gradient);
	OPTIONAL_UNSET(texture->color);
	texture->texture = NULL;
	texture->alpha_mode = BLEND_BLEND;
	texture->update_tex_coords = false;
	texture->initialized = false;
	texture->file = NULL;
	texture->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		texture->uniform_buffers[i] = GFX_BUFFER_INIT();
	texture->vertexes_buffer = GFX_BUFFER_INIT();
	texture->indices_buffer = GFX_BUFFER_INIT();
	return true;
}

static void dtr(struct ui_object *object)
{
	struct ui_texture *texture = (struct ui_texture*)object;
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_delete_buffer(g_wow->device, &texture->uniform_buffers[i]);
	gfx_delete_buffer(g_wow->device, &texture->vertexes_buffer);
	gfx_delete_buffer(g_wow->device, &texture->indices_buffer);
	gfx_delete_attributes_state(g_wow->device, &texture->attributes_state);
	if (texture->texture)
		cache_unref_by_ref_blp(g_wow->cache, texture->texture);
	mem_free(MEM_UI, texture->file);
	ui_layered_region_vtable.dtr(object);
}

static void load_xml(struct ui_object *object, const struct xml_layout_frame *layout_frame)
{
	ui_layered_region_vtable.load_xml(object, layout_frame);
	struct ui_texture *texture = (struct ui_texture*)object;
	const struct xml_texture *xml_texture = xml_clayout_frame_as_texture(layout_frame);
	const char *file = NULL;
	if (xml_texture)
	{
		if (OPTIONAL_ISSET(xml_texture->tex_coords))
		{
			OPTIONAL_SET(texture->tex_coords);
			ui_tex_coords_init_xml(&OPTIONAL_GET(texture->tex_coords), &OPTIONAL_GET(xml_texture->tex_coords));
		}
		if (OPTIONAL_ISSET(xml_texture->gradient))
		{
			OPTIONAL_SET(texture->gradient);
			ui_gradient_init_xml(&OPTIONAL_GET(texture->gradient), &OPTIONAL_GET(xml_texture->gradient));
		}
		if (OPTIONAL_ISSET(xml_texture->color))
		{
			OPTIONAL_SET(texture->color);
			ui_color_init_xml(&OPTIONAL_GET(texture->color), &OPTIONAL_GET(xml_texture->color));
		}
		if (xml_texture->file)
			file = xml_texture->file;
		if (xml_texture->alpha_mode)
		{
			if (!blend_mode_from_string(xml_texture->alpha_mode, &texture->alpha_mode))
				LOG_ERROR("invalid blend mode: %s", xml_texture->alpha_mode);
		}
	}
	if (file)
		ui_texture_set_file(texture, file);
}

static void update_vertexes_buffer(struct ui_texture *texture)
{
	struct shader_ui_input vertexes[4];
	VEC2_SET(vertexes[0].position, 0, 0);
	VEC2_SET(vertexes[1].position, 1, 0);
	VEC2_SET(vertexes[2].position, 1, 1);
	VEC2_SET(vertexes[3].position, 0, 1);
	if (OPTIONAL_ISSET(texture->tex_coords))
	{
		VEC2_CPY(vertexes[0].uv, OPTIONAL_GET(texture->tex_coords).top_left);
		VEC2_CPY(vertexes[1].uv, OPTIONAL_GET(texture->tex_coords).top_right);
		VEC2_CPY(vertexes[2].uv, OPTIONAL_GET(texture->tex_coords).bottom_right);
		VEC2_CPY(vertexes[3].uv, OPTIONAL_GET(texture->tex_coords).bottom_left);
	}
	else
	{
		VEC2_SET(vertexes[0].uv, 0, 0);
		VEC2_SET(vertexes[1].uv, 1, 0);
		VEC2_SET(vertexes[2].uv, 1, 1);
		VEC2_SET(vertexes[3].uv, 0, 1);
	}
	VEC4_SET(vertexes[0].color, 1, 1, 1, 1);
	VEC4_SET(vertexes[1].color, 1, 1, 1, 1);
	VEC4_SET(vertexes[2].color, 1, 1, 1, 1);
	VEC4_SET(vertexes[3].color, 1, 1, 1, 1);
	if (OPTIONAL_ISSET(texture->gradient))
	{
		if (OPTIONAL_GET(texture->gradient).orientation == ORIENTATION_VERTICAL)
		{
			VEC4_MUL(vertexes[0].color, vertexes[0].color, OPTIONAL_GET(texture->gradient).max_color);
			VEC4_MUL(vertexes[1].color, vertexes[1].color, OPTIONAL_GET(texture->gradient).max_color);
			VEC4_MUL(vertexes[2].color, vertexes[2].color, OPTIONAL_GET(texture->gradient).min_color);
			VEC4_MUL(vertexes[3].color, vertexes[3].color, OPTIONAL_GET(texture->gradient).min_color);
		}
		else
		{
			VEC4_MUL(vertexes[0].color, vertexes[0].color, OPTIONAL_GET(texture->gradient).max_color);
			VEC4_MUL(vertexes[1].color, vertexes[1].color, OPTIONAL_GET(texture->gradient).min_color);
			VEC4_MUL(vertexes[2].color, vertexes[2].color, OPTIONAL_GET(texture->gradient).min_color);
			VEC4_MUL(vertexes[3].color, vertexes[3].color, OPTIONAL_GET(texture->gradient).max_color);
		}
	}
	gfx_set_buffer_data(&texture->vertexes_buffer, vertexes, sizeof(vertexes), 0);
}

static void render(struct ui_object *object)
{
	struct ui_texture *texture = (struct ui_texture*)object;
	if (UI_REGION->hidden)
		return;
	if (!texture->initialized)
	{
		static const uint16_t indices[6] = {0, 1, 2, 0, 2, 3};
		gfx_create_buffer(g_wow->device, &texture->vertexes_buffer, GFX_BUFFER_VERTEXES, NULL, 4 * sizeof(struct shader_ui_input), GFX_BUFFER_STATIC);
		gfx_create_buffer(g_wow->device, &texture->indices_buffer, GFX_BUFFER_INDICES, indices, sizeof(indices), GFX_BUFFER_IMMUTABLE);
		update_vertexes_buffer(texture);
		gfx_attribute_bind_t binds[] =
		{
			{&texture->vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, position)},
			{&texture->vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, color)},
			{&texture->vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, uv)},
		};
		gfx_create_attributes_state(g_wow->device, &texture->attributes_state, binds, sizeof(binds) / sizeof(*binds), &texture->indices_buffer, GFX_INDEX_UINT16);
		for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
			gfx_create_buffer(g_wow->device, &texture->uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_ui_model_block), GFX_BUFFER_STREAM);
		texture->initialized = true;
	}
	else if (texture->update_tex_coords)
	{
		update_vertexes_buffer(texture);
		texture->update_tex_coords = false;
	}
	struct shader_ui_model_block model_block;
	VEC4_CPY(model_block.color, UI_LAYERED_REGION->vertex_color);
	if (OPTIONAL_ISSET(texture->color))
		VEC4_MUL(model_block.color, model_block.color, OPTIONAL_GET(texture->color));
	model_block.color.w *= ui_object_get_alpha(UI_OBJECT);
	if (model_block.color.w == 0)
		return;
	model_block.use_mask = 0;
	VEC4_SET(model_block.uv_transform, 1, 0, 1, 0);
	interface_set_render_ctx(UI_OBJECT->interface, true);
	struct vec3f tmp = {(float)ui_region_get_left(UI_REGION), (float)ui_region_get_top(UI_REGION), 0};
	struct vec3f scale = {(float)ui_region_get_width(UI_REGION), (float)ui_region_get_height(UI_REGION), 1};
	MAT4_TRANSLATE(model_block.mvp, UI_OBJECT->interface->mat, tmp);
	MAT4_SCALE(model_block.mvp, model_block.mvp, scale);
	gfx_bind_attributes_state(g_wow->device, &texture->attributes_state, &UI_OBJECT->interface->input_layout);
	{
		enum interface_blend_state blend_state;
		switch (texture->alpha_mode)
		{
			case BLEND_DISABLE:
				blend_state = INTERFACE_BLEND_OPAQUE;
				model_block.alpha_test = 0;
				break;
			default:
				/* FALLTHROUGH */
			case BLEND_BLEND:
				blend_state = INTERFACE_BLEND_ALPHA;
				model_block.alpha_test = 0;
				break;
			case BLEND_ALPHAKEY:
				blend_state = INTERFACE_BLEND_OPAQUE;
				model_block.alpha_test = 224. / 255;
				break;
			case BLEND_ADD:
				blend_state = INTERFACE_BLEND_ADD;
				model_block.alpha_test = 0;
				break;
			case BLEND_MOD:
				blend_state = INTERFACE_BLEND_MOD;
				model_block.alpha_test = 0;
				break;
		}
		gfx_bind_pipeline_state(g_wow->device, &UI_OBJECT->interface->pipeline_states[blend_state]);
	}
	gfx_set_buffer_data(&texture->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
	gfx_bind_constant(g_wow->device, 1, &texture->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	if (texture->texture && texture->texture->initialized)
		gfx_set_texture_addressing(&texture->texture->texture, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP, GFX_TEXTURE_ADDRESSING_CLAMP);
	blp_texture_bind(texture->texture, 0);
	gfx_draw_indexed(g_wow->device, 6, 0);
	ui_layered_region_vtable.render(object);
}

void ui_texture_set_file(struct ui_texture *texture, const char *f)
{
	if (!f || !f[0])
	{
		texture->texture = NULL;
		texture->file = NULL;
		return;
	}
	char file[512];
	snprintf(file, sizeof(file), "%s", f);
	normalize_blp_filename(file, sizeof(file));
	if (texture->file && !strcmp(texture->file, file))
		return;
	if (texture->texture)
		cache_unref_by_ref_blp(g_wow->cache, texture->texture);
	mem_free(MEM_UI, texture->file);
	texture->file = mem_strdup(MEM_UI, file);
	if (texture->file)
	{
		if (cache_ref_by_key_blp(g_wow->cache, texture->file, &texture->texture))
			blp_texture_ask_load(texture->texture);
		else
			texture->texture = NULL;
	}
	else
	{
		LOG_ERROR("failed to duplicate file name");
	}
}

static int lua_GetTexCoord(lua_State *L)
{
	LUA_METHOD_TEXTURE();
	if (argc != 1)
		return luaL_error(L, "Usage: Texture:GetTexCoords()");
	if (!OPTIONAL_ISSET(texture->tex_coords))
	{
		lua_pushnumber(L, 0);
		lua_pushnumber(L, 0);
		lua_pushnumber(L, 0);
		lua_pushnumber(L, 1);
		lua_pushnumber(L, 1);
		lua_pushnumber(L, 0);
		lua_pushnumber(L, 1);
		lua_pushnumber(L, 1);
		return 8;
	}
	lua_pushnumber(L, OPTIONAL_GET(texture->tex_coords).top_left.x);
	lua_pushnumber(L, OPTIONAL_GET(texture->tex_coords).top_left.y);
	lua_pushnumber(L, OPTIONAL_GET(texture->tex_coords).bottom_left.x);
	lua_pushnumber(L, OPTIONAL_GET(texture->tex_coords).bottom_left.y);
	lua_pushnumber(L, OPTIONAL_GET(texture->tex_coords).top_right.x);
	lua_pushnumber(L, OPTIONAL_GET(texture->tex_coords).top_right.y);
	lua_pushnumber(L, OPTIONAL_GET(texture->tex_coords).bottom_right.x);
	lua_pushnumber(L, OPTIONAL_GET(texture->tex_coords).bottom_right.y);
	return 8;
}

static int lua_SetTexCoord(lua_State *L)
{
	LUA_METHOD_TEXTURE();
	if (argc == 5)
	{
		float values[4];
		for (size_t i = 0; i < 4; ++i)
		{
			if (!lua_isnumber(L, 2 + i))
				return luaL_argerror(L, 2 + i, "number expected");
			values[i] = lua_tonumber(L, 2 + i);
		}
		OPTIONAL_SET(texture->tex_coords);
		ui_tex_coords_init(&OPTIONAL_GET(texture->tex_coords), values[0], values[1], values[2], values[3]);
		texture->update_tex_coords = true;
		return 0;
	}
	if (argc == 9)
	{
		struct vec2f values[4];
		for (size_t i = 0; i < 8; ++i)
		{
			if (!lua_isnumber(L, 2 + i))
				return luaL_argerror(L, 2 + i, "number expected");
			((float*)values)[i] = lua_tonumber(L, 2 + i);
		}
		OPTIONAL_SET(texture->tex_coords);
		ui_tex_coords_init_vec(&OPTIONAL_GET(texture->tex_coords), values[0], values[2], values[3], values[1]);
		texture->update_tex_coords = true;
		return 0;
	}
	return luaL_error(L, "Usage: Texture:SetTexCoords(left, right, top, bottom)");
}

static int lua_GetTexture(lua_State *L)
{
	LUA_METHOD_TEXTURE();
	if (argc != 1)
		return luaL_error(L, "Usage: Texture:GetTexture()");
	lua_pushstring(L, texture->file);
	return 1;
}

static int lua_SetTexture(lua_State *L)
{
	LUA_METHOD_TEXTURE();
	if (argc != 2)
		return luaL_error(L, "Usage: Texture:SetTexture(\"file\")");
	const char *str;
	if (lua_isnil(L, 2))
	{
		str = NULL;
	}
	else
	{
		if (!lua_isstring(L, 2))
			return luaL_argerror(L, 2, "string expected");
		str = lua_tostring(L, 2);
	}
	ui_texture_set_file(texture, str);
	return 0;
}

static int lua_IsDesaturated(lua_State *L)
{
	LUA_METHOD_TEXTURE();
	if (argc != 1)
		return luaL_error(L, "Usage: Texture:IsDesaturated()");
	LUA_UNIMPLEMENTED_METHOD();
	lua_pushboolean(L, false);
	return 1;
}

static int lua_SetDesaturated(lua_State *L)
{
	LUA_METHOD_TEXTURE();
	if (argc != 2)
		return luaL_error(L, "Usage: Texture:SetDesaturated(desaturated)");
	LUA_UNIMPLEMENTED_METHOD();
	lua_pushboolean(L, true); /* seems to be used to check if shader if supported */
	return 1;
}

static bool register_methods(struct jks_array *methods)
{
	/*
	   GetBlendMode
	   GetTexCoordModifiesRect
	   GetVertexColor
	   SetBlendMode
	   SetGradient
	   SetGradientAlpha
	   SetTexCoordModifiesRect
	 */
	UI_REGISTER_METHOD(GetTexCoord);
	UI_REGISTER_METHOD(SetTexCoord);
	UI_REGISTER_METHOD(GetTexture);
	UI_REGISTER_METHOD(SetTexture);
	UI_REGISTER_METHOD(IsDesaturated);
	UI_REGISTER_METHOD(SetDesaturated);
	return ui_layered_region_vtable.register_methods(methods);
}

UI_INH0(layered_region, void, post_load);
UI_INH0(layered_region, void, register_in_interface);
UI_INH0(layered_region, void, unregister_in_interface);
UI_INH0(layered_region, void, eval_name);
UI_INH1(layered_region, void, on_click, enum gfx_mouse_button, button);
UI_INH0(layered_region, float, get_alpha);
UI_INH1(layered_region, void, set_alpha, float, alpha);
UI_INH1(layered_region, void, set_hidden, bool, hidden);
UI_INH2(layered_region, void, get_size, int32_t*, x, int32_t*, y);
UI_INH0(layered_region, void, set_dirty_coords);
UI_INH1(layered_region, void, on_mouse_move, gfx_pointer_event_t*, event);
UI_INH1(layered_region, void, on_mouse_down, gfx_mouse_event_t*, event);
UI_INH1(layered_region, void, on_mouse_up, gfx_mouse_event_t*, event);
UI_INH1(layered_region, void, on_mouse_scroll, gfx_scroll_event_t*, event);
UI_INH1(layered_region, bool, on_key_down, gfx_key_event_t*, event);
UI_INH1(layered_region, bool, on_key_up, gfx_key_event_t*, event);
UI_INH0(layered_region, struct ui_font_instance*, as_font_instance);
UI_INH0(layered_region, const char*, get_name);

const struct ui_object_vtable ui_texture_vtable =
{
	UI_OBJECT_VTABLE("Texture")
};
