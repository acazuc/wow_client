#include "interface.h"

#include "itf/addon.h"
#include "itf/font.h"

#include "lua/lua_script.h"
#include "lua/functions.h"

#include "ui/edit_box.h"
#include "ui/frame.h"

#include "xml/ui.h"

#include "font/model.h"
#include "font/font.h"

#include "snd/snd.h"

#include "shaders.h"
#include "wow_lua.h"
#include "memory.h"
#include "cache.h"
#include "log.h"
#include "wow.h"
#include "blp.h"
#include "bc.h"

#include <libxml/tree.h>

#include <gfx/window.h>
#include <gfx/device.h>

#include <libwow/mpq.h>
#include <libwow/blp.h>

#include <inttypes.h>
#include <string.h>
#include <dirent.h>
#include <math.h>

#define GLUE_WIDTH 1370
#define GLUE_HEIGHT 768

MEMORY_DECL(UI);

static bool load_src_addon(struct interface *interface, const char *name);
static bool add_addons(struct interface *interface);
static bool load_addons(struct interface *interface);
static gfx_cursor_t *load_cursor(const char *file);

static const register_lua_functions_t g_init_functions[] =
{
	register_misc_functions,
	register_std_functions,
	register_objects_functions,
	register_unit_functions,
	register_pet_functions,
	register_guild_functions,
	register_system_functions,
	register_voice_functions,
	register_party_functions,
	register_input_functions,
	register_audio_functions,
	register_loot_functions,
	register_secure_functions,
	register_battle_ground_functions,
	register_chat_functions,
	register_addon_functions,
	register_lfg_functions,
	register_skill_functions,
	register_quest_functions,
	register_world_map_functions,
	register_arena_functions,
	register_glue_functions,
	register_social_functions,
	register_kb_functions,
	register_spell_functions,
	register_pvp_functions,
	register_talent_functions,
	register_auction_functions,
	register_macro_functions,
	register_inbox_functions,
	register_trade_functions,
	register_combat_log_functions,
	register_craft_functions,
	register_bindings_functions,
	register_taxi_functions,
};

static const gfx_input_layout_bind_t g_binds[] =
{
	{GFX_ATTR_R32G32_FLOAT      , sizeof(float) * 8, sizeof(float) * 0},
	{GFX_ATTR_R32G32B32A32_FLOAT, sizeof(float) * 8, sizeof(float) * 2},
	{GFX_ATTR_R32G32_FLOAT      , sizeof(float) * 8, sizeof(float) * 6},
};

static void *lua_allocator(void *ud, void *ptr, size_t osize, size_t nsize)
{
	(void)ud;
	(void)osize;
	if (nsize == 0)
	{
		mem_free(MEM_LUA, ptr);
		return NULL;
	}
	return mem_realloc(MEM_LUA, ptr, nsize);
}

static void layout_frames_hmap_destructor(jks_hmap_key_t key, void *val)
{
	mem_free(MEM_UI, key.ptr);
	(void)val;
}

static void regions_hmap_destructor(jks_hmap_key_t key, void *val)
{
	mem_free(MEM_UI, key.ptr);
	(void)val;
}

static void frames_hmap_destructor(jks_hmap_key_t key, void *val)
{
	mem_free(MEM_UI, key.ptr);
	(void)val;
}

static void fonts_hmap_destructor(jks_hmap_key_t key, void *val)
{
	mem_free(MEM_UI, key.ptr);
	(void)val;
}

static void render_font_model_hmap_destructor(jks_hmap_key_t key, void *val)
{
	struct interface_font_model *font_model = val;
	mem_free(MEM_UI, key.ptr);
	jks_hmap_destroy(&font_model->fonts);
	cache_unref_by_ref_font(g_wow->cache, font_model->model);
}

static void render_font_hmap_destructor(jks_hmap_key_t key, void *val)
{
	(void)key;
	struct interface_font *font = val;
	font_delete(font->font);
	font_delete(font->outline_normal);
	font_delete(font->outline_thick);
	font_atlas_delete(font->atlas);
}

static void xml_ui_dtr(void *ptr)
{
	xml_element_delete(*(struct xml_element**)ptr);
}

static void addon_dtr(void *ptr)
{
	addon_delete(*(struct addon**)ptr);
}

static const char *cursor_files[CURSOR_LAST] =
{
	"UnableQuestTurnin",
	"UnableQuestRepeatable",
	"UnableQuest",
	"UnableInnkeeper",
	"UnableSkinAlliance",
	"UnableSkinHorde",
	"UnableItem",
	"UnableRepairNpc",
	"UnableRepair",
	"UnableLootAll",
	"UnableMail",
	"UnablePickLock",
	"UnableGatherHerbs",
	"UnableSkin",
	"UnableMine",
	"UnableTrainer",
	"UnableTaxi",
	"UnablePickup",
	"UnableInspect",
	"UnableSpeak",
	"UnableInteract",
	"UnableAttack",
	"UnableBuy",
	"UnableCast",
	"UnablePoint",
	"QuestTurnin",
	"QuestRepeatable",
	"Quest",
	"Innkeeper",
	"SkinAlliance",
	"SkinHorde",
	"Item",
	"RepairNpc",
	"Repair",
	"LootAll",
	"Mail",
	"PickLock",
	"GatherHerbs",
	"Skin",
	"Mine",
	"Trainer",
	"Taxi",
	"Pickup",
	"Inspect",
	"Speak",
	"Interact",
	"Attack",
	"Buy",
	"Cast",
	"Point"
};

struct interface *interface_new(void)
{
	struct interface *interface = mem_zalloc(MEM_UI, sizeof(*interface));
	if (!interface)
		return NULL;
	interface->L = lua_newstate(lua_allocator, NULL);
	if (!interface->L)
	{
		LOG_ERROR("failed to create lua state");
		mem_free(MEM_UI, interface);
		return NULL;
	}
	lua_checkstack(interface->L, 1024 * 16);
	luaL_openlibs(interface->L);
	interface->is_gluescreen = false;
	lua_pushnumber(interface->L, M_PI);
	lua_setglobal(interface->L, "PI");
	for (char c = 'A'; c <= 'Z'; ++c)
	{
		static char key_text[6] = "KEY_A";
		key_text[4] = c;
		lua_pushstring(interface->L, &key_text[4]);
		lua_setglobal(interface->L, key_text);
	}
	for (char c = '0'; c <= '9'; ++c)
	{
		static char key_text[6] = "KEY_0";
		key_text[4] = c;
		lua_pushstring(interface->L, &key_text[4]);
		lua_setglobal(interface->L, key_text);
	}
	for (size_t i = 0; i < sizeof(g_init_functions) / sizeof(*g_init_functions); ++i)
		g_init_functions[i](interface->L);
	interface->width = GLUE_WIDTH;
	interface->height = GLUE_HEIGHT;
	interface->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	interface->rasterizer_state = GFX_RASTERIZER_STATE_INIT();
	interface->depth_stencil_state = GFX_DEPTH_STENCIL_STATE_INIT();
	interface->input_layout = GFX_INPUT_LAYOUT_INIT();
	for (size_t i = 0; i < sizeof(interface->blend_states) / sizeof(*interface->blend_states); ++i)
		interface->blend_states[i] = GFX_BLEND_STATE_INIT();
	for (enum interface_blend_state blend = 0; blend < INTERFACE_BLEND_LAST; ++blend)
		interface->pipeline_states[blend] = GFX_PIPELINE_STATE_INIT();
	for (size_t i = 0; i < sizeof(interface->root_frames) / sizeof(*interface->root_frames); ++i)
		jks_array_init(&interface->root_frames[i], sizeof(struct ui_frame*), NULL, &jks_array_memory_fn_UI);
	for (size_t i = 0; i < sizeof(interface->frames_events) / sizeof(*interface->frames_events); ++i)
		jks_array_init(&interface->frames_events[i], sizeof(struct ui_frame*), NULL, &jks_array_memory_fn_UI);
	for (size_t i = 0; i < sizeof(interface->cursors) / sizeof(*interface->cursors); ++i)
	{
		char filename[256];
		snprintf(filename, sizeof(filename), "Interface/Cursor/%s.blp", cursor_files[i]);
		normalize_blp_filename(filename, sizeof(filename));
		interface->cursors[i] = load_cursor(filename);
		if (!interface->cursors[i])
			LOG_WARN("can't create cursor %s", cursor_type_to_string(i));
	}
	jks_array_init(&interface->ext_addons, sizeof(struct addon*), NULL, &jks_array_memory_fn_UI);
	jks_array_init(&interface->xml_ui, sizeof(struct xml_ui*), xml_ui_dtr, &jks_array_memory_fn_UI);
	jks_array_init(&interface->addons, sizeof(struct addon*), addon_dtr, &jks_array_memory_fn_UI);
	jks_hmap_init(&interface->virtual_layout_frames, sizeof(struct xml_layout_frame*), layout_frames_hmap_destructor, jks_hmap_hash_str, jks_hmap_cmp_str, &jks_hmap_memory_fn_UI);
	jks_hmap_init(&interface->render_fonts, sizeof(struct interface_font_model), render_font_model_hmap_destructor, jks_hmap_hash_str, jks_hmap_cmp_str, &jks_hmap_memory_fn_UI);
	jks_hmap_init(&interface->regions, sizeof(struct ui_region*), regions_hmap_destructor, jks_hmap_hash_str, jks_hmap_cmp_str, &jks_hmap_memory_fn_UI);
	jks_hmap_init(&interface->frames, sizeof(struct ui_frame*), frames_hmap_destructor, jks_hmap_hash_str, jks_hmap_cmp_str, &jks_hmap_memory_fn_UI);
	jks_hmap_init(&interface->fonts, sizeof(struct ui_font*), fonts_hmap_destructor, jks_hmap_hash_str, jks_hmap_cmp_str, &jks_hmap_memory_fn_UI);
	gfx_set_cursor(g_wow->window, interface->cursors[CURSOR_POINT]);
	interface->white_pixel = GFX_TEXTURE_INIT();
	static const uint8_t data[4] = {0xff, 0xff, 0xff, 0xff};
	gfx_create_texture(g_wow->device, &interface->white_pixel, GFX_TEXTURE_2D, GFX_B8G8R8A8, 1, 1, 1, 0);
	gfx_set_texture_data(&interface->white_pixel, 0, 0, 1, 1, 0, 4, data);
	return interface;
}

void interface_delete(struct interface *interface)
{
	gfx_delete_depth_stencil_state(g_wow->device, &interface->depth_stencil_state);
	gfx_delete_rasterizer_state(g_wow->device, &interface->rasterizer_state);
	gfx_delete_attributes_state(g_wow->device, &interface->attributes_state);
	gfx_delete_input_layout(g_wow->device, &interface->input_layout);
	for (size_t i = 0; i < sizeof(interface->blend_states) / sizeof(*interface->blend_states); ++i)
		gfx_delete_blend_state(g_wow->device, &interface->blend_states[i]);
	for (enum interface_blend_state blend = 0; blend < INTERFACE_BLEND_LAST; ++blend)
		gfx_delete_pipeline_state(g_wow->device, &interface->pipeline_states[blend]);
	interface_clear(interface);
	for (size_t i = 0; i < sizeof(interface->root_frames) / sizeof(*interface->root_frames); ++i)
		jks_array_destroy(&interface->root_frames[i]);
	for (size_t i = 0; i < sizeof(interface->frames_events) / sizeof(*interface->frames_events); ++i)
		jks_array_destroy(&interface->frames_events[i]);
	jks_array_destroy(&interface->ext_addons);
	jks_array_destroy(&interface->xml_ui);
	jks_array_destroy(&interface->addons);
	jks_hmap_destroy(&interface->virtual_layout_frames);
	jks_hmap_destroy(&interface->render_fonts);
	jks_hmap_destroy(&interface->regions);
	jks_hmap_destroy(&interface->frames);
	jks_hmap_destroy(&interface->fonts);
	gfx_delete_texture(g_wow->device, &interface->white_pixel);
	lua_script_delete(interface->error_script);
	lua_close(interface->L);
	for (size_t i = 0; i < sizeof(interface->cursors) / sizeof(*interface->cursors); ++i)
		gfx_delete_cursor(g_wow->window, interface->cursors[i]);
	mem_free(MEM_UI, interface);
}

void interface_clear(struct interface *interface)
{
	for (size_t i = 0; i < sizeof(interface->root_frames) / sizeof(*interface->root_frames); ++i)
	{
		struct jks_array *root_frames = &interface->root_frames[i];
		for (size_t j = 0; j < root_frames->size; ++j)
			ui_object_delete((struct ui_object*)*JKS_ARRAY_GET(root_frames, j, struct ui_frame*));
		jks_array_resize(root_frames, 0);
	}
	for (size_t i = 0; i < sizeof(interface->frames_events) / sizeof(*interface->frames_events); ++i)
		jks_array_resize(&interface->frames_events[i], 0);
	jks_array_resize(&interface->xml_ui, 0);
	jks_hmap_destroy(&interface->virtual_layout_frames);
	jks_hmap_destroy(&interface->render_fonts);
	jks_hmap_destroy(&interface->regions);
	jks_hmap_destroy(&interface->frames);
	jks_hmap_destroy(&interface->fonts);
	jks_hmap_init(&interface->virtual_layout_frames, sizeof(struct xml_layout_frame*), layout_frames_hmap_destructor, jks_hmap_hash_str, jks_hmap_cmp_str, &jks_hmap_memory_fn_UI);
	jks_hmap_init(&interface->render_fonts, sizeof(struct interface_font_model), render_font_model_hmap_destructor, jks_hmap_hash_str, jks_hmap_cmp_str, &jks_hmap_memory_fn_UI);
	jks_hmap_init(&interface->regions, sizeof(struct ui_region*), regions_hmap_destructor, jks_hmap_hash_str, jks_hmap_cmp_str, &jks_hmap_memory_fn_UI);
	jks_hmap_init(&interface->frames, sizeof(struct ui_frame*), frames_hmap_destructor, jks_hmap_hash_str, jks_hmap_cmp_str, &jks_hmap_memory_fn_UI);
	jks_hmap_init(&interface->fonts, sizeof(struct ui_font*), fonts_hmap_destructor, jks_hmap_hash_str, jks_hmap_cmp_str, &jks_hmap_memory_fn_UI);
	interface->active_input = NULL;
}

void initialize(struct interface *interface)
{
	gfx_create_input_layout(g_wow->device, &interface->input_layout, g_binds, sizeof(g_binds) / sizeof(*g_binds), &g_wow->shaders->ui);
	gfx_create_blend_state(g_wow->device, &interface->blend_states[INTERFACE_BLEND_OPAQUE], false, GFX_BLEND_ONE, GFX_BLEND_ZERO, GFX_BLEND_ONE, GFX_BLEND_ZERO, GFX_EQUATION_ADD, GFX_EQUATION_ADD, GFX_COLOR_MASK_ALL);
	gfx_create_blend_state(g_wow->device, &interface->blend_states[INTERFACE_BLEND_ALPHA], true, GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE_MINUS_SRC_ALPHA, GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE_MINUS_SRC_ALPHA, GFX_EQUATION_ADD, GFX_EQUATION_ADD, GFX_COLOR_MASK_ALL);
	gfx_create_blend_state(g_wow->device, &interface->blend_states[INTERFACE_BLEND_ADD], true, GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE, GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE, GFX_EQUATION_ADD, GFX_EQUATION_ADD, GFX_COLOR_MASK_ALL);
	gfx_create_blend_state(g_wow->device, &interface->blend_states[INTERFACE_BLEND_MOD], true, GFX_BLEND_SRC_COLOR, GFX_BLEND_ONE, GFX_BLEND_SRC_COLOR, GFX_BLEND_ONE, GFX_EQUATION_ADD, GFX_EQUATION_ADD, GFX_COLOR_MASK_ALL);
	gfx_create_depth_stencil_state(g_wow->device, &interface->depth_stencil_state, false, false, GFX_CMP_ALWAYS, false, 0, GFX_CMP_NEVER, 0, 0, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP);
	gfx_create_rasterizer_state(g_wow->device, &interface->rasterizer_state, GFX_FILL_SOLID, GFX_CULL_NONE, GFX_FRONT_CCW, true);
	for (enum interface_blend_state blend = 0; blend < INTERFACE_BLEND_LAST; ++blend)
	{
		gfx_create_pipeline_state(g_wow->device,
			&interface->pipeline_states[blend],
			&g_wow->shaders->ui,
			&interface->rasterizer_state,
			&interface->depth_stencil_state,
			&interface->blend_states[blend],
			&interface->input_layout,
			GFX_PRIMITIVE_TRIANGLES);
	}
	interface->initialized = true;
}

static void set_frame_screen(struct interface *interface)
{
	int64_t started = nanotime();
	load_src_addon(interface, "FrameXML");
	load_addons(interface);
	int64_t ended = nanotime();
	interface_execute_event(g_wow->interface, EVENT_PLAYER_ENTERING_WORLD, 0);
	interface_execute_event(g_wow->interface, EVENT_VARIABLES_LOADED, 0);
	interface_execute_event(g_wow->interface, EVENT_UPDATE_CHAT_WINDOWS, 0);
	snd_set_glue_music(g_wow->snd, NULL);
	LOG_INFO("loaded FrameXML in %" PRId64 " ms", (ended - started) / 1000000);
}

static void set_glue_screen(struct interface *interface)
{
	int64_t started = nanotime();
	load_src_addon(interface, "GlueXML");
	add_addons(interface);
	int64_t ended = nanotime();
	ui_object_set_hidden((struct ui_object*)interface_get_frame(interface, "AccountLogin"), false);
	/* interface_execute_event(interface, EVENT_SET_GLUE_SCREEN, 0); */
	LOG_INFO("loaded GlueXML in %" PRId64 " ms", (ended - started) / 1000000);
}

void interface_render(struct interface *interface)
{
#ifndef NDEBUG
	static uint64_t last_stacksize = 0;
	if (g_wow->frametime - last_stacksize > 1000000000)
	{
		last_stacksize = g_wow->frametime;
		if (lua_gettop(interface->L))
			LOG_DEBUG("stack: %d", lua_gettop(interface->L));
	}
#endif
	if (!interface->initialized)
		initialize(interface);
	if (interface->switch_framescreen)
	{
		interface_clear(interface);
		set_frame_screen(interface);
		interface->switch_framescreen = false;
		interface->is_gluescreen = false;
		gfx_resize_event_t event = {.width = g_wow->window->width, g_wow->window->height};
		interface_on_window_resized(interface, &event);
	}
	if (interface->switch_gluescreen)
	{
		interface_clear(interface);
		set_glue_screen(interface);
		interface->switch_gluescreen = false;
		interface->is_gluescreen = true;
		gfx_resize_event_t event = {.width = g_wow->window->width, g_wow->window->height};
		interface_on_window_resized(interface, &event);
	}
	gfx_bind_render_target(g_wow->device, NULL);
	interface->render_ctx = false;
	MAT4_ORTHO(float, interface->mat, 0.f, interface->width, interface->height, 0.f, -2.f, 2.f);
	for (size_t i = 0; i < sizeof(interface->root_frames) / sizeof(*interface->root_frames); ++i)
	{
		struct jks_array *strata = &interface->root_frames[i];
		for (size_t j = 0; j < strata->size; ++j)
		{
			struct ui_frame *frame = *JKS_ARRAY_GET(strata, j, struct ui_frame*);
			if (frame->region.hidden)
				continue;
			ui_object_render((struct ui_object*)frame);
		}
	}
}

void interface_enable_scissor(int32_t x, int32_t y, int32_t width, int32_t height)
{
	gfx_set_scissor(g_wow->device, x, g_wow->render_height - height - y, width, height);
}

void interface_disable_scissor(void)
{
	gfx_set_scissor(g_wow->device, 0, 0, g_wow->render_width, g_wow->render_height);
}

void interface_on_window_resized(struct interface *interface, gfx_resize_event_t *event)
{
	if (!interface->is_gluescreen)
	{
		interface->width = event->width;
		interface->height = event->height;
	}
	else
	{
		interface->width = GLUE_WIDTH;
		interface->height = GLUE_HEIGHT;
	}
	for (size_t i = 0; i < sizeof(interface->root_frames) / sizeof(*interface->root_frames); ++i)
	{
		struct jks_array *strata = &interface->root_frames[i];
		for (size_t j = 0; j < strata->size; ++j)
		{
			struct ui_frame *frame = *JKS_ARRAY_GET(strata, j, struct ui_frame*);
			ui_object_set_dirty_coords((struct ui_object*)frame);
		}
	}
	interface_execute_event(interface, EVENT_DISPLAY_SIZE_CHANGED, 0);
}

void interface_on_mouse_move(struct interface *interface, gfx_pointer_event_t *event)
{
	event->x *= interface->width / (float)g_wow->window->width;
	event->y *= interface->height / (float)g_wow->window->height;
	interface->mouse_x = event->x;
	interface->mouse_y = event->y;
	for (size_t i = sizeof(interface->root_frames) / sizeof(*interface->root_frames); i > 0; --i)
	{
		struct jks_array *strata = &interface->root_frames[i - 1];
		for (size_t j = strata->size; j > 0; --j)
		{
			struct ui_frame *frame = *JKS_ARRAY_GET(strata, j - 1, struct ui_frame*);
			if (frame->region.hidden)
				continue;
			ui_object_on_mouse_move((struct ui_object*)frame, event);
		}
	}
}

bool interface_on_mouse_down(struct interface *interface, gfx_mouse_event_t *event)
{
	event->x *= interface->width / (float)g_wow->window->width;
	event->y *= interface->height / (float)g_wow->window->height;
	for (size_t i = sizeof(interface->root_frames) / sizeof(*interface->root_frames); i > 0; --i)
	{
		struct jks_array *strata = &interface->root_frames[i - 1];
		for (size_t j = strata->size; j > 0; --j)
		{
			struct ui_frame *frame = *JKS_ARRAY_GET(strata, j - 1, struct ui_frame*);
			if (frame->region.hidden)
				continue;
			ui_object_on_mouse_down((struct ui_object*)frame, event);
		}
	}
	return event->used;
}

bool interface_on_mouse_up(struct interface *interface, gfx_mouse_event_t *event)
{
	event->x *= interface->width / (float)g_wow->window->width;
	event->y *= interface->height / (float)g_wow->window->height;
	for (size_t i = sizeof(interface->root_frames) / sizeof(*interface->root_frames); i > 0; --i)
	{
		struct jks_array *strata = &interface->root_frames[i - 1];
		for (size_t j = strata->size; j > 0; --j)
		{
			struct ui_frame *frame = *JKS_ARRAY_GET(strata, j - 1, struct ui_frame*);
			if (frame->region.hidden)
				continue;
			ui_object_on_mouse_up((struct ui_object*)frame, event);
		}
	}
	return event->used;
}

bool interface_on_mouse_scroll(struct interface *interface, gfx_scroll_event_t *event)
{
	event->mouse_x *= interface->width / (float)g_wow->window->width;
	event->mouse_y *= interface->height / (float)g_wow->window->height;
	for (size_t i = sizeof(interface->root_frames) / sizeof(*interface->root_frames); i > 0; --i)
	{
		struct jks_array *strata = &interface->root_frames[i - 1];
		for (size_t j = strata->size; j > 0; --j)
		{
			struct ui_frame *frame = *JKS_ARRAY_GET(strata, j - 1, struct ui_frame*);
			if (frame->region.hidden)
				continue;
			ui_object_on_mouse_scroll((struct ui_object*)frame, event);
		}
	}
	return event->used;
}

bool interface_on_key_down(struct interface *interface, gfx_key_event_t *event)
{
	if (interface->active_input)
	{
		if (ui_object_on_key_down((struct ui_object*)interface->active_input, event))
			return true;
		return true;
	}
	for (size_t i = sizeof(interface->root_frames) / sizeof(*interface->root_frames); i > 0; --i)
	{
		struct jks_array *strata = &interface->root_frames[i - 1];
		for (size_t j = strata->size; j > 0; --j)
		{
			struct ui_frame *frame = *JKS_ARRAY_GET(strata, j - 1, struct ui_frame*);
			if (frame->region.hidden)
				continue;
			if (ui_object_on_key_down((struct ui_object*)frame, event))
				return true;
		}
	}
	return false;
}

bool interface_on_key_up(struct interface *interface, gfx_key_event_t *event)
{
	if (interface->active_input)
	{
		if (ui_object_on_key_up((struct ui_object*)interface->active_input, event))
			return true;
		return true;
	}
	for (size_t i = sizeof(interface->root_frames) / sizeof(*interface->root_frames); i > 0; --i)
	{
		struct jks_array *strata = &interface->root_frames[i - 1];
		for (size_t j = strata->size; j > 0; --j)
		{
			struct ui_frame *frame = *JKS_ARRAY_GET(strata, j - 1, struct ui_frame*);
			if (frame->region.hidden)
				continue;
			if (ui_object_on_key_up((struct ui_object*)frame, event))
				return true;
		}
	}
	return false;
}

bool interface_on_key_press(struct interface *interface, gfx_key_event_t *event)
{
	if (interface->active_input)
	{
		if (ui_edit_box_on_key_press(interface->active_input, event))
			return true;
		return true;
	}
	return false;
}

bool interface_on_char(struct interface *interface, gfx_char_event_t *event)
{
	if (interface->active_input)
	{
		if (ui_edit_box_on_char(interface->active_input, event))
			return true;
		return true;
	}
	return false;
}

void interface_set_cursor(struct interface *interface, enum cursor_type cursor)
{
	gfx_set_cursor(g_wow->window, interface->cursors[cursor]);
}

bool interface_load_xml(struct interface *interface, struct addon *addon, const char *filename, const char *data, size_t len)
{
	xmlDoc *doc = xmlReadMemory(data, len, NULL, "utf8", 0);
	if (!doc)
	{
		LOG_ERROR("xmlReadMemory failed");
		return false;
	}
	xmlNode *root = xmlDocGetRootElement(doc);
	if (!root)
	{
		xmlFreeDoc(doc);
		LOG_ERROR("no root node");
		return false;
	}
	if (root->type != XML_ELEMENT_NODE)
	{
		xmlFreeDoc(doc);
		LOG_ERROR("root node is not XML_ELEMENT_NODE");
		return false;
	}
	if (strcmp((const char*)root->name, "Ui"))
	{
		xmlFreeDoc(doc);
		LOG_ERROR("root node not <Ui>");
		return false;
	}
	struct xml_ui *ui = xml_ui_new(addon, filename);
	struct xml_node node;
	xml_node_parse(&node, root);
	xml_element_parse((struct xml_element*)ui, &node);
	xmlFreeDoc(doc);
	xmlCleanupParser();
	if (!jks_array_push_back(&interface->xml_ui, &ui))
	{
		xml_element_delete((struct xml_element*)ui);
		LOG_ERROR("failed to push back xml file to ui");
		return false;
	}
	return true;
}

bool interface_load_lua(struct interface *interface, const char *data, size_t len, const char *source)
{
	bool ret = false;
	struct lua_script *script = NULL;
	char *tmp;
	char *text = mem_malloc(MEM_UI, len + 1);
	if (!text)
	{
		LOG_ERROR("failed to malloc lua string");
		goto err;
	}
	memcpy(text, data, len);
	text[len] = '\0';
	tmp = text;
	while ((tmp = strstr(tmp, "\\«")))
	{
		tmp[1] = '"';
		tmp[2] = ' ';
	}
	tmp = text;
	while ((tmp = strstr(tmp, "»\\")))
	{
		tmp[0] = ' ';
		tmp[1] = '\\';
		tmp[2] = '"';
	}
	script = lua_script_new(interface->L, text, source);
	if (!script)
	{
		LOG_ERROR("failed to create lua script");
		goto err;
	}
	if (!lua_script_execute(script))
	{
		LOG_ERROR("failed to execute lua script");
		goto err;
	}
	ret = true;

err:
	lua_script_delete(script);
	mem_free(MEM_UI, text);
	return ret;
}

void interface_set_active_input(struct interface *interface, struct ui_edit_box *edit_box)
{
	if (interface->active_input)
		char_input_set_enabled(&interface->active_input->char_input, false);
	interface->active_input = edit_box;
	if (interface->active_input)
	{
		interface->active_input->last_action = g_wow->frametime;
		char_input_set_enabled(&interface->active_input->char_input, true);
	}
}

struct interface_font *interface_ref_render_font(struct interface *interface, const char *name, uint32_t size)
{
	struct interface_font_model *font_model = jks_hmap_get(&interface->render_fonts, JKS_HMAP_KEY_STR((char*)name));
	if (!font_model)
	{
		struct font_model *model;
		if (!cache_ref_by_key_font(g_wow->cache, name, &model))
		{
			LOG_ERROR("unknown font: %s", name);
			mem_free(MEM_UI, font_model);
			return NULL;
		}
		char *key = mem_strdup(MEM_UI, name);
		if (!key)
		{
			LOG_ERROR("can't duplicate font model key");
			cache_unref_by_ref_font(g_wow->cache, model);
			return NULL;
		}
		struct interface_font_model new_model;
		new_model.name = key;
		jks_hmap_init(&new_model.fonts, sizeof(struct interface_font), render_font_hmap_destructor, jks_hmap_hash_u32, jks_hmap_cmp_u32, &jks_hmap_memory_fn_UI);
		new_model.model = model;
		font_model = jks_hmap_set(&interface->render_fonts, JKS_HMAP_KEY_STR(key), &new_model);
		if (!font_model)
		{
			LOG_ERROR("can't add font model to hmap");
			cache_unref_by_ref_font(g_wow->cache, model);
			free(font_model);
			free(key);
			return NULL;
		}
	}
	struct interface_font *font = jks_hmap_get(&font_model->fonts, JKS_HMAP_KEY_U32(size));
	if (font)
	{
		font->refcount++;
		return font;
	}
	struct interface_font new_font;
	new_font.refcount = 1;
	new_font.size = size;
	new_font.atlas = font_atlas_new();
	new_font.model = font_model;
	if (!new_font.atlas)
	{
		LOG_ERROR("font atlas creation failed");
		return NULL;
	}
	new_font.font = font_new(font_model->model, size, 0, new_font.atlas);
	if (!new_font.font)
	{
		LOG_ERROR("font creation failed");
		font_atlas_delete(new_font.atlas);
		return NULL;
	}
	new_font.outline_thick = font_new(font_model->model, size, 64, new_font.atlas);
	if (!new_font.outline_thick)
	{
		LOG_ERROR("thick outline font creation failed");
		font_atlas_delete(new_font.atlas);
		font_delete(new_font.font);
		return NULL;
	}
	new_font.outline_normal = font_new(font_model->model, size, 96, new_font.atlas);
	if (!new_font.outline_normal)
	{
		LOG_ERROR("normal outline font creation failed");
		font_atlas_delete(new_font.atlas);
		font_delete(new_font.font);
		font_delete(new_font.outline_thick);
		return NULL;
	}
	font = jks_hmap_set(&font_model->fonts, JKS_HMAP_KEY_U32(size), &new_font);
	if (!font)
	{
		LOG_ERROR("can't add font to hmap");
		font_atlas_delete(new_font.atlas);
		font_delete(new_font.font);
		font_delete(new_font.outline_thick);
		font_delete(new_font.outline_normal);
		return NULL;
	}
	return font;
}

void interface_unref_render_font(struct interface *interface, struct interface_font *font)
{
	if (--font->refcount)
		return;
	struct interface_font_model *font_model = font->model;
	if (!jks_hmap_erase(&font_model->fonts, JKS_HMAP_KEY_U32(font->size)))
		LOG_WARN("can't erase font from hmap");
	if (font_model->fonts.size)
		return;
	if (!jks_hmap_erase(&interface->render_fonts, JKS_HMAP_KEY_STR(font_model->name)))
		LOG_WARN("can't erase font model from hmap");
}

void interface_register_root_frame(struct interface *interface, struct ui_frame *frame)
{
	if (frame->strata >= sizeof(interface->root_frames) / sizeof(*interface->root_frames))
	{
		LOG_ERROR("invalid strata: %d", frame->strata);
		return;
	}
	if (!jks_array_push_back(&interface->root_frames[frame->strata], &frame))
		LOG_ERROR("failed to push root frame");
}

void interface_unregister_root_frame(struct interface *interface, struct ui_frame *frame)
{
	if (frame->strata >= sizeof(interface->root_frames) / sizeof(*interface->root_frames))
	{
		LOG_ERROR("invalid strata: %d", frame->strata);
		return;
	}
	struct jks_array *frames = &interface->root_frames[frame->strata];
	for (size_t i = 0; i < frames->size; ++i)
	{
		struct ui_frame *iter = *JKS_ARRAY_GET(frames, i, struct ui_frame*);
		if (iter == frame)
		{
			if (!jks_array_erase(frames, i))
				LOG_ERROR("failed to unregister root frame");
			break;
		}
	}
}

void interface_register_frame_event(struct interface *interface, struct ui_frame *frame, enum event_type event)
{
	if (!jks_array_push_back(&interface->frames_events[event], &frame))
		LOG_ERROR("failed to register frame event");
}

void interface_unregister_frame_event(struct interface *interface, struct ui_frame *frame, enum event_type event)
{
	struct jks_array *frames = &interface->frames_events[event];
	for (size_t i = 0; i < frames->size; ++i)
	{
		struct ui_frame *iter = *JKS_ARRAY_GET(frames, i, struct ui_frame*);
		if (iter == frame)
		{
			if (!jks_array_erase(frames, i))
				LOG_ERROR("failed to unregister frame event");
			return;
		}
	}
}

void interface_execute_event(struct interface *interface, enum event_type event, int params)
{
	int stack_size = lua_gettop(interface->L);
	struct jks_array *frames = &interface->frames_events[event];
	for (size_t i = 0; i < frames->size; ++i)
	{
		struct ui_frame *frame = *JKS_ARRAY_GET(frames, i, struct ui_frame*);
		lua_pushstring(interface->L, event_type_to_string(event));
		lua_setglobal(interface->L, "event");
		for (int param = 0; param < params; ++param)
		{
			lua_pushvalue(interface->L, -2 * params);
			lua_pushvalue(interface->L, -2 * params);
		}
		ui_frame_execute_script(frame, "OnEvent", params);
	}
	lua_settop(interface->L, stack_size - params * 2);
}

void interface_register_virtual_layout_frame(struct interface *interface, const char *name, struct xml_layout_frame *layout_frame)
{
	char *key = mem_strdup(MEM_UI, name);
	if (!key)
	{
		LOG_ERROR("failed to duplicate frame name");
		return;
	}
	if (!jks_hmap_set(&interface->virtual_layout_frames, JKS_HMAP_KEY_PTR(key), &layout_frame))
	{
		free(key);
		LOG_ERROR("failed to push virtual layout frame");
	}
}

struct xml_layout_frame *interface_get_virtual_layout_frame(struct interface *interface, const char *name)
{
	struct xml_layout_frame **layout_frame = jks_hmap_get(&interface->virtual_layout_frames, JKS_HMAP_KEY_PTR((void*)name));
	if (!layout_frame)
		return NULL;
	return *layout_frame;
}

void interface_unregister_virtual_layout_frame(struct interface *interface, const char *name)
{
	if (!jks_hmap_erase(&interface->virtual_layout_frames, JKS_HMAP_KEY_PTR((void*)name)))
		LOG_ERROR("failed to unregister virtual layout frame");
}

void interface_register_region(struct interface *interface, const char *name, struct ui_region *region)
{
	char *key = mem_strdup(MEM_UI, name);
	if (!key)
	{
		LOG_ERROR("failed to duplicate region name");
		return;
	}
	if (!jks_hmap_set(&interface->regions, JKS_HMAP_KEY_PTR(key), &region))
	{
		free(key);
		LOG_ERROR("failed to register region");
	}
}

struct ui_region *interface_get_region(struct interface *interface, const char *name)
{
	struct ui_region **region = jks_hmap_get(&interface->regions, JKS_HMAP_KEY_PTR((void*)name));
	if (!region)
		return NULL;
	return *region;
}

void interface_unregister_region(struct interface *interface, const char *name)
{
	if (!jks_hmap_erase(&interface->regions, JKS_HMAP_KEY_PTR((void*)name)))
		LOG_ERROR("failed to unregister region");
}

void interface_register_frame(struct interface *interface, const char *name, struct ui_frame *frame)
{
	char *key = mem_strdup(MEM_UI, name);
	if (!key)
	{
		LOG_ERROR("failed to duplicate frame name");
		return;
	}
	if (!jks_hmap_set(&interface->frames, JKS_HMAP_KEY_PTR(key), &frame))
	{
		free(key);
		LOG_ERROR("failed to register frame");
	}
}

struct ui_frame *interface_get_frame(struct interface *interface, const char *name)
{
	struct ui_frame **frame = jks_hmap_get(&interface->frames, JKS_HMAP_KEY_PTR((void*)name));
	if (!frame)
		return NULL;
	return *frame;
}

void interface_unregister_frame(struct interface *interface, const char *name)
{
	if (!jks_hmap_erase(&interface->frames, JKS_HMAP_KEY_PTR((void*)name)))
		LOG_ERROR("failed to unregister frame");
}

void interface_register_font(struct interface *interface, const char *name, struct ui_font *font)
{
	char *key = mem_strdup(MEM_UI, name);
	if (!key)
	{
		LOG_ERROR("failed to duplicate font name");
		return;
	}
	if (!jks_hmap_set(&interface->fonts, JKS_HMAP_KEY_PTR(key), &font))
	{
		free(key);
		LOG_ERROR("failed to register font");
	}
}

struct ui_font *interface_get_font(struct interface *interface, const char *name)
{
	struct ui_font **font = jks_hmap_get(&interface->fonts, JKS_HMAP_KEY_PTR((void*)name));
	if (!font)
		return NULL;
	return *font;
}

void interface_unregister_font(struct interface *interface, const char *name)
{
	if (!jks_hmap_erase(&interface->fonts, JKS_HMAP_KEY_PTR((void*)name)))
		LOG_ERROR("failed to unregister font");
}

void interface_set_error_script(struct interface *interface, struct lua_script *script)
{
	lua_script_delete(interface->error_script);
	interface->error_script = script;
}

void interface_set_render_ctx(struct interface *interface, bool enable)
{
	if (interface->render_ctx == enable)
		return;
	interface->render_ctx = enable;
	if (enable)
		gfx_set_viewport(g_wow->device, 0, 0, g_wow->render_width, g_wow->render_height);
}

void load_ext_addons(struct interface *interface)
{
	for (size_t i = 0; i < interface->addons.size; ++i)
		addon_load(*JKS_ARRAY_GET(&interface->addons, i, struct addon*));
}

static bool load_src_addon(struct interface *interface, const char *name)
{
	struct addon *addon = src_addon_new(interface, name);
	if (!addon)
	{
		LOG_ERROR("failed to create src addon %s", name);
		return false;
	}
	if (!addon_load(addon))
	{
		LOG_ERROR("failed to load src addon");
		return false;
	}
	addon_delete(addon);
	return true;
}

static bool test_pub_file(const char *fn)
{
	static const uint8_t pub_ref[] =
	{
		0x01, 0xc3, 0x5b, 0x50, 0x84, 0xb9, 0x3e, 0x32,
		0x42, 0x8c, 0xd0, 0xc7, 0x48, 0xfa, 0x0e, 0x5d,
		0x54, 0x5a, 0xa3, 0x0e, 0x14, 0xba, 0x9e, 0x0d,
		0xb9, 0x5d, 0x8b, 0xee, 0xb6, 0x84, 0x93, 0x45,
		0x75, 0xff, 0x31, 0xfe, 0x2f, 0x64, 0x3f, 0x3d,
		0x6d, 0x07, 0xd9, 0x44, 0x9b, 0x40, 0x85, 0x59,
		0x34, 0x4e, 0x10, 0xe1, 0xe7, 0x43, 0x69, 0xef,
		0x7c, 0x16, 0xfc, 0xb4, 0xed, 0x1b, 0x95, 0x28,
		0xa8, 0x23, 0x76, 0x51, 0x31, 0x57, 0x30, 0x2b,
		0x79, 0x08, 0x50, 0x10, 0x1c, 0x4a, 0x1a, 0x2c,
		0xc8, 0x8b, 0x8f, 0x05, 0x2d, 0x22, 0x3d, 0xdb,
		0x5a, 0x24, 0x7a, 0x0f, 0x13, 0x50, 0x37, 0x8f,
		0x5a, 0xcc, 0x9e, 0x04, 0x44, 0x0e, 0x87, 0x01,
		0xd4, 0xa3, 0x15, 0x94, 0x16, 0x34, 0xc6, 0xc2,
		0xc3, 0xfb, 0x49, 0xfe, 0xe1, 0xf9, 0xda, 0x8c,
		0x50, 0x3c, 0xbe, 0x2c, 0xbb, 0x57, 0xed, 0x46,
		0xb9, 0xad, 0x8b, 0xc6, 0xdf, 0x0e, 0xd6, 0x0f,
		0xbe, 0x80, 0xb3, 0x8b, 0x1e, 0x77, 0xcf, 0xad,
		0x22, 0xcf, 0xb7, 0x4b, 0xcf, 0xfb, 0xf0, 0x6b,
		0x11, 0x45, 0x2d, 0x7a, 0x81, 0x18, 0xf2, 0x92,
		0x7e, 0x98, 0x56, 0x5d, 0x5e, 0x69, 0x72, 0x0a,
		0x0d, 0x03, 0x0a, 0x85, 0xa2, 0x85, 0x9c, 0xcb,
		0xfb, 0x56, 0x6e, 0x8f, 0x44, 0xbb, 0x8f, 0x02,
		0x22, 0x68, 0x63, 0x97, 0xbc, 0x85, 0xba, 0xa8,
		0xf7, 0xb5, 0x40, 0x68, 0x3c, 0x77, 0x86, 0x6f,
		0x4b, 0xd7, 0x88, 0xca, 0x8a, 0xd7, 0xce, 0x36,
		0xf0, 0x45, 0x6e, 0xd5, 0x64, 0x79, 0x0f, 0x17,
		0xfc, 0x64, 0xdd, 0x10, 0x6f, 0xf3, 0xf5, 0xe0,
		0xa6, 0xc3, 0xfb, 0x1b, 0x8c, 0x29, 0xef, 0x8e,
		0xe5, 0x34, 0xcb, 0xd1, 0x2a, 0xce, 0x79, 0xc3,
		0x9a, 0x0d, 0x36, 0xea, 0x01, 0xe0, 0xaa, 0x91,
		0x20, 0x54, 0xf0, 0x72, 0xd8, 0x1e, 0xc7, 0x89,
		0xd2
	};
	char content[258];
	FILE *fp = fopen(fn, "rb");
	if (fp == NULL)
		return false;
	if (fread(content, 1, 258, fp) != 257)
	{
		fclose(fp);
		return false;
	}
	fclose(fp);
	if (memcmp(pub_ref, content, 257))
		return false;
	return true;
}

static int addon_cmp(const void *a, const void *b)
{
	const struct addon *a1 = *(const struct addon**)a;
	const struct addon *a2 = *(const struct addon**)b;
	return strcmp(a1->name, a2->name);
}

static bool add_addons(struct interface *interface)
{
	jks_array_resize(&interface->addons, 0);
	char path[256];
	snprintf(path, sizeof(path), "%s/Interface/AddOns", g_wow->game_path);
	DIR *dir = opendir(path);
	if (!dir)
	{
		LOG_WARN("can't open directory: %s", path);
		return true;
	}
	struct dirent *dirent;
	while ((dirent = readdir(dir)))
	{
		if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, ".."))
			continue;
		char pub_fn[256 * 4];
		snprintf(pub_fn, sizeof(pub_fn), "%s/%s/%s.pub", path, dirent->d_name, dirent->d_name);
		bool is_internal = test_pub_file(pub_fn);
		struct addon *addon;
		if (is_internal)
			addon = int_addon_new(interface, dirent->d_name);
		else
			addon = NULL; /* ext_addon_new(interface, dirent->d_name); */
		if (!addon)
		{
			LOG_ERROR("failed to load %s addon %s", is_internal ? "internal" : "external", dirent->d_name);
			continue;
		}
		if (!jks_array_push_back(&interface->addons, &addon))
		{
			LOG_ERROR("failed to add addon to list");
			addon_delete(addon);
			continue;
		}
		if (!is_internal && !jks_array_push_back(&interface->ext_addons, &addon))
		{
			LOG_ERROR("failed to add addon to ext list");
			jks_array_resize(&interface->addons, interface->addons.size - 1);
			addon_delete(addon);
			continue;
		}
		LOG_INFO("added %s addon %s", is_internal ? "internal" : "external", dirent->d_name);
	}
	closedir(dir);
	qsort(interface->ext_addons.data, interface->ext_addons.size, sizeof(struct addon*), addon_cmp);
	return true;
}

static void load_addon(struct addon *addon)
{
	if (!addon->enabled)
		return;
	for (size_t i = 0; i < addon->dependencies.size; ++i)
	{
		const char *dependency = *JKS_ARRAY_GET(&addon->dependencies, i, const char*);
		struct addon *dep = interface_get_addon(addon->interface, dependency);
		if (!dep)
		{
			LOG_ERROR("dependency not found: %s", dependency);
			return;
		}
		load_addon(dep);
	}
	if (!addon_load(addon))
	{
		LOG_WARN("failed to load addon %s", addon->name);
		return;
	}
}

static bool load_addons(struct interface *interface)
{
	for (size_t i = 0; i < interface->addons.size; ++i)
	{
		struct addon *addon = *JKS_ARRAY_GET(&interface->addons, i, struct addon*);
		load_addon(addon);
	}
	return false;
}

static gfx_cursor_t *load_cursor(const char *file)
{
	struct wow_mpq_file *mpq_file = NULL;
	struct wow_blp_file *blp_file = NULL;
	gfx_cursor_t *ret = NULL;
	uint8_t *data = NULL;
	uint32_t width;
	uint32_t height;

	if (!file)
		return NULL;
	mpq_file = wow_mpq_get_file(g_wow->mpq_compound, file);
	if (!mpq_file)
	{
		LOG_ERROR("failed to open %s", file);
		goto end;
	}
	blp_file = wow_blp_file_new(mpq_file);
	if (!blp_file)
	{
		LOG_ERROR("failed to parse blp file %s", file);
		goto end;
	}

	if (!blp_decode_rgba(blp_file, 0, &width, &height, &data))
	{
		LOG_ERROR("failed to decode blp file %s", file);
		goto end;
	}

	ret = gfx_create_cursor(g_wow->window, data, width, height, 0, 0);
	if (!ret)
	{
		LOG_ERROR("failed to create gfx cursor");
		goto end;
	}
end:
	wow_blp_file_delete(blp_file);
	wow_mpq_file_delete(mpq_file);
	mem_free(MEM_UI, data);
	return ret;
}

struct addon *interface_get_addon(struct interface *interface, const char *name)
{
	for (size_t i = 0; i < interface->addons.size; ++i)
	{
		struct addon *addon = *JKS_ARRAY_GET(&interface->addons, i, struct addon*);
		if (!strcmp(addon->name, name))
			return addon;
	}
	return NULL;
}

const char *lua_gfx_mouse_to_string(enum gfx_mouse_button button)
{
	static const char *names[] =
	{
		[GFX_MOUSE_BUTTON_LEFT]   = "LeftButton",
		[GFX_MOUSE_BUTTON_RIGHT]  = "RightButton",
		[GFX_MOUSE_BUTTON_MIDDLE] = "MiddleButton",
		[GFX_MOUSE_BUTTON_4]      = "Button4",
		[GFX_MOUSE_BUTTON_5]      = "Button5",
		[GFX_MOUSE_BUTTON_6]      = "Button6",
		[GFX_MOUSE_BUTTON_7]      = "Button7",
	};
	if (button < sizeof(names) / sizeof(*names))
		return names[button];
	return NULL;
}

const char *lua_gfx_key_to_string(enum gfx_key_code key)
{
	static const char *names[] =
	{
		"Unknown",
		"A",
		"B",
		"C",
		"D",
		"E",
		"F",
		"G",
		"H",
		"I",
		"J",
		"K",
		"L",
		"M",
		"N",
		"O",
		"P",
		"Q",
		"R",
		"S",
		"T",
		"U",
		"V",
		"W",
		"X",
		"Y",
		"Z",

		"0",
		"1",
		"2",
		"3",
		"4",
		"5",
		"6",
		"7",
		"8",
		"9",

		"NUMPAD0",
		"NUMPAD1",
		"NUMPAD2",
		"NUMPAD3",
		"NUMPAD4",
		"NUMPAD5",
		"NUMPAD6",
		"NUMPAD7",
		"NUMPAD8",
		"NUMPAD9",
		"NUMPADDIVIDE",
		"NUMPADMULTIPLY",
		"NUMPADMINUS",
		"NUMPADPLUS",
		"NUMPADEQUALS",
		"NUMPADDECIMAL",
		"NUMPADENTER", /* guessed */

		"F1",
		"F2",
		"F3",
		"F4",
		"F5",
		"F6",
		"F7",
		"F8",
		"F9",
		"F10",
		"F11",
		"F12",
		"F13",
		"F14",
		"F15",
		"F16",
		"F17",
		"F18",
		"F19",
		"F20",
		"F21",
		"F22",
		"F23",
		"F24",

		"LSHIFT",
		"RSHIFT",
		"LCTRL",
		"RCTRL",
		"LALT",
		"RALT",
		"LSUPER", /* guessed */
		"RSUPER", /* guessed */

		"LEFT",
		"RIGHT",
		"UP",
		"DOWN",

		"SPACE",
		"BACKSPACE",
		"ENTER",
		"TAB",

		"ESCAPE",
		"PAUSE",
		"DELETE",
		"INSERT",
		"HOME",
		"PAGEUP",
		"PAGEDOWN",
		"END",

		"COMMA",
		"PERIOD",
		"SLASH",
		"APOSTROPHE",
		"SEMICOLON",
		"TILDE", /* guessed */
		"LEFTBRACKET",
		"RIGHTBRACKET",
		"BACKSLASH",
		"EQUAL", /* guessed */
		"SUBTRACT", /* guessed */

		"SCROLLLOCK",
		"NUMLOCK",
		"CAPSLOCK",

		"PRINTSCREEN",
	};
	if (key < sizeof(names) / sizeof(*names))
		return names[key];
	return NULL;
}
