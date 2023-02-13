#include "ui/minimap.h"

#include "itf/interface.h"

#include "xml/minimap.h"

#include "obj/player.h"

#include "map/map.h"

#include "shaders.h"
#include "wow_lua.h"
#include "memory.h"
#include "cache.h"
#include "log.h"
#include "wow.h"
#include "blp.h"

#include <gfx/device.h>

#include <string.h>
#include <math.h>

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_MINIMAP() LUA_METHOD(Minimap, minimap)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_FRAME->region)
#define UI_FRAME (&minimap->frame)

static float zooms[] =
{
	0.5f,
	0.4f,
	0.3f,
	0.2f,
	0.1f,
};

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_frame_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_minimap *minimap = (struct ui_minimap*)object;
	UI_OBJECT->mask |= UI_OBJECT_minimap;
	minimap->zoom = 0;
	minimap->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		minimap->uniform_buffers[i] = GFX_BUFFER_INIT();
	minimap->vertexes_buffer = GFX_BUFFER_INIT();
	minimap->indices_buffer = GFX_BUFFER_INIT();
	minimap->initialized = false;
	if (cache_ref_by_key_blp(g_wow->cache, "TEXTURES\\MINIMAPMASK.BLP", &minimap->mask))
	{
		blp_texture_ask_load(minimap->mask);
	}
	else
	{
		LOG_ERROR("failed to open minimap mask");
		minimap->mask = NULL;
	}
	return true;
}

static void dtr(struct ui_object *object)
{
	struct ui_minimap *minimap = (struct ui_minimap*)object;
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_delete_buffer(g_wow->device, &minimap->uniform_buffers[i]);
	gfx_delete_buffer(g_wow->device, &minimap->vertexes_buffer);
	gfx_delete_buffer(g_wow->device, &minimap->indices_buffer);
	gfx_delete_attributes_state(g_wow->device, &minimap->attributes_state);
	ui_frame_vtable.dtr(object);
}

static void render(struct ui_object *object)
{
	struct ui_minimap *minimap = (struct ui_minimap*)object;
	if (UI_REGION->hidden)
		return;
	if (!minimap->initialized)
	{
		static const uint16_t indices[6] = {0, 1, 2, 0, 2, 3};
		static const struct shader_ui_input vertexes[4] =
		{
			{{0.f, 0.f}, {1.f, 1.f, 1.f, 1.f}, {0.f, 0.f}},
			{{1.f, 0.f}, {1.f, 1.f, 1.f, 1.f}, {1.f, 0.f}},
			{{1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}, {1.f, 1.f}},
			{{0.f, 1.f}, {1.f, 1.f, 1.f, 1.f}, {0.f, 1.f}},
		};
		gfx_create_buffer(g_wow->device, &minimap->vertexes_buffer, GFX_BUFFER_VERTEXES, vertexes, sizeof(vertexes), GFX_BUFFER_IMMUTABLE);
		gfx_create_buffer(g_wow->device, &minimap->indices_buffer, GFX_BUFFER_INDICES, indices, sizeof(indices), GFX_BUFFER_IMMUTABLE);
		const struct gfx_attribute_bind binds[] =
		{
			{&minimap->vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, position)},
			{&minimap->vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, color)},
			{&minimap->vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, uv)},
		};
		gfx_create_attributes_state(g_wow->device, &minimap->attributes_state, binds, sizeof(binds) / sizeof(*binds), &minimap->indices_buffer, GFX_INDEX_UINT16);
		for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
			gfx_create_buffer(g_wow->device, &minimap->uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_ui_model_block), GFX_BUFFER_STREAM);
		minimap->initialized = true;
	}
	struct shader_ui_model_block model_block;
	{
		float offset_x = fmod(((struct worldobj*)g_wow->player)->position.x, 533.333334f) / 533.333334f;
		float offset_z = fmod(((struct worldobj*)g_wow->player)->position.z, 533.333334f) / 533.333334f;
		offset_x += floor(((struct worldobj*)g_wow->player)->position.x / 533.333334f) - g_wow->map->minimap.texture_x;
		offset_z += floor(((struct worldobj*)g_wow->player)->position.z / 533.333334f) - g_wow->map->minimap.texture_z;
		if (offset_x < 0)
			offset_x += 1;
		if (offset_z < 0)
			offset_z += 1;
		offset_x -= .5f;
		offset_z -= .5f;
		float scale = zooms[minimap->zoom];
		VEC4_SET(model_block.uv_transform, scale, (1 - scale) * .5 + offset_z * 0.333333, scale, (1 - scale) * .5 - offset_x * 0.333333);
	}
	model_block.alpha_test = 0;
	model_block.use_mask = 1;
	VEC4_SET(model_block.color, 1, 1, 1, 1);
	model_block.color.w *= ui_object_get_alpha(UI_OBJECT);
	if (model_block.color.w == 0)
		return;
	struct vec3f tmp = {(float)ui_region_get_left(UI_REGION), (float)ui_region_get_top(UI_REGION), 0};
	struct vec3f scale = {(float)ui_region_get_width(UI_REGION), (float)ui_region_get_height(UI_REGION), 1};
	MAT4_TRANSLATE(model_block.mvp, UI_OBJECT->interface->mat, tmp);
	MAT4_SCALE(model_block.mvp, model_block.mvp, scale);
	gfx_bind_attributes_state(g_wow->device, &minimap->attributes_state, &UI_OBJECT->interface->input_layout);
	gfx_bind_pipeline_state(g_wow->device, &UI_OBJECT->interface->pipeline_states[BLEND_BLEND]);
	gfx_set_buffer_data(&minimap->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
	gfx_bind_constant(g_wow->device, 1, &minimap->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	const gfx_texture_t *textures[2] = {&g_wow->map->minimap.texture, minimap->mask ? &minimap->mask->texture : NULL};
	gfx_bind_samplers(g_wow->device, 0, 2, textures);
	gfx_draw_indexed(g_wow->device, 3 * 32, 0);
	ui_frame_vtable.render(object);
}

static void on_click(struct ui_object *object, enum gfx_mouse_button button)
{
	ui_frame_vtable.on_click(object, button);
}

static void on_mouse_scroll(struct ui_object *object, struct gfx_scroll_event *event)
{
	struct ui_minimap *minimap = (struct ui_minimap*)object;
	if (event->used)
		return;
	if (!UI_REGION->hovered)
		return;
	if (event->y > 0)
	{
		if (minimap->zoom < sizeof(zooms) / sizeof(*zooms) - 1)
			minimap->zoom++;
	}
	else if (event->y < 0)
	{
		if (minimap->zoom > 0)
			minimap->zoom--;
	}
	event->used = true;
}

static int lua_GetZoom(lua_State *L)
{
	LUA_METHOD_MINIMAP();
	if (argc != 1)
		return luaL_error(L, "Usage: Minimap:GetZoom()");
	lua_pushinteger(L, minimap->zoom);
	return 1;
}

static int lua_SetZoom(lua_State *L)
{
	LUA_METHOD_MINIMAP();
	if (argc != 2)
		return luaL_error(L, "Usage: Minimap:SetZoom(zoom)");
	if (!lua_isinteger(L, 2))
		return luaL_argerror(L, 2, "integer expected");
	int zoom = lua_tointeger(L, 2);
	if (zoom < 0)
		zoom = 0;
	else if ((unsigned)zoom >= sizeof(zooms) / sizeof(*zooms))
		zoom = sizeof(zooms) / sizeof(*zooms) - 1;
	minimap->zoom = zoom;
	return 0;
}

static int lua_GetZoomLevels(lua_State *L)
{
	LUA_METHOD_MINIMAP();
	if (argc != 2)
		return luaL_error(L, "Usage: Minimap:GetZoomLevels()");
	lua_pushnumber(L, 5);
	return 1;
}

static int lua_SetMaskTexture(lua_State *L)
{
	LUA_METHOD_MINIMAP();
	if (argc != 2)
		return luaL_error(L, "Usage: Minimap:SetMaskTexture(\"file\")");
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
	if (!str)
		str = "textures/minimapmask.blp";
	char file[512];
	snprintf(file, sizeof(file), "%s", str);
	normalize_blp_filename(file, sizeof(file));
	if (minimap->mask)
		cache_unref_by_ref_blp(g_wow->cache, minimap->mask);
	if (cache_ref_by_key_blp(g_wow->cache, file, &minimap->mask))
		blp_texture_ask_load(minimap->mask);
	else
		minimap->mask = NULL;
	return 0;
}

static bool register_methods(struct jks_array *methods)
{
	/*
	   GetPingPosition
	   PingLocation
	   SetArrowModel
	   SetBlipTexture
	   SetIconTexture
	   SetPlayerModel
	 */
	UI_REGISTER_METHOD(GetZoom);
	UI_REGISTER_METHOD(SetZoom);
	UI_REGISTER_METHOD(GetZoomLevels);
	UI_REGISTER_METHOD(SetMaskTexture);
	return ui_frame_vtable.register_methods(methods);
}

UI_INH1(frame, void, load_xml, const struct xml_layout_frame*, layout_frame);
UI_INH0(frame, void, post_load);
UI_INH0(frame, void, register_in_interface);
UI_INH0(frame, void, unregister_in_interface);
UI_INH0(frame, void, eval_name);
UI_INH0(frame, float, get_alpha);
UI_INH1(frame, void, set_alpha, float, alpha);
UI_INH1(frame, void, set_hidden, bool, hidden);
UI_INH2(frame, void, get_size, int32_t*, x, int32_t*, y);
UI_INH0(frame, void, set_dirty_coords);
UI_INH1(frame, void, on_mouse_move, struct gfx_pointer_event*, event);
UI_INH1(frame, void, on_mouse_down, struct gfx_mouse_event*, event);
UI_INH1(frame, void, on_mouse_up, struct gfx_mouse_event*, event);
UI_INH1(frame, bool, on_key_down, struct gfx_key_event*, event);
UI_INH1(frame, bool, on_key_up, struct gfx_key_event*, event);
UI_INH0(frame, struct ui_font_instance*, as_font_instance);
UI_INH0(frame, const char*, get_name);

const struct ui_object_vtable ui_minimap_vtable =
{
	UI_OBJECT_VTABLE("Minimap")
};
