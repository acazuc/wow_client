#include "frame.h"

#include "itf/interface.h"

#include "ui/layered_region.h"
#include "ui/backdrop.h"
#include "ui/texture.h"

#include "xml/attributes.h"
#include "xml/layers.h"
#include "xml/frame.h"

#include "lua/lua_script.h"

#include "wow_lua.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <string.h>
#include <stdlib.h>

#ifdef interface
# undef interface
#endif

MEMORY_DECL(UI);

#define LUA_METHOD_FRAME() LUA_METHOD(Frame, frame)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&frame->region)

static void set_script(struct ui_frame *frame, const char *name, struct lua_script *script, bool is_method);

static void attributes_destructor(jks_hmap_key_t key, void *value)
{
	mem_free(MEM_UI, key.ptr);
	luaL_unref(g_wow->interface->L, LUA_REGISTRYINDEX, *(int*)value);
}

static void scripts_destructor(jks_hmap_key_t key,  void *value)
{
	mem_free(MEM_UI, key.ptr);
	struct ui_frame_script *script = value;
	lua_script_delete(script->script);
}

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_region_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_frame *frame = (struct ui_frame*)object;
	UI_OBJECT->mask |= UI_OBJECT_frame;
	frame->backdrop = NULL;
	frame->strata = FRAMESTRATA_PARENT;
	frame->top_level = false;
	frame->movable = false;
	frame->resizable = false;
	frame->level = 0;
	frame->clamped_to_screen = false;
	frame->is_protected = false;
	frame->mouse_wheel_enabled = false;
	frame->id = 0;
	frame->scale = 1;
	if (parent && ui_object_get_name((struct ui_object*)parent))
	{
		frame->parent_name = mem_strdup(MEM_UI, ui_object_get_name((struct ui_object*)parent));
		if (!frame->parent_name)
			LOG_ERROR("failed to duplicate parent name");
	}
	else
	{
		frame->parent_name = NULL;
	}
	jks_hmap_init(&frame->attributes, sizeof(int), attributes_destructor, jks_hmap_hash_str, jks_hmap_cmp_str, &jks_hmap_memory_fn_UI);
	jks_hmap_init(&frame->scripts, sizeof(struct ui_frame_script), scripts_destructor, jks_hmap_hash_str, jks_hmap_cmp_str, &jks_hmap_memory_fn_UI);
	for (size_t i = 0; i < sizeof(frame->layers) / sizeof(*frame->layers); ++i)
		jks_array_init(&frame->layers[i], sizeof(struct ui_layered_region*), NULL, &jks_array_memory_fn_UI);
	jks_array_init(&frame->registered_events, sizeof(enum event_type), NULL, &jks_array_memory_fn_UI);
	jks_array_init(&frame->regions_to_load, sizeof(struct ui_region*), NULL, &jks_array_memory_fn_UI);
	jks_array_init(&frame->frames, sizeof(struct ui_frame*), NULL, &jks_array_memory_fn_UI);
	return true;
}

static void dtr(struct ui_object *object)
{
	struct ui_frame *frame = (struct ui_frame*)object;
	ui_object_unregister_in_interface(UI_OBJECT);
	for (size_t i = 0; i < frame->registered_events.size; ++i)
		interface_unregister_frame_event(UI_OBJECT->interface, frame, *JKS_ARRAY_GET(&frame->registered_events, i, enum event_type));
	for (size_t i = 0; i < sizeof(frame->layers) / sizeof(*frame->layers); ++i)
	{
		for (size_t j = 0; j < frame->layers[i].size; ++j)
			ui_object_delete(*JKS_ARRAY_GET(&frame->layers[i], j, struct ui_object*));
		jks_array_destroy(&frame->layers[i]);
	}
	jks_hmap_destroy(&frame->scripts);
	jks_hmap_destroy(&frame->attributes);
	jks_array_destroy(&frame->registered_events);
	jks_array_destroy(&frame->regions_to_load);
	for (size_t i = 0; i < frame->frames.size; ++i)
		ui_object_delete(*JKS_ARRAY_GET(&frame->frames, i, struct ui_object*));
	jks_array_destroy(&frame->frames);
	mem_free(MEM_UI, frame->parent_name);
	ui_backdrop_delete(frame->backdrop);
	frame->backdrop = NULL;
	ui_region_vtable.dtr(object);
}

static void load_xml(struct ui_object *object, const struct xml_layout_frame *layout_frame)
{
	ui_region_vtable.load_xml(object, layout_frame);
	struct ui_frame *frame = (struct ui_frame*)object;
	const struct xml_frame *xml_frame = xml_clayout_frame_as_frame(layout_frame);
	if (frame)
	{
		if (xml_frame->parent)
			frame->parent_name = mem_strdup(MEM_UI, xml_frame->parent);
		if (xml_frame->frame_strata)
		{
			if (!frame_strata_from_string(xml_frame->frame_strata, &frame->strata))
				LOG_ERROR("invalid strata: %s", xml_frame->frame_strata);
		}
		if (OPTIONAL_ISSET(xml_frame->backdrop))
		{
			if (!frame->backdrop)
				frame->backdrop = ui_backdrop_new(UI_OBJECT->interface, UI_REGION);
			ui_backdrop_load_xml(frame->backdrop, &OPTIONAL_GET(xml_frame->backdrop));
		}
		if (OPTIONAL_ISSET(xml_frame->alpha))
			UI_OBJECT->alpha = OPTIONAL_GET(xml_frame->alpha);
		if (OPTIONAL_ISSET(xml_frame->top_level))
			frame->top_level = OPTIONAL_GET(xml_frame->top_level);
		if (OPTIONAL_ISSET(xml_frame->movable))
			frame->movable = OPTIONAL_GET(xml_frame->movable);
		if (OPTIONAL_ISSET(xml_frame->resizable))
			frame->resizable = OPTIONAL_GET(xml_frame->resizable);
		if (OPTIONAL_ISSET(xml_frame->frame_level))
			frame->level = OPTIONAL_GET(xml_frame->frame_level);
		if (OPTIONAL_ISSET(xml_frame->id))
			frame->id = OPTIONAL_GET(xml_frame->id);
		if (OPTIONAL_ISSET(xml_frame->enable_mouse))
			UI_REGION->mouse_enabled = OPTIONAL_GET(xml_frame->enable_mouse);
		if (OPTIONAL_ISSET(xml_frame->enable_keyboard))
			UI_REGION->keyboard_enabled = OPTIONAL_GET(xml_frame->enable_keyboard);
		if (OPTIONAL_ISSET(xml_frame->clamped_to_screen))
			frame->clamped_to_screen = OPTIONAL_GET(xml_frame->clamped_to_screen);
		if (OPTIONAL_ISSET(xml_frame->is_protected))
			frame->is_protected = OPTIONAL_GET(xml_frame->is_protected);
		if (OPTIONAL_ISSET(xml_frame->hit_rect_insets))
			ui_inset_init_xml(&UI_REGION->hit_rect_insets, &OPTIONAL_GET(xml_frame->hit_rect_insets));
		if (OPTIONAL_ISSET(xml_frame->layers))
		{
			for (size_t i = 0; i < OPTIONAL_GET(xml_frame->layers).layers.size; ++i)
			{
				struct xml_layer *layer = *JKS_ARRAY_GET(&OPTIONAL_GET(xml_frame->layers).layers, i, struct xml_layer*);
				enum draw_layer draw_layer;
				if (layer->level)
				{
					if (!draw_layer_from_string(layer->level, &draw_layer))
					{
						LOG_ERROR("invalid draw layer: %s", layer->level);
						draw_layer = DRAWLAYER_ARTWORK;
					}
				}
				else
				{
					draw_layer = DRAWLAYER_ARTWORK;
				}
				for (size_t j = 0; j < layer->elements.size; ++j)
				{
					struct xml_layout_frame *child = *JKS_ARRAY_GET(&layer->elements, j, struct xml_layout_frame*);
					struct ui_region *region = xml_load_interface(child, UI_OBJECT->interface, UI_REGION);
					struct ui_layered_region *layered_region = ui_object_as_layered_region((struct ui_object*)region);
					if (!layered_region)
					{
						LOG_ERROR("created region isn't layered_region");
						ui_object_delete((struct ui_object*)region);
						continue;
					}
					ui_object_load_xml((struct ui_object*)layered_region, child);
					layered_region->draw_layer = draw_layer;
					if (!jks_array_push_back(&frame->layers[draw_layer], &layered_region))
						LOG_ERROR("failed to add region to layer");
					if (!jks_array_push_back(&frame->regions_to_load, &layered_region))
						LOG_ERROR("failed to add region to load");
				}
			}
		}
		if (OPTIONAL_ISSET(xml_frame->frames))
		{
			for (size_t i = 0; i < OPTIONAL_GET(xml_frame->frames).frames.size; ++i)
			{
				struct xml_layout_frame *child = *JKS_ARRAY_GET(&OPTIONAL_GET(xml_frame->frames).frames, i, struct xml_layout_frame*);
				struct ui_region *region = xml_load_interface(child, UI_OBJECT->interface, UI_REGION);
				struct ui_frame *child_frame = ui_object_as_frame((struct ui_object*)region);
				if (!child_frame)
				{
					LOG_ERROR("created frame isn't frame");
					ui_object_delete((struct ui_object*)region);
					continue;
				}
				ui_object_load_xml((struct ui_object*)child_frame, child);
				if (!jks_array_push_back(&frame->regions_to_load, &child_frame))
					LOG_ERROR("failed to add region to load");
			}
		}
		if (OPTIONAL_ISSET(xml_frame->scripts))
		{
			JKS_HMAP_FOREACH(iter, &OPTIONAL_GET(xml_frame->scripts).scripts)
			{
				const char *key = (const char*)jks_hmap_iterator_get_key(&iter).ptr;
				const char *val = *(const char**)jks_hmap_iterator_get_value(&iter);
				struct lua_script *lua_script = lua_script_new(UI_OBJECT->interface->L, val, key);
				if (!lua_script)
				{
					LOG_ERROR("failed to create lua script for script %s", key);
					continue;
				}
				set_script(frame, key, lua_script, false);
			}
		}
		if (OPTIONAL_ISSET(xml_frame->attributes))
		{
			struct lua_State *L = ui_object_get_L(UI_OBJECT);
			for (size_t i = 0; i < OPTIONAL_GET(xml_frame->attributes).attributes.size; ++i)
			{
				struct xml_attribute *attribute = *JKS_ARRAY_GET(&OPTIONAL_GET(xml_frame->attributes).attributes, i, struct xml_attribute*);
				if (!attribute->type || !attribute->type[0])
				{
					lua_pushstring(L, attribute->value);
				}
				else if (!strcmp(attribute->type, "boolean"))
				{
					lua_pushboolean(L, !strcmp(attribute->value, "true"));
				}
				else if (!strcmp(attribute->type, "number"))
				{
					char *endptr;
					float value = strtof(attribute->value, &endptr);
					if (endptr == attribute->value)
						value = 0;
					lua_pushnumber(L, value);
				}
				else if (!strcmp(attribute->type, "string"))
				{
					lua_pushstring(L, attribute->value);
				}
				else if (!strcmp(attribute->type, "nil"))
				{
					lua_pushnil(L);
				}
				else
				{
					LOG_ERROR("unknown attribute type: %s", attribute->type);
					lua_pushnil(L);
				}
				char *key = mem_strdup(MEM_UI, attribute->name);
				if (key)
				{
					int ref = luaL_ref(L, LUA_REGISTRYINDEX);
					jks_hmap_set(&frame->attributes, JKS_HMAP_KEY_PTR(key), &ref);
				}
				else
				{
					LOG_ERROR("failed to duplicate attribute name");
				}
			}
		}
	}
}

static void post_load(struct ui_object *object)
{
	struct ui_frame *frame = (struct ui_frame*)object;
	if (frame->parent_name && frame->parent_name[0] && (!UI_REGION->parent || !ui_object_get_name((struct ui_object*)UI_REGION->parent) || strcmp(ui_object_get_name((struct ui_object*)UI_REGION->parent), frame->parent_name)))
	{
		if (!(UI_REGION->parent = (struct ui_region*)interface_get_frame(UI_OBJECT->interface, frame->parent_name)))
			LOG_WARN("unknown parent: %s", frame->parent_name);
	}
	if (frame->strata == FRAMESTRATA_PARENT)
	{
		if (UI_REGION->parent)
		{
			struct ui_frame *prnt = ui_object_as_frame((struct ui_object*)UI_REGION->parent);
			if (prnt)
				frame->strata = prnt->strata;
			else
				frame->strata = FRAMESTRATA_BACKGROUND;
		}
		else
		{
			frame->strata = FRAMESTRATA_BACKGROUND;
		}
	}
	if (UI_REGION->parent)
		ui_frame_add_child(ui_object_as_frame((struct ui_object*)UI_REGION->parent), frame);
	ui_region_vtable.post_load(object);
	for (size_t i = 0; i < frame->regions_to_load.size; ++i)
	{
		struct ui_region *region = *JKS_ARRAY_GET(&frame->regions_to_load, i, struct ui_region*);
		ui_object_post_load((struct ui_object*)region);
	}
	jks_array_resize(&frame->regions_to_load, 0);
	jks_array_shrink(&frame->regions_to_load);
	ui_frame_execute_script(frame, "OnLoad", 0);
}

static void eval_name(struct ui_object *object)
{
	struct ui_frame *frame = (struct ui_frame*)object;
	ui_region_vtable.eval_name(object);
	for (size_t i = 0; i < frame->regions_to_load.size; ++i)
	{
		struct ui_region *region = *JKS_ARRAY_GET(&frame->regions_to_load, i, struct ui_region*);
		ui_object_eval_name((struct ui_object*)region);
	}
}

static void register_in_interface(struct ui_object *object)
{
	struct ui_frame *frame = (struct ui_frame*)object;
	if (UI_REGION->name)
		interface_register_frame(UI_OBJECT->interface, UI_REGION->name, frame);
	if (!UI_REGION->parent)
		interface_register_root_frame(UI_OBJECT->interface, frame);
	ui_region_vtable.register_in_interface(object);
}

static void unregister_in_interface(struct ui_object *object)
{
	struct ui_frame *frame = (struct ui_frame*)object;
	if (UI_REGION->name)
		interface_unregister_frame(UI_OBJECT->interface, UI_REGION->name);
	if (!UI_REGION->parent)
		interface_unregister_root_frame(UI_OBJECT->interface, frame);
	ui_region_vtable.unregister_in_interface(object);
}

static void set_script(struct ui_frame *frame, const char *name, struct lua_script *script, bool is_method)
{
	jks_hmap_iterator_t iter = jks_hmap_iterator_find(&frame->scripts, JKS_HMAP_KEY_PTR((void*)name));
	if (!jks_hmap_iterator_is_end(&frame->scripts, &iter))
	{
		struct ui_frame_script *frame_script = jks_hmap_iterator_get_value(&iter);
		lua_script_delete(frame_script->script);
		frame_script->script = script;
		return;
	}
	char *key = mem_strdup(MEM_UI, name);
	if (!key)
	{
		LOG_ERROR("failed to duplicate key");
		return;
	}
	struct ui_frame_script frame_script;
	frame_script.script = script;
	frame_script.is_method = is_method;
	jks_hmap_set(&frame->scripts, JKS_HMAP_KEY_PTR(key), &frame_script);
}

/*
 * lua_pushstring(L, argname);
 * lua_pushx(L, val);
 * ...
 * ui_frame_execute_script(frame, "On..", n);
 */
bool ui_frame_execute_script(struct ui_frame *frame, const char *name, size_t args)
{
	struct lua_State *L = ui_object_get_L(UI_OBJECT);
	bool ret = false;
	int prev_this;
	int prev_self;
	int stack_size = lua_gettop(L);
	int argc = 0;
	char argid[32];
	jks_hmap_iterator_t iter = jks_hmap_iterator_find(&frame->scripts, JKS_HMAP_KEY_PTR((void*)name));
	if (jks_hmap_iterator_is_end(&frame->scripts, &iter))
		goto popargs;
	struct ui_frame_script *script = jks_hmap_iterator_get_value(&iter);
#if 0
	LOG_WARN("executing script %s for object %s", name, UI_REGION->name);
#endif
	/* get ref of previous self & this
	 */
	lua_getglobal(L, "this");
	prev_this = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_getglobal(L, "self");
	prev_self = luaL_ref(L, LUA_REGISTRYINDEX);
	/* store previous global "argX"
	 */
	for (size_t i = 1; i <= args; ++i)
	{
		snprintf(argid, sizeof(argid), "arg%d", (int)i);
		lua_getglobal(L, argid);
	}
	/* set new self & this
	 */
	ui_push_lua_object(L, UI_OBJECT);
	lua_setglobal(L, "this");
	ui_push_lua_object(L, UI_OBJECT);
	lua_setglobal(L, "self");
	lua_script_push_pointer(script->script);
	if (script->is_method)
	{
		ui_push_lua_object(L, UI_OBJECT); /* this as first argument */
		argc++;
	}
	for (size_t i = 1; i <= args; ++i)
	{
		/* arg1name, arg1val, arg2name, arg2val, fn, frame */
		size_t base_idx = -1 - args * 2 + (i - 1) * 2 - argc - args;
		const char *argname = lua_tostring(L, base_idx);
		lua_pushvalue(L, base_idx + 1);
		if (argname)
		{
			lua_setglobal(L, argname);
			lua_pushvalue(L, base_idx + 1);
		}
		argc++;
		base_idx--;
		lua_pushvalue(L, base_idx + 1);
		snprintf(argid, sizeof(argid), "arg%d", (int)i);
		lua_setglobal(L, argid);
	}
	if (!lua_script_call(script->script, argc, 0))
		LOG_WARN("executed script %s for object %s %p", name, UI_REGION->name, frame);
	/* restore previous self & this
	 */
	lua_rawgeti(L, LUA_REGISTRYINDEX, prev_this);
	luaL_unref(L, LUA_REGISTRYINDEX, prev_this);
	lua_setglobal(L, "this");
	lua_rawgeti(L, LUA_REGISTRYINDEX, prev_self);
	luaL_unref(L, LUA_REGISTRYINDEX, prev_self);
	lua_setglobal(L, "self");
	ret = true;
	/* restore previous "argX"
	 */
	for (size_t i = args; i > 0; --i)
	{
		snprintf(argid, sizeof(argid), "arg%d", (int)i);
		lua_setglobal(L, argid);
	}
popargs:
	lua_settop(L, stack_size);
	lua_pop(L, args * 2);
	return ret;
}

static void on_mouse_move(struct ui_object *object, gfx_pointer_event_t *event)
{
	struct ui_frame *frame = (struct ui_frame*)object;
	for (size_t i = frame->frames.size; i > 0; --i)
	{
		struct ui_frame *child = *JKS_ARRAY_GET(&frame->frames, i - 1, struct ui_frame*);
		if (child->region.hidden)
			continue;
		ui_object_on_mouse_move((struct ui_object*)child, event);
	}
	bool hovered = UI_REGION->hovered;
	ui_region_vtable.on_mouse_move(object, event);
	if (UI_REGION->hovered != hovered)
	{
		struct lua_State *L = ui_object_get_L(UI_OBJECT);
		if (UI_REGION->hovered)
		{
			lua_pushstring(L, "motion");
			lua_pushboolean(L, true);
			ui_frame_execute_script(frame, "OnEnter", 1);
		}
		else
		{
			lua_pushstring(L, "motion");
			lua_pushboolean(L, true);
			ui_frame_execute_script(frame, "OnLeave", 1);
		}
	}
}

static void on_mouse_down(struct ui_object *object, gfx_mouse_event_t *event)
{
	struct ui_frame *frame = (struct ui_frame*)object;
	for (size_t i = frame->frames.size; i > 0; --i)
	{
		struct ui_frame *child = *JKS_ARRAY_GET(&frame->frames, i - 1, struct ui_frame*);
		if (child->region.hidden)
			continue;
		ui_object_on_mouse_down((struct ui_object*)child, event);
	}
	bool clicked = UI_REGION->clicked;
	ui_region_vtable.on_mouse_down(object, event);
	if (!clicked && UI_REGION->clicked)
	{
		struct lua_State *L = ui_object_get_L(UI_OBJECT);
		lua_pushstring(L, "button");
		lua_pushstring(L, lua_gfx_mouse_to_string(event->button));
		ui_frame_execute_script(frame, "OnMouseDown", 1);
	}
}

static void on_mouse_up(struct ui_object *object, gfx_mouse_event_t *event)
{
	struct ui_frame *frame = (struct ui_frame*)object;
	for (size_t i = frame->frames.size; i > 0; --i)
	{
		struct ui_frame *child = *JKS_ARRAY_GET(&frame->frames, i - 1, struct ui_frame*);
		if (child->region.hidden)
			continue;
		ui_object_on_mouse_up((struct ui_object*)child, event);
	}
	if (UI_REGION->hovered)
	{
		struct lua_State *L = ui_object_get_L(UI_OBJECT);
		lua_pushstring(L, "button");
		lua_pushstring(L, lua_gfx_mouse_to_string(event->button));
		ui_frame_execute_script(frame, "OnMouseUp", 1);
		if (UI_REGION->clicked)
		{
			lua_pushstring(L, "button");
			lua_pushstring(L, lua_gfx_mouse_to_string(event->button));
			lua_pushstring(L, "down");
			lua_pushboolean(L, false);
			ui_frame_execute_script(frame, "PreClick", 2);
			ui_object_on_click((struct ui_object*)frame, event->button);
			lua_pushstring(L, "button");
			lua_pushstring(L, lua_gfx_mouse_to_string(event->button));
			lua_pushstring(L, "down");
			lua_pushboolean(L, false);
			ui_frame_execute_script(frame, "PostClick", 2);
		}
	}
	ui_region_vtable.on_mouse_up(object, event);
}

static void on_mouse_scroll(struct ui_object *object, gfx_scroll_event_t *event)
{
	struct ui_frame *frame = (struct ui_frame*)object;
	for (size_t i = frame->frames.size; i > 0; --i)
	{
		struct ui_frame *child = *JKS_ARRAY_GET(&frame->frames, i - 1, struct ui_frame*);
		if (child->region.hidden)
			continue;
		ui_object_on_mouse_scroll((struct ui_object*)child, event);
	}
	if (!event->used && UI_REGION->hovered)
	{
		struct lua_State *L = ui_object_get_L(UI_OBJECT);
		lua_pushstring(L, "delta");
		lua_pushinteger(L, event->y);
		if (ui_frame_execute_script(frame, "OnMouseWheel", 1))
			event->used = true;
	}
	ui_region_vtable.on_mouse_scroll(object, event);
}

static bool on_key_down(struct ui_object *object, gfx_key_event_t *event)
{
	struct ui_frame *frame = (struct ui_frame*)object;
	for (size_t i = frame->frames.size; i > 0; --i)
	{
		struct ui_frame *child = *JKS_ARRAY_GET(&frame->frames, i - 1, struct ui_frame*);
		if (child->region.hidden)
			continue;
		if (ui_object_on_key_down((struct ui_object*)child, event))
			return true;
	}
	struct lua_State *L = ui_object_get_L(UI_OBJECT);
	lua_pushstring(L, "key");
	lua_pushinteger(L, event->key);
	if (ui_frame_execute_script(frame, "OnKeyDown", 1))
		return true;
	return ui_region_vtable.on_key_down(object, event);
}

static bool on_key_up(struct ui_object *object, gfx_key_event_t *event)
{
	struct ui_frame *frame = (struct ui_frame*)object;
	for (size_t i = frame->frames.size; i > 0; --i)
	{
		struct ui_frame *child = *JKS_ARRAY_GET(&frame->frames, i - 1, struct ui_frame*);
		if (child->region.hidden)
			continue;
		if (ui_object_on_key_up((struct ui_object*)child, event))
			return true;
	}
	struct lua_State *L = ui_object_get_L(UI_OBJECT);
	lua_pushstring(L, "key");
	lua_pushinteger(L, event->key);
	if (ui_frame_execute_script(frame, "OnKeyUp", 1))
		return true;
	return ui_region_vtable.on_key_up(object, event);
}

void ui_frame_render_layers(struct ui_frame *frame)
{
	for (size_t i = 0; i < sizeof(frame->layers) / sizeof(*frame->layers); ++i)
	{
		struct jks_array *layer = &frame->layers[i];
		for (size_t j = 0; j < layer->size; ++j)
		{
			struct ui_layered_region *child = *JKS_ARRAY_GET(layer, j, struct ui_layered_region*);
			if (child->region.hidden)
				continue;
			ui_object_render((struct ui_object*)child);
		}
	}
}

void ui_frame_render_childs(struct ui_frame *frame)
{
	for (size_t i = 0; i < frame->frames.size; ++i)
	{
		struct ui_frame *child = *JKS_ARRAY_GET(&frame->frames, i, struct ui_frame*);
		if (child->region.hidden)
			continue;
		ui_object_render((struct ui_object*)child);
	}
}

static void render(struct ui_object *object)
{
	struct ui_frame *frame = (struct ui_frame*)object;
	if (UI_REGION->hidden)
		return;
	struct lua_State *L = ui_object_get_L(UI_OBJECT);
	lua_pushstring(L, "elapsed");
	lua_pushnumber(L, (g_wow->frametime - g_wow->lastframetime) / 1000000000.);
	ui_frame_execute_script(frame, "OnUpdate", 1);
	if (frame->backdrop)
		ui_backdrop_render(frame->backdrop);
	ui_frame_render_layers(frame);
	ui_frame_render_childs(frame);
	ui_region_vtable.render(object);
}

static void on_click(struct ui_object *object, enum gfx_mouse_button button)
{
	struct ui_frame *frame = (struct ui_frame*)object;
	struct lua_State *L = ui_object_get_L(UI_OBJECT);
	lua_pushstring(L, "button");
	lua_pushstring(L, lua_gfx_mouse_to_string(button));
	lua_pushstring(L, "down");
	lua_pushboolean(L, false);
	ui_frame_execute_script(frame, "OnClick", 2);
	ui_object_vtable.on_click(object, button);
}

static void set_alpha(struct ui_object *object, float alpha)
{
	struct ui_frame *frame = (struct ui_frame*)object;
	if (UI_OBJECT->alpha == alpha)
		return;
	ui_object_vtable.set_alpha(object, alpha);
	ui_region_calc_effective_alpha(UI_REGION);
	for (size_t i = 0; i < frame->frames.size; ++i)
	{
		struct ui_frame *child = *JKS_ARRAY_GET(&frame->frames, i, struct ui_frame*);
		ui_region_calc_effective_alpha((struct ui_region*)child);
	}
}

static void update_visibility(struct ui_frame *frame, bool hidden)
{
	if (UI_REGION->hidden)
	{
		/* XXX: OnLeave */
		ui_frame_execute_script(frame, "OnHide", 0);
	}
	else
	{
		/* XXX: OnEnter */
		ui_frame_execute_script(frame, "OnShow", 0);
	}
	for (size_t i = 0; i < sizeof(frame->layers) / sizeof(*frame->layers); ++i)
	{
		struct jks_array *layer = &frame->layers[i];
		for (size_t j = 0; j < layer->size; ++j)
		{
			struct ui_layered_region *region = *JKS_ARRAY_GET(layer, j, struct ui_layered_region*);
			struct ui_frame *child = ui_object_as_frame((struct ui_object*)region);
			if (!child)
				continue;
			if (hidden && child->region.hidden)
				continue;
			update_visibility(child, hidden);
		}
	}
	for (size_t i = 0; i < frame->frames.size; ++i)
	{
		struct ui_frame *child = *JKS_ARRAY_GET(&frame->frames, i, struct ui_frame*);
		if (hidden && child->region.hidden)
			continue;
		update_visibility(child, hidden);
	}
}

static void set_hidden(struct ui_object *object, bool hidden)
{
	struct ui_frame *frame = (struct ui_frame*)object;
	if (UI_REGION->hidden == hidden)
		return;
	UI_REGION->hidden = hidden;
	update_visibility(frame, hidden);
}

void ui_frame_set_id(struct ui_frame *frame, int id)
{
	if (frame->id == id)
		return;
	frame->id = id;
	/* TODO event ? */
}

void ui_frame_set_strata(struct ui_frame *frame, enum frame_strata strata)
{
	if (frame->strata == strata)
		return;
	struct ui_frame *parent_frame = UI_REGION->parent ? ui_object_as_frame((struct ui_object*)UI_REGION->parent) : NULL;
	if (parent_frame)
		ui_frame_remove_child(parent_frame, frame);
	frame->strata = strata;
	if (parent_frame)
		ui_frame_add_child(parent_frame, frame);
}

bool ui_frame_has_registered_event(struct ui_frame *frame, enum event_type event)
{
	for (size_t i = 0; i < frame->registered_events.size; ++i)
	{
		if (*JKS_ARRAY_GET(&frame->registered_events, i, enum event_type) == event)
			return true;
	}
	return false;
}

void ui_frame_register_event(struct ui_frame *frame, enum event_type event)
{
	if (ui_frame_has_registered_event(frame, event))
		return;
	if (!jks_array_push_back(&frame->registered_events, &event))
		LOG_ERROR("failed to add registered event");
	interface_register_frame_event(UI_OBJECT->interface, frame, event);
}

void ui_frame_unregister_event(struct ui_frame *frame, enum event_type event)
{
	for (size_t i = 0; i < frame->registered_events.size; ++i)
	{
		if (*JKS_ARRAY_GET(&frame->registered_events, i, enum event_type) == event)
		{
			if (!jks_array_erase(&frame->registered_events, i))
				LOG_ERROR("failed to remove registered event");
			interface_unregister_frame_event(UI_OBJECT->interface, frame, event);
			return;
		}
	}
}

static bool compare(struct ui_frame *v1, struct ui_frame *v2)
{
	if (v1->strata < v2->strata)
		return true;
	if (v1->strata > v2->strata)
		return false;
	if (v1->level < v2->level)
		return true;
	if (v1->level > v2->level)
		return false;
	return false;
}

void ui_frame_add_child(struct ui_frame *frame, struct ui_frame *child)
{
	for (size_t i = 0; i < frame->frames.size; ++i)
	{
		struct ui_frame *tmp = *JKS_ARRAY_GET(&frame->frames, i, struct ui_frame*);
		if (compare(child, tmp))
		{
			if (!jks_array_push(&frame->frames, &child, i))
				LOG_ERROR("failed to push frame");
			return;
		}
	}
	if (!jks_array_push_back(&frame->frames, &child))
		LOG_ERROR("failed to push frame");
}

void ui_frame_remove_child(struct ui_frame *frame, struct ui_frame *child)
{
	for (size_t i = 0; i < frame->frames.size; ++i)
	{
		struct ui_frame *tmp = *JKS_ARRAY_GET(&frame->frames, i, struct ui_frame*);
		if (tmp == child)
		{
			jks_array_erase(&frame->frames, i);
			return;
		}
	}
}

static void set_dirty_coords(struct ui_object *object)
{
	struct ui_frame *frame = (struct ui_frame*)object;
	ui_region_vtable.set_dirty_coords(object);
	for (size_t i = 0; i < sizeof(frame->layers) / sizeof(*frame->layers); ++i)
	{
		const struct jks_array *layer = &frame->layers[i];
		for (size_t j = 0; j < layer->size; ++j)
		{
			struct ui_layered_region *region = *JKS_ARRAY_GET(layer, j, struct ui_layered_region*);
			ui_object_set_dirty_coords((struct ui_object*)region);
		}
	}
	for (size_t i = 0; i < frame->frames.size; ++i)
	{
		struct ui_frame *child = *JKS_ARRAY_GET(&frame->frames, i, struct ui_frame*);
		ui_object_set_dirty_coords((struct ui_object*)child);
	}
	if (frame->backdrop)
		frame->backdrop->dirty_size = true;
}

static int lua_GetScript(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 2)
		return luaL_error(L, "Usage: Frame:GetScript(\"script\")");
	if (!lua_isstring(L, 2))
		return luaL_argerror(L, 2, "string expected");
	const char *name = lua_tostring(L, 2);
	if (!name)
		return 0;
	jks_hmap_iterator_t iter = jks_hmap_iterator_find(&frame->scripts, JKS_HMAP_KEY_PTR((void*)name));
	if (jks_hmap_iterator_is_end(&frame->scripts, &iter))
		return 0;
	struct ui_frame_script *script = jks_hmap_iterator_get_value(&iter);
	lua_script_push_pointer(script->script);
	return 1;
}

static int lua_SetScript(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc < 3)
		return luaL_error(L, "Usage: Frame:SetScript(\"script\", function)");
	if (!lua_isstring(L, 2))
		return luaL_argerror(L, 2, "string expected");
	const char *name = lua_tostring(L, 2);
	if (!name)
		return 0;
	if (!strcmp(name, "OnAttributeChanged"))
		LOG_INFO("setting OnAttributeChanged to %p", frame);
	if (lua_isnil(L, 3))
	{
		jks_hmap_erase(&frame->scripts, JKS_HMAP_KEY_PTR((void*)name));
		return 0;
	}
	if (!lua_isfunction(L, 3))
		return luaL_argerror(L, 3, "function expected");
	const void *fn = lua_topointer(L, 3);
	if (!fn)
		return 0;
	int fnref = luaL_ref(L, LUA_REGISTRYINDEX);
	struct lua_script *script = lua_script_new_ref(UI_OBJECT->interface->L, fnref);
	set_script(frame, name, script, true);
	return 0;
}

static int lua_HasScript(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 2)
		return luaL_error(L, "Usage: Frame:HasScript(\"script\")");
	if (!lua_isstring(L, 2))
		return luaL_argerror(L, 2, "string expected");
	const char *name = lua_tostring(L, 2);
	if (!name)
		return 0;
	jks_hmap_iterator_t iter = jks_hmap_iterator_find(&frame->scripts, JKS_HMAP_KEY_PTR((void*)name));
	if (jks_hmap_iterator_is_end(&frame->scripts, &iter))
		lua_pushboolean(L, false);
	else
		lua_pushboolean(L, true);
	return 1;
}

static int lua_CreateFontString(lua_State *L)
{
	LUA_METHOD_FRAME();
	const char *name;
	const char *layer;
	const char *inherits;
	if (argc >= 2)
	{
		name = lua_tostring(L, 2);
		if (argc >= 3)
		{
			layer = lua_tostring(L, 3);
			if (argc >= 4)
			{
				inherits = lua_tostring(L, 4);
			}
			else
			{
				inherits = NULL;
			}
		}
		else
		{
			layer = NULL;
		}
	}
	else
	{
		name = NULL;
	}
	if (argc >= 5)
		return luaL_error(L, "Usage: Frame:CreateFontString([\"name\" [, \"layer\" [, \"inherits\"]]])");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_GetBackdropColor(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: Frame:GetBackdropColor()");
	ui_push_lua_color(L, frame->backdrop ? &frame->backdrop->color : NULL);
	return 4;
}

static int lua_SetBackdropColor(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc < 4 || argc > 5)
		return luaL_error(L, "Usage: Frame:SetBackdropColor(r, g, b [, a])");
	struct ui_color color;
	if (!ui_get_lua_color(L, 2, &color))
		return luaL_argerror(L, 2, "color expected");
	if (!frame->backdrop)
	{
		frame->backdrop = ui_backdrop_new(UI_OBJECT->interface, UI_REGION);
		if (!frame->backdrop)
		{
			LOG_ERROR("failed to create backdrop");
			return 0;
		}
	}
	frame->backdrop->color = color;
	return 0;
}

static int lua_GetBackdropBorderColor(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: Frame:GetBackdropBorderColor()");
	ui_push_lua_color(L, frame->backdrop ? &frame->backdrop->border_color : NULL);
	return 4;
}

static int lua_SetBackdropBorderColor(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc < 4 || argc > 5)
		return luaL_error(L, "Usage: Frame:SetBackdropBorderColor(r, g, b [, a])");
	struct ui_color color;
	if (!ui_get_lua_color(L, 2, &color))
		return luaL_argerror(L, 2, "color expected");
	if (!frame->backdrop)
	{
		frame->backdrop = ui_backdrop_new(UI_OBJECT->interface, UI_REGION);
		if (!frame->backdrop)
		{
			LOG_ERROR("failed to create backdrop");
			return 0;
		}
	}
	frame->backdrop->border_color = color;
	return 0;
}

static int lua_GetFrameLevel(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: Frame:GetFrameLevel()");
	lua_pushinteger(L, frame->level);
	return 1;
}

static int lua_SetFrameLevel(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 2)
		return luaL_error(L, "Usage: Frame:SetFrameLevel(\"level\")");
	if (!lua_isnumber(L, 2))
	{
		char tmp[256];
		snprintf(tmp, sizeof(tmp), "number expected, got %s", lua_typename(L, lua_type(L, 2)));
		return luaL_argerror(L, 2, tmp);
	}
	int level = lua_tonumber(L, 2);
	if (level != frame->level)
	{
		struct ui_frame *parent_frame = UI_REGION->parent ? ui_object_as_frame((struct ui_object*)UI_REGION->parent) : NULL;
		if (parent_frame)
			ui_frame_remove_child(parent_frame, frame);
		frame->level = lua_tonumber(L, 2);
		if (parent_frame)
			ui_frame_add_child(parent_frame, frame);
	}
	return 0;
}

static int lua_GetAttribute(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 2 && argc != 4)
		return luaL_error(L, "Usage: Frame:GetAttribute([prefix], name, [suffix])");
	const char *prefix;
	const char *suffix;
	const char *key;
	if (argc == 2)
	{
		if (!lua_isstring(L, 2))
			return luaL_argerror(L, 2, "string expected");
		key = lua_tostring(L, 2);
		prefix = NULL;
		suffix = NULL;
	}
	else
	{
		if (!lua_isstring(L, 2))
			return luaL_argerror(L, 2, "string expected");
		if (!lua_isstring(L, 3))
			return luaL_argerror(L, 3, "string expected");
		if (!lua_isstring(L, 4))
			return luaL_argerror(L, 4, "string expected");
		prefix = lua_tostring(L, 2);
		key = lua_tostring(L, 3);
		suffix = lua_tostring(L, 4);
	}
	/* TODO use prefix / suffix */
	jks_hmap_iterator_t iter = jks_hmap_iterator_find(&frame->attributes, JKS_HMAP_KEY_PTR((void*)key));
	if (jks_hmap_iterator_is_end(&frame->attributes, &iter))
	{
		/* LOG_WARN("getting inexisting attribute: %s", key); */
		return 0;
	}
	lua_rawgeti(L, LUA_REGISTRYINDEX, *(int*)jks_hmap_iterator_get_value(&iter));
	return 1;
}

static int lua_SetAttribute(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 3)
		return luaL_error(L, "Usage: Frame:SetAttribute(\"name\", value)");
	if (!lua_isstring(L, 2))
	{
		char msg[256];
		snprintf(msg, sizeof(msg), "string expected, got %s", lua_typename(L, lua_type(L, 2)));
		return luaL_argerror(L, 2, msg);
	}
	const char *key = lua_tostring(L, 2);
	if (!key)
		return luaL_argerror(L, 2, "lua_tostring failed");
	if (lua_isnil(L, 3))
	{
		jks_hmap_erase(&frame->attributes, JKS_HMAP_KEY_PTR((void*)key));
		lua_pushstring(L, "name");
		lua_pushstring(L, key);
		lua_pushstring(L, "value");
		lua_pushnil(L);
	}
	else
	{
		char *key_dup = mem_strdup(MEM_UI, key);
		if (!key_dup)
		{
			LOG_ERROR("failed to duplicate key");
			return 0;
		}
		int ref = luaL_ref(L, LUA_REGISTRYINDEX);
		jks_hmap_set(&frame->attributes, JKS_HMAP_KEY_PTR(key_dup), &ref);
		lua_pushstring(L, "name");
		lua_pushstring(L, key);
		lua_pushstring(L, "value");
		lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
	}
	ui_frame_execute_script(frame, "OnAttributeChanged", 2);
	return 0;
}

static int lua_RegisterEvent(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 2)
		return luaL_error(L, "Usage: Frame:RegisterEvent(\"event\")");
	if (!lua_isstring(L, 2))
		return luaL_argerror(L, 2, "string expected");
	const char *evt = lua_tostring(L, 2);
	enum event_type type;
	if (!event_type_from_string(evt, &type))
	{
		char tmp[256];
		snprintf(tmp, sizeof(tmp), "invalid event: %s", evt);
		return luaL_argerror(L, 2, tmp);
	}
	ui_frame_register_event(frame, type);
	return 0;
}

static int lua_UnregisterEvent(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 2)
		return luaL_error(L, "Usage: Frame:UnregisterEvent(\"event\")");
	if (!lua_isstring(L, 2))
		return luaL_argerror(L, 2, "string expected");
	const char *evt = lua_tostring(L, 2);
	enum event_type type;
	if (!event_type_from_string(evt, &type))
	{
		char tmp[256];
		snprintf(tmp, sizeof(tmp), "invalid event: %s", evt);
		return luaL_argerror(L, 2, tmp);
	}
	ui_frame_unregister_event(frame, type);
	return 0;
}

static int lua_IsEventRegistered(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 2)
		return luaL_error(L, "Usage: Frame:IsEventRegistered(\"event\")");
	if (!lua_isstring(L, 2))
		return luaL_argerror(L, 2, "string expected");
	const char *event = lua_tostring(L, 2);
	enum event_type type;
	if (!event_type_from_string(event, &type))
	{
		char tmp[256];
		snprintf(tmp, sizeof(tmp), "invalid event: %s", event);
		return luaL_argerror(L, 2, tmp);
	}
	lua_pushboolean(L, ui_frame_has_registered_event(frame, type));
	return 1;
}

static int lua_RegisterAllEvents(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: Frame:RegisterAllEvents()");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_UnregisterAllEvents(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: Frame:UnregisterAllEvents()");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_GetID(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: Frame:GetID()");
	lua_pushinteger(L, frame->id);
	return 1;
}

static int lua_SetID(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 2)
		return luaL_error(L, "Usage: Frame:SetID(id)");
	lua_Integer id = luaL_checkinteger(L, 2);
	ui_frame_set_id(frame, id);
	return 0;
}

static int lua_GetScale(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: Frame:GetScale()");
	lua_pushnumber(L, frame->scale);
	return 1;
}

static int lua_SetScale(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 2)
		return luaL_error(L, "Usage: Frame:SetScale(scale)");
	if (!lua_isnumber(L, 2))
		return luaL_argerror(L, 2, "number expected");
	frame->scale = lua_tonumber(L, 2);
	return 0;
}

static int lua_GetEffectiveScale(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: Frame:GetEffectiveScale()");
	lua_pushnumber(L, frame->scale);
	return 1;
}

static int lua_RegisterForDrag(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc < 2)
		return luaL_error(L, "Usage: Frame:RegisterForDrag(\"button\")");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_EnableMouse(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 2)
		return luaL_error(L, "Usage: Frame:EnableMouse(enabled)");
	UI_REGION->mouse_enabled = lua_tointeger(L, 2);
	return 0;
}

static int lua_IsMouseEnabled(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: Frame:IsMouseEnable()");
	lua_pushboolean(L, UI_REGION->mouse_enabled);
	return 1;
}

static int lua_EnableKeyboard(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 2)
		return luaL_error(L, "Usage: Frame:EnableKeyboard(enabled)");
	UI_REGION->keyboard_enabled = lua_tointeger(L, 2);
	return 0;
}

static int lua_IsKeyboardEnabled(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: Frame:IsKeyboardEnabled()");
	lua_pushboolean(L, UI_REGION->keyboard_enabled);
	return 1;
}

static int lua_EnableMouseWheel(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc > 2)
		return luaL_error(L, "Usage: Frame:EnableMouseWheel([enabled])");
	if (argc == 2)
	{
		frame->mouse_wheel_enabled = lua_tointeger(L, 2);
	}
	else
	{
		frame->mouse_wheel_enabled = true;
	}
	return 0;
}

static int lua_IsMouseWheelEnabled(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: Frame:IsMouseWheelEnabled()");
	lua_pushboolean(L, frame->mouse_wheel_enabled);
	return 1;
}

static int lua_CreateTexture(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc < 1 || argc > 4)
		return luaL_error(L, "Usage: CreateTexture([\"name\" [, \"layer\" [, \"inherits\"]]])");
	const char *name = NULL;
	enum draw_layer layer;
	if (argc >= 2)
	{
		if (!lua_isstring(L, 2))
			return luaL_argerror(L, 2, "string expected");
		name = lua_tostring(L, 2);
		if (!name)
			return 0;
	}
	if (argc >= 3)
	{
		if (!lua_isstring(L, 3))
			return luaL_argerror(L, 3, "string expected");
		const char *str = lua_tostring(L, 3);
		if (!str)
			return 0;
		if (!draw_layer_from_string(str, &layer))
		{
			char tmp[128];
			snprintf(tmp, sizeof(tmp), "invalid draw layer: %s", str);
			return luaL_argerror(L, 3, tmp);
		}
	}
	else
	{
		layer = DRAWLAYER_ARTWORK;
	}
	struct jks_array inherits; /* struct xml_layout_frame* */
	jks_array_init(&inherits, sizeof(struct xml_layout_frame*), NULL, &jks_array_memory_fn_UI);
	if (argc >= 4)
	{
		if (!lua_isstring(L, 4))
		{
			jks_array_destroy(&inherits);
			return luaL_argerror(L, 4, "string expected");
		}
		const char *str = lua_tostring(L, 4);
		if (str)
		{
			struct xml_layout_frame *layout_frame = interface_get_virtual_layout_frame(UI_OBJECT->interface, str);
			if (layout_frame)
			{
				if (!jks_array_push_back(&inherits, &layout_frame))
					LOG_ERROR("failed to push inherit");
			}
		}
	}
	struct ui_texture *texture = ui_texture_new(UI_OBJECT->interface, name, UI_REGION);
	if (!texture)
	{
		jks_array_destroy(&inherits);
		return 0;
	}
	for (size_t i = 0; i < inherits.size; ++i)
		ui_object_load_xml(UI_OBJECT, *JKS_ARRAY_GET(&inherits, i, struct xml_layout_frame*));
	ui_object_eval_name((struct ui_object*)texture);
	ui_object_post_load((struct ui_object*)texture);
	if (!jks_array_push_back(&frame->layers[layer], &texture))
		LOG_ERROR("failed to push region to layer");
	ui_push_lua_object(L, (struct ui_object*)texture);
	jks_array_destroy(&inherits);
	return 1;
}

static int lua_Raise(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: Frame:Raise()");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_Lower(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: Frame:Lower()");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_IsUserPlaced(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: Frame:IsUserPlaced()");
	lua_pushboolean(L, false);
	return 1;
}

static int lua_SetBackdrop(lua_State *L)
{
	LUA_METHOD_FRAME();
	if (argc == 1)
	{
		ui_backdrop_delete(frame->backdrop);
		frame->backdrop = NULL;
		return 0;
	}
	LUA_UNIMPLEMENTED_METHOD();
	/* XXX */
	return 0;
}

static int lua_HookScript(lua_State *L)
{
	LUA_METHOD_FRAME();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_StopMovingOrSizing(lua_State *L)
{
	LUA_METHOD_FRAME();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_SetMovable(lua_State *L)
{
	LUA_METHOD_FRAME();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_SetFrameStrata(lua_State *L)
{
	LUA_METHOD_FRAME();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_SetToplevel(lua_State *L)
{
	LUA_METHOD_FRAME();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static bool register_methods(struct jks_array *methods)
{
	/*
	   CreateTitleRegion
	   DisableDrawLayer
	   EnableDrawLayer
	   GetBackdrop
	   GetChildren
	   GetClampRectInsets
	   GetEffectiveAlpha
	   GetFrameStrata
	   GetFrameType
	   GetHitRectInsets
	   GetMaxResize
	   GetMinResize
	   GetNumChildren
	   GetNumRegions
	   GetRegions
	   GetTitleRegion
	   IsClampedToScreen
	   IsFrameType
	   IsMovable
	   IsResizable
	   IsToplevel
	   SetClampedToScreen
	   SetClampRectInsets
	   SetHitRectInsets
	   SetMaxResize
	   SetMinResize
	   SetResizable
	   SetUserPlaced
	   StartMoving
	   StartSizing
	   UnregisterAllEvents
	 */
	UI_REGISTER_METHOD(GetScript);
	UI_REGISTER_METHOD(SetScript);
	UI_REGISTER_METHOD(HasScript);
	UI_REGISTER_METHOD(CreateFontString);
	UI_REGISTER_METHOD(GetBackdropColor);
	UI_REGISTER_METHOD(SetBackdropColor);
	UI_REGISTER_METHOD(GetBackdropBorderColor);
	UI_REGISTER_METHOD(SetBackdropBorderColor);
	UI_REGISTER_METHOD(GetFrameLevel);
	UI_REGISTER_METHOD(SetFrameLevel);
	UI_REGISTER_METHOD(GetAttribute);
	UI_REGISTER_METHOD(SetAttribute);
	UI_REGISTER_METHOD(RegisterEvent);
	UI_REGISTER_METHOD(UnregisterEvent);
	UI_REGISTER_METHOD(IsEventRegistered);
	UI_REGISTER_METHOD(RegisterAllEvents);
	UI_REGISTER_METHOD(UnregisterAllEvents);
	UI_REGISTER_METHOD(GetID);
	UI_REGISTER_METHOD(SetID);
	UI_REGISTER_METHOD(GetScale);
	UI_REGISTER_METHOD(SetScale);
	UI_REGISTER_METHOD(GetEffectiveScale);
	UI_REGISTER_METHOD(RegisterForDrag);
	UI_REGISTER_METHOD(EnableMouse);
	UI_REGISTER_METHOD(IsMouseEnabled);
	UI_REGISTER_METHOD(EnableKeyboard);
	UI_REGISTER_METHOD(IsKeyboardEnabled);
	UI_REGISTER_METHOD(EnableMouseWheel);
	UI_REGISTER_METHOD(IsMouseWheelEnabled);
	UI_REGISTER_METHOD(CreateTexture);
	UI_REGISTER_METHOD(Raise);
	UI_REGISTER_METHOD(Lower);
	UI_REGISTER_METHOD(IsUserPlaced);
	UI_REGISTER_METHOD(SetBackdrop);
	UI_REGISTER_METHOD(HookScript);
	UI_REGISTER_METHOD(StopMovingOrSizing);
	UI_REGISTER_METHOD(SetMovable);
	UI_REGISTER_METHOD(SetFrameStrata);
	UI_REGISTER_METHOD(SetToplevel);
	return ui_region_vtable.register_methods(methods);
}

UI_INH0(region, float, get_alpha);
UI_INH2(region, void, get_size, int32_t*, x, int32_t*, y);
UI_INH0(region, struct ui_font_instance*, as_font_instance);
UI_INH0(region, const char*, get_name);

const struct ui_object_vtable ui_frame_vtable =
{
	UI_OBJECT_VTABLE("Frame")
};
