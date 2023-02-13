#include "ui/model.h"

#include "itf/interface.h"

#include "xml/model.h"

#include "gx/m2_particles.h"
#include "gx/m2.h"

#include "shaders.h"
#include "wow_lua.h"
#include "log.h"
#include "wow.h"

#include <gfx/device.h>

#include <math.h>

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_MODEL() LUA_METHOD(Model, model)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_FRAME->region)
#define UI_FRAME (&model->frame)

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_frame_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_model *model = (struct ui_model*)object;
	UI_OBJECT->mask |= UI_OBJECT_model;
	model->m2 = NULL;
	model->camera = 0;
	model->scale = 1;
	model->fog_near = 1;
	model->fog_far = 1;
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		model->uniform_buffers[i] = GFX_BUFFER_INIT();
		model->particles_uniform_buffers[i] = GFX_BUFFER_INIT();
	}
	ui_color_init(&model->fog_color, 1, 1, 1, 1);
	return true;
}

static void dtr(struct ui_object *object)
{
	struct ui_model *model = (struct ui_model*)object;
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		gfx_delete_buffer(g_wow->device, &model->uniform_buffers[i]);
		gfx_delete_buffer(g_wow->device, &model->particles_uniform_buffers[i]);
	}
	gx_m2_instance_gc(model->m2);
	ui_frame_vtable.dtr(object);
}

static void load_xml(struct ui_object *object, const struct xml_layout_frame *layout_frame)
{
	ui_frame_vtable.load_xml(object, layout_frame);
	struct ui_model *model = (struct ui_model*)object;
	const struct xml_model *xml_model = xml_clayout_frame_as_model(layout_frame);
	if (xml_model)
	{
		if (xml_model->file)
		{
			char filename[512];
			snprintf(filename, sizeof(filename), "%s", xml_model->file);
			normalize_m2_filename(filename, sizeof(filename));
			model->m2 = gx_m2_instance_new_filename(filename);
			if (!model->m2)
				LOG_ERROR("failed to create m2 renderer instance");
			gx_m2_ask_load(model->m2->parent);
		}
		if (OPTIONAL_ISSET(xml_model->fog_color))
			ui_color_init_xml(&model->fog_color, &OPTIONAL_GET(xml_model->fog_color));
		if (OPTIONAL_ISSET(xml_model->scale))
			model->scale = OPTIONAL_GET(xml_model->scale);
		if (OPTIONAL_ISSET(xml_model->fog_near))
			model->fog_near = OPTIONAL_GET(xml_model->fog_near);
		if (OPTIONAL_ISSET(xml_model->fog_far))
			model->fog_far = OPTIONAL_GET(xml_model->fog_far);
	}
}

static void render(struct ui_object *object)
{
	struct ui_model *model = (struct ui_model*)object;
	if (model->m2 && model->m2->parent->cameras && model->camera < model->m2->parent->cameras_nb)
	{
		if (!model->uniform_buffers[0].handle.u64)
		{
			for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
			{
				gfx_create_buffer(g_wow->device, &model->uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_m2_scene_block), GFX_BUFFER_STREAM);
				gfx_create_buffer(g_wow->device, &model->particles_uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_particle_scene_block), GFX_BUFFER_STREAM);
			}
		}
		int32_t left = ui_region_get_left(UI_REGION) * (g_wow->render_width / (float)UI_OBJECT->interface->width);
		int32_t top = ui_region_get_top(UI_REGION) * (g_wow->render_height / (float)UI_OBJECT->interface->height);
		int32_t width = ui_region_get_width(UI_REGION) * (g_wow->render_width / (float)UI_OBJECT->interface->width);
		int32_t height = ui_region_get_height(UI_REGION) * (g_wow->render_height / (float)UI_OBJECT->interface->height);
		gfx_set_viewport(g_wow->device, left, top, width, height);
		gfx_clear_depth_stencil(g_wow->device, NULL, 1, 0);
		struct gx_m2_render_params params;
		VEC3_CPY(params.fog_color, model->fog_color);
		{
			struct wow_m2_camera *camera = &model->m2->parent->cameras[model->camera];
			float aspect = ui_region_get_width(UI_REGION) / (float)ui_region_get_height(UI_REGION);
			float fov = camera->fov / sqrtf(1 + powf(aspect, 2));
			MAT4_PERSPECTIVE(params.p, fov, aspect, camera->near_clip, camera->far_clip);
			struct vec3f position = {camera->position_base.x, camera->position_base.z, -camera->position_base.y};
			struct vec3f target = {camera->target_position_base.x, camera->target_position_base.z, -camera->target_position_base.y};
			struct vec3f up = {0, 1, 0};
			MAT4_LOOKAT(float, params.v, position, target, up);
			/* XXX tracks */
			MAT4_MUL(params.vp, params.p, params.v);
			const struct vec4f right = {1, 0, 0, 0};
			const struct vec4f bottom = {0, -1, 0, 0};
			struct mat4f tmp;
			struct vec4f t;
			MAT4_INVERSE(float, tmp, params.v);
			MAT4_VEC4_MUL(t, tmp, right);
			VEC3_CPY(params.view_right, t);
			MAT4_VEC4_MUL(t, tmp, bottom);
			VEC3_CPY(params.view_bottom, t);
		}
		{
			struct shader_m2_scene_block scene_block;
			VEC4_SET(scene_block.light_direction, -1, -1, 1, 0);
			VEC4_SETV(scene_block.specular_color, 0);
			VEC4_SETV(scene_block.diffuse_color, 0);
			VEC4_SETV(scene_block.ambient_color, 0);
			scene_block.fog_range.y = model->fog_far;
			scene_block.fog_range.x = scene_block.fog_range.y * model->fog_near;
			gfx_set_buffer_data(&model->uniform_buffers[g_wow->draw_frame_id], &scene_block, sizeof(scene_block), 0);
			gfx_bind_constant(g_wow->device, 2, &model->uniform_buffers[g_wow->draw_frame_id], sizeof(scene_block), 0);
			model->m2->camera = model->camera;
			model->m2->bones_calculated = false;
			gx_m2_instance_force_update(model->m2, &params);
			model->m2->enable_lights = true;
			gx_m2_instance_render(model->m2, false, &params);
			gx_m2_instance_render(model->m2, true, &params);
		}
		if (model->m2->gx_particles)
		{
			struct shader_particle_scene_block scene_block;
			scene_block.fog_range.y = model->fog_far;
			scene_block.fog_range.x = scene_block.fog_range.y * model->fog_near;
			gfx_set_buffer_data(&model->particles_uniform_buffers[g_wow->draw_frame_id], &scene_block, sizeof(scene_block), 0);
			gfx_bind_constant(g_wow->device, 2, &model->particles_uniform_buffers[g_wow->draw_frame_id], sizeof(scene_block), 0);
			gx_m2_particles_update(model->m2->gx_particles, &params);
			gx_m2_instance_render_particles(model->m2, &params);
		}
	}
	ui_frame_vtable.render(object);
}

void ui_model_set_model(struct ui_model *model, const char *file)
{
	gx_m2_instance_gc(model->m2);
	char filename[512];
	snprintf(filename, sizeof(filename), "%s", file);
	normalize_m2_filename(filename, sizeof(filename));
	model->m2 = gx_m2_instance_new_filename(filename);
	if (!model->m2)
		LOG_ERROR("failed to create m2 renderer instance");
	gx_m2_ask_load(model->m2->parent);
}

void ui_model_set_scale(struct ui_model *model, float scale)
{
	if (model->scale == scale)
		return;
	model->scale = scale;
	/* TODO event ? */
}

void ui_model_set_camera(struct ui_model *model, uint32_t camera)
{
	if (model->camera == camera)
		return;
	model->camera = camera;
	/* TODO event ? */
}

void ui_model_set_fog_near(struct ui_model *model, float fog_near)
{
	if (model->fog_near == fog_near)
		return;
	model->fog_near = fog_near;
	/* TODO event ? */
}

void ui_model_set_fog_far(struct ui_model *model, float fog_far)
{
	if (model->fog_far == fog_far)
		return;
	model->fog_far = fog_far;
	/* TODO event ? */
}

void ui_model_set_fog_color(struct ui_model *model, const struct ui_color *color)
{
	model->fog_color = *color;
	/* TODO event ? */
}

static int lua_SetSequence(lua_State *L)
{
	LUA_METHOD_MODEL();
	if (argc != 2)
		return luaL_error(L, "Usage: Model:SetSequence(sequence)");
	if (!lua_isinteger(L, 2))
		return luaL_argerror(L, 2, "integer expected");
	int sequence = lua_tointeger(L, 2);
	if (model->m2)
		gx_m2_instance_set_sequence(model->m2, sequence);
	return 0;
}

static int lua_SetSequenceTime(lua_State *L)
{
	LUA_METHOD_MODEL();
	if (argc != 3)
		return luaL_error(L, "Usage: Model:SetSequenceTime(sequence, time)");
	LUA_UNIMPLEMENTED_METHOD();
	if (!lua_isinteger(L, 2))
		return luaL_argerror(L, 2, "integer expected");
	int sequence = lua_tointeger(L, 2);
	if (model->m2)
		gx_m2_instance_set_sequence(model->m2, sequence);
	/* XXX: sequence start */
	return 0;
}

static int lua_GetModel(lua_State *L)
{
	LUA_METHOD_MODEL();
	if (argc != 1)
		return luaL_error(L, "Usage: Model:GetModel()");
	LUA_UNIMPLEMENTED_METHOD();
	lua_pushstring(L, "");
	return 1;
}

static int lua_SetModel(lua_State *L)
{
	LUA_METHOD_MODEL();
	if (argc != 2)
		return luaL_error(L, "Usage: Model:SetModel(\"file\")");
	const char *file = lua_tostring(L, 2);
	ui_model_set_model(model, file);
	return 0;
}

static int lua_GetModelScale(lua_State *L)
{
	LUA_METHOD_MODEL();
	if (argc != 1)
		return luaL_error(L, "Usage: Model:GetModelScale()");
	lua_pushnumber(L, model->scale);
	return 1;
}

static int lua_SetModelScale(lua_State *L)
{
	LUA_METHOD_MODEL();
	if (argc != 2)
		return luaL_error(L, "Usage: Model:SetModelScale(scale)");
	if (!lua_isnumber(L, 2))
		return luaL_argerror(L, 2, "number expected");
	ui_model_set_scale(model, lua_tonumber(L, 2));
	return 0;
}

static int lua_SetCamera(lua_State *L)
{
	LUA_METHOD_MODEL();
	if (argc != 2)
		return luaL_error(L, "Usage: Model:SetCamera(camera)");
	if (!lua_isinteger(L, 2))
		return luaL_argerror(L, 2, "integer expected");
	ui_model_set_camera(model, lua_tointeger(L, 2));
	return 0;
}

static int lua_GetFogNear(lua_State *L)
{
	LUA_METHOD_MODEL();
	if (argc != 1)
		return luaL_error(L, "Usage: Model:GetFogNear()");
	lua_pushnumber(L, model->fog_near);
	return 1;
}

static int lua_SetFogNear(lua_State *L)
{
	LUA_METHOD_MODEL();
	if (argc != 2)
		return luaL_error(L, "Usage: Model:SetFogNear(near)");
	if (!lua_isnumber(L, 2))
		return luaL_argerror(L, 2, "number expected");
	ui_model_set_fog_near(model, lua_tonumber(L, 2));
	return 0;
}

static int lua_GetFogFar(lua_State *L)
{
	LUA_METHOD_MODEL();
	if (argc != 1)
		return luaL_error(L, "Usage: Model:GetFogFar()");
	lua_pushnumber(L, model->fog_far);
	return 1;
}

static int lua_SetFogFar(lua_State *L)
{
	LUA_METHOD_MODEL();
	if (argc != 2)
		return luaL_error(L, "Usage: Model:SetFogFar(far)");
	if (!lua_isnumber(L, 2))
		return luaL_argerror(L, 2, "number expected");
	ui_model_set_fog_far(model, lua_tonumber(L, 2));
	return 0;
}

static int lua_GetFogColor(lua_State *L)
{
	LUA_METHOD_MODEL();
	if (argc != 1)
		return luaL_error(L, "Usage: Model:GetFogColor()");
	lua_pushnumber(L, model->fog_color.r);
	lua_pushnumber(L, model->fog_color.g);
	lua_pushnumber(L, model->fog_color.b);
	lua_pushnumber(L, model->fog_color.a);
	return 4;
}

static int lua_SetFogColor(lua_State *L)
{
	LUA_METHOD_MODEL();
	if (argc < 4 || argc > 5)
		return luaL_error(L, "Usage: Model:SetFogColor(r, g, b [, a])");
	struct ui_color color;
	if (!ui_get_lua_color(L, 2, &color))
		return luaL_argerror(L, 2, "color expected");
	ui_model_set_fog_color(model, &color);
	return 0;
}

static int lua_ClearFog(lua_State *L)
{
	LUA_METHOD_MODEL();
	if (argc != 1)
		return luaL_error(L, "Usage: Model:ClearFog()");
	/* tkt */
	model->fog_near = 100000000000000000000.f;
	model->fog_far = 100000000000000000000.f;
	ui_color_init(&model->fog_color, 1, 1, 1, 1);
	return 0;
}

static bool register_methods(struct jks_array *methods)
{
	/*
	   AdvanceTime
	   ClearModel
	   GetFacing
	   GetLight
	   GetPosition
	   ReplaceIconTexture
	   SetFacing
	   SetLight
	   SetPosition
	 */
	UI_REGISTER_METHOD(SetSequence);
	UI_REGISTER_METHOD(SetSequenceTime);
	UI_REGISTER_METHOD(GetModel);
	UI_REGISTER_METHOD(SetModel);
	UI_REGISTER_METHOD(GetModelScale);
	UI_REGISTER_METHOD(SetModelScale);
	UI_REGISTER_METHOD(SetCamera);
	UI_REGISTER_METHOD(GetFogNear);
	UI_REGISTER_METHOD(SetFogNear);
	UI_REGISTER_METHOD(GetFogFar);
	UI_REGISTER_METHOD(SetFogFar);
	UI_REGISTER_METHOD(GetFogColor);
	UI_REGISTER_METHOD(SetFogColor);
	UI_REGISTER_METHOD(ClearFog);
	return ui_frame_vtable.register_methods(methods);
}

UI_INH0(frame, void, post_load);
UI_INH0(frame, void, register_in_interface);
UI_INH0(frame, void, unregister_in_interface);
UI_INH0(frame, void, eval_name);
UI_INH1(frame, void, on_click, enum gfx_mouse_button, button);
UI_INH0(frame, float, get_alpha);
UI_INH1(frame, void, set_alpha, float, alpha);
UI_INH1(frame, void, set_hidden, bool, hidden);
UI_INH2(frame, void, get_size, int32_t*, x, int32_t*, y);
UI_INH0(frame, void, set_dirty_coords);
UI_INH1(frame, void, on_mouse_move, struct gfx_pointer_event*, event);
UI_INH1(frame, void, on_mouse_down, struct gfx_mouse_event*, event);
UI_INH1(frame, void, on_mouse_up, struct gfx_mouse_event*, event);
UI_INH1(frame, void, on_mouse_scroll, struct gfx_scroll_event*, event);
UI_INH1(frame, bool, on_key_down, struct gfx_key_event*, event);
UI_INH1(frame, bool, on_key_up, struct gfx_key_event*, event);
UI_INH0(frame, struct ui_font_instance*, as_font_instance);
UI_INH0(frame, const char*, get_name);

const struct ui_object_vtable ui_model_vtable =
{
	UI_OBJECT_VTABLE("Model")
};
