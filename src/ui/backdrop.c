#include "backdrop.h"

#include "itf/interface.h"

#include "xml/backdrop.h"

#include "ui/region.h"

#include "shaders.h"
#include "memory.h"
#include "cache.h"
#include "log.h"
#include "blp.h"
#include "wow.h"

#include <gfx/device.h>

#include <string.h>
#include <stdio.h>

#ifdef interface
# undef interface
#endif

struct ui_backdrop *ui_backdrop_new(struct interface *interface, struct ui_region *parent)
{
	struct ui_backdrop *backdrop = mem_malloc(MEM_UI, sizeof(*backdrop));
	if (!backdrop)
	{
		LOG_ERROR("failed to allocate backdrop");
		return NULL;
	}
	backdrop->interface = interface;
	backdrop->edge_file = NULL;
	backdrop->bg_file = NULL;
	backdrop->tile = false;
	backdrop->edge_texture = NULL;
	backdrop->bg_texture = NULL;
	backdrop->parent = parent;
	backdrop->edge_initialized = false;
	backdrop->bg_initialized = false;
	backdrop->dirty_size = false;
	ui_inset_init(&backdrop->background_insets, 0, 0, 0, 0);
	ui_value_init(&backdrop->edge_size, 0);
	ui_value_init(&backdrop->tile_size, 1);
	ui_color_init(&backdrop->border_color, 1, 1, 1, 1);
	ui_color_init(&backdrop->color, 1, 1, 1, 1);
	backdrop->edge_attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	backdrop->bg_attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		backdrop->edge_uniform_buffers[i] =  GFX_BUFFER_INIT();
		backdrop->bg_uniform_buffers[i] = GFX_BUFFER_INIT();
	}
	backdrop->edge_vertexes_buffer = GFX_BUFFER_INIT();
	backdrop->edge_indices_buffer = GFX_BUFFER_INIT();
	backdrop->bg_vertexes_buffer = GFX_BUFFER_INIT();
	backdrop->bg_indices_buffer = GFX_BUFFER_INIT();
	return backdrop;
}

void ui_backdrop_delete(struct ui_backdrop *backdrop)
{
	if (!backdrop)
		return;
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		gfx_delete_buffer(g_wow->device, &backdrop->edge_uniform_buffers[i]);
		gfx_delete_buffer(g_wow->device, &backdrop->bg_uniform_buffers[i]);
	}
	gfx_delete_buffer(g_wow->device, &backdrop->edge_vertexes_buffer);
	gfx_delete_buffer(g_wow->device, &backdrop->edge_indices_buffer);
	gfx_delete_buffer(g_wow->device, &backdrop->bg_vertexes_buffer);
	gfx_delete_buffer(g_wow->device, &backdrop->bg_indices_buffer);
	gfx_delete_attributes_state(g_wow->device, &backdrop->edge_attributes_state);
	gfx_delete_attributes_state(g_wow->device, &backdrop->bg_attributes_state);
	if (backdrop->edge_texture)
		cache_unref_by_ref_blp(g_wow->cache, backdrop->edge_texture);
	if (backdrop->bg_texture)
		cache_unref_by_ref_blp(g_wow->cache, backdrop->bg_texture);
	mem_free(MEM_UI, backdrop->edge_file);
	mem_free(MEM_UI, backdrop->bg_file);
	mem_free(MEM_UI, backdrop);
}

void ui_backdrop_load_xml(struct ui_backdrop *backdrop, const struct xml_backdrop *xml)
{
	if (OPTIONAL_ISSET(xml->background_insets))
		ui_inset_init_xml(&backdrop->background_insets, &OPTIONAL_GET(xml->background_insets));
	if (OPTIONAL_ISSET(xml->edge_size))
		ui_value_init_xml(&backdrop->edge_size, &OPTIONAL_GET(xml->edge_size));
	if (OPTIONAL_ISSET(xml->tile_size))
		ui_value_init_xml(&backdrop->tile_size, &OPTIONAL_GET(xml->tile_size));
	if (xml->edge_file)
	{
		char filename[512];
		snprintf(filename, sizeof(filename), "%s", xml->edge_file);
		normalize_blp_filename(filename, sizeof(filename));
		mem_free(MEM_UI, backdrop->edge_file);
		backdrop->edge_file = mem_strdup(MEM_UI, filename);
		if (!backdrop->edge_file)
			LOG_ERROR("failed to duplicate edge file name");
	}
	if (xml->bg_file)
	{
		char filename[512];
		snprintf(filename, sizeof(filename), "%s", xml->bg_file);
		normalize_blp_filename(filename, sizeof(filename));
		mem_free(MEM_UI, backdrop->bg_file);
		backdrop->bg_file = mem_strdup(MEM_UI, filename);
		if (!backdrop->bg_file)
			LOG_ERROR("failed to duplicate bg file name");
	}
	if (OPTIONAL_ISSET(xml->color))
		ui_color_init_xml(&backdrop->color, &OPTIONAL_GET(xml->color));
	if (OPTIONAL_ISSET(xml->border_color))
		ui_color_init_xml(&backdrop->border_color, &OPTIONAL_GET(xml->border_color));
	if (OPTIONAL_ISSET(xml->tile))
		backdrop->tile = OPTIONAL_GET(xml->tile);
	if (backdrop->edge_file)
	{
		if (cache_ref_by_key_blp(g_wow->cache, backdrop->edge_file, &backdrop->edge_texture))
			blp_texture_ask_load(backdrop->edge_texture);
		else
			backdrop->edge_texture = NULL;
	}
	if (backdrop->bg_file)
	{
		if (cache_ref_by_key_blp(g_wow->cache, backdrop->bg_file, &backdrop->bg_texture))
			blp_texture_ask_load(backdrop->bg_texture);
		else
			backdrop->bg_texture = NULL;
	}
}

static void update_bg_vertexes_buffer(struct ui_backdrop *backdrop)
{
	float left = backdrop->background_insets.abs.left;
	float right = ui_region_get_width(backdrop->parent) - backdrop->background_insets.abs.right;
	float top = backdrop->background_insets.abs.top;
	float bottom = ui_region_get_height(backdrop->parent) - backdrop->background_insets.abs.bottom;
	float uv_left;
	float uv_right;
	float uv_top;
	float uv_bottom;
	if (backdrop->tile)
	{
		uv_left = 0;
		uv_right = (float)(right - left) / backdrop->tile_size.abs;
		uv_top = 0;
		uv_bottom = (float)(bottom - top) / backdrop->tile_size.abs;
	}
	else
	{
		uv_left = 0;
		uv_right = 1;
		uv_top = 0;
		uv_bottom = 1;
	}
	struct shader_ui_input vertexes[4] =
	{
		{{left , top}   , {1, 1, 1, 1}, {uv_left , uv_top}},
		{{right, top}   , {1, 1, 1, 1}, {uv_right, uv_top}},
		{{right, bottom}, {1, 1, 1, 1}, {uv_right, uv_bottom}},
		{{left , bottom}, {1, 1, 1, 1}, {uv_left , uv_bottom}},
	};
	gfx_set_buffer_data(&backdrop->bg_vertexes_buffer, vertexes, sizeof(vertexes), 0);
}

static void update_edge_vertexes_buffer(struct ui_backdrop *backdrop)
{
	float edge_abs = backdrop->edge_size.abs;
	float left = 0;
	float right = ui_region_get_width(backdrop->parent);
	float top = 0;
	float bottom = ui_region_get_height(backdrop->parent);
	float border_tex_height = ((bottom - top) - backdrop->edge_size.abs * 2) / (float)backdrop->edge_size.abs;
	float border_tex_width = ((right - left) - backdrop->edge_size.abs * 2) / (float)backdrop->edge_size.abs;
	struct shader_ui_input vertexes[32] =
	{
		/* top left */
		{{left           , top           }, {1, 1, 1, 1}, {.5  , 0}},
		{{left + edge_abs, top           }, {1, 1, 1, 1}, {.625, 0}},
		{{left + edge_abs, top + edge_abs}, {1, 1, 1, 1}, {.625, 1}},
		{{left           , top + edge_abs}, {1, 1, 1, 1}, {.5  , 1}},
		/* top right */
		{{right - edge_abs, top           }, {1, 1, 1, 1}, {.625, 0}},
		{{right           , top           }, {1, 1, 1, 1}, {.75 , 0}},
		{{right           , top + edge_abs}, {1, 1, 1, 1}, {.75 , 1}},
		{{right - edge_abs, top + edge_abs}, {1, 1, 1, 1}, {.625, 1}},
		/* bottom right */
		{{right - edge_abs, bottom - edge_abs}, {1, 1, 1, 1}, {.875, 0}},
		{{right           , bottom - edge_abs}, {1, 1, 1, 1}, {1   , 0}},
		{{right           , bottom           }, {1, 1, 1, 1}, {1   , 1}},
		{{right - edge_abs, bottom           }, {1, 1, 1, 1}, {.875, 1}},
		/* bottom left */
		{{left           , bottom - edge_abs}, {1, 1, 1, 1}, {.75 , 0}},
		{{left + edge_abs, bottom - edge_abs}, {1, 1, 1, 1}, {.875, 0}},
		{{left + edge_abs, bottom           }, {1, 1, 1, 1}, {.875, 1}},
		{{left           , bottom           }, {1, 1, 1, 1}, {.75 , 1}},
		/* top */
		{{left + edge_abs , top           }, {1, 1, 1, 1}, {.25 , border_tex_width}},
		{{right - edge_abs, top           }, {1, 1, 1, 1}, {.25 , 0               }},
		{{right - edge_abs, top + edge_abs}, {1, 1, 1, 1}, {.375, 0               }},
		{{left + edge_abs , top + edge_abs}, {1, 1, 1, 1}, {.375, border_tex_width}},
		/* right */
		{{right - edge_abs, top + edge_abs   }, {1, 1, 1, 1}, {.125, 0                }},
		{{right           , top + edge_abs   }, {1, 1, 1, 1}, {.25 , 0                }},
		{{right           , bottom - edge_abs}, {1, 1, 1, 1}, {.25 , border_tex_height}},
		{{right - edge_abs, bottom - edge_abs}, {1, 1, 1, 1}, {.125, border_tex_height}},
		/* bottom */
		{{left + edge_abs , bottom - edge_abs}, {1, 1, 1, 1}, {.375, 1}},
		{{right - edge_abs, bottom - edge_abs}, {1, 1, 1, 1}, {.375, border_tex_width}},
		{{right - edge_abs, bottom           }, {1, 1, 1, 1}, {.5  , border_tex_width}},
		{{left + edge_abs , bottom           }, {1, 1, 1, 1}, {.5  , 1}},
		/* left */
		{{left           , top + edge_abs   }, {1, 1, 1, 1}, {0   , 0                }},
		{{left + edge_abs, top + edge_abs   }, {1, 1, 1, 1}, {.125, 0                }},
		{{left + edge_abs, bottom - edge_abs}, {1, 1, 1, 1}, {.125, border_tex_height}},
		{{left           , bottom - edge_abs}, {1, 1, 1, 1}, {0   , border_tex_height}},
	};
	gfx_set_buffer_data(&backdrop->edge_vertexes_buffer, vertexes, sizeof(vertexes), 0);
}

void ui_backdrop_render(struct ui_backdrop *backdrop)
{
	interface_set_render_ctx(backdrop->interface, true);
	if (backdrop->bg_file)
	{
		if (!backdrop->bg_initialized)
		{
			uint16_t indices[6] = {0, 1, 2, 0, 2, 3};
			gfx_create_buffer(g_wow->device, &backdrop->bg_vertexes_buffer, GFX_BUFFER_VERTEXES, NULL, 4 * sizeof(struct shader_ui_input), GFX_BUFFER_STATIC);
			gfx_create_buffer(g_wow->device, &backdrop->bg_indices_buffer, GFX_BUFFER_INDICES, indices, sizeof(indices), GFX_BUFFER_IMMUTABLE);
			update_bg_vertexes_buffer(backdrop);
			gfx_attribute_bind_t binds[] =
			{
				{&backdrop->bg_vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, position)},
				{&backdrop->bg_vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, color)},
				{&backdrop->bg_vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, uv)},
			};
			gfx_create_attributes_state(g_wow->device, &backdrop->bg_attributes_state, binds, sizeof(binds) / sizeof(*binds), &backdrop->bg_indices_buffer, GFX_INDEX_UINT16);
			for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
				gfx_create_buffer(g_wow->device, &backdrop->bg_uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_ui_model_block), GFX_BUFFER_STREAM);
			backdrop->bg_initialized = true;
		}
		else if (backdrop->dirty_size)
		{
			update_bg_vertexes_buffer(backdrop);
		}
		struct shader_ui_model_block model_block;
		VEC4_CPY(model_block.color, backdrop->color);
		model_block.color.w *= ui_object_get_alpha((struct ui_object*)backdrop->parent);
		if (model_block.color.w != 0)
		{
			VEC4_SET(model_block.uv_transform, 1, 0, 1, 0);
			model_block.alpha_test = 0;
			model_block.use_mask = 0;
			struct vec3f tmp = {(float)ui_region_get_left(backdrop->parent), (float)ui_region_get_top(backdrop->parent), 0};
			MAT4_TRANSLATE(model_block.mvp, backdrop->interface->mat, tmp);
			gfx_set_buffer_data(&backdrop->bg_uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
			gfx_bind_attributes_state(g_wow->device, &backdrop->bg_attributes_state, &backdrop->interface->input_layout);
			blp_texture_bind(backdrop->bg_texture, 0);
			gfx_bind_pipeline_state(g_wow->device, &backdrop->interface->pipeline_states[INTERFACE_BLEND_ALPHA]);
			gfx_bind_constant(g_wow->device, 1, &backdrop->bg_uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
			gfx_draw_indexed(g_wow->device, 6, 0);
		}
	}
	if (backdrop->edge_file)
	{
		if (!backdrop->edge_initialized)
		{
			uint16_t indices[48] =
			{
				0 , 1 , 2 , 0 , 2 , 3,
				4 , 5 , 6 , 4 , 6 , 7,
				8 , 9 , 10, 8 , 10, 11,
				12, 13, 14, 12, 14, 15,
				16, 17, 18, 16, 18, 19,
				20, 21, 22, 20, 22, 23,
				24, 25, 26, 24, 26, 27,
				28, 29, 30, 28, 30, 31,
			};
			gfx_create_buffer(g_wow->device, &backdrop->edge_vertexes_buffer, GFX_BUFFER_VERTEXES, NULL, 32 * sizeof(struct shader_ui_input), GFX_BUFFER_STATIC);
			gfx_create_buffer(g_wow->device, &backdrop->edge_indices_buffer, GFX_BUFFER_INDICES, indices, sizeof(indices), GFX_BUFFER_IMMUTABLE);
			update_edge_vertexes_buffer(backdrop);
			gfx_attribute_bind_t binds[] =
			{
				{&backdrop->edge_vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, position)},
				{&backdrop->edge_vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, color)},
				{&backdrop->edge_vertexes_buffer, sizeof(struct shader_ui_input), offsetof(struct shader_ui_input, uv)},
			};
			gfx_create_attributes_state(g_wow->device, &backdrop->edge_attributes_state, binds, sizeof(binds) / sizeof(*binds), &backdrop->edge_indices_buffer, GFX_INDEX_UINT16);
			for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
				gfx_create_buffer(g_wow->device, &backdrop->edge_uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_ui_model_block), GFX_BUFFER_STREAM);
			backdrop->edge_initialized = true;
		}
		else if (backdrop->dirty_size)
		{
			update_edge_vertexes_buffer(backdrop);
		}
		struct shader_ui_model_block model_block;
		VEC4_CPY(model_block.color, backdrop->border_color);
		model_block.color.w *= ui_object_get_alpha((struct ui_object*)backdrop->parent);
		if (model_block.color.w != 0)
		{
			VEC4_SET(model_block.uv_transform, 1, 0, 1, 0);
			model_block.alpha_test = 0;
			model_block.use_mask = 0;
			struct vec3f tmp = {(float)ui_region_get_left(backdrop->parent), (float)ui_region_get_top(backdrop->parent), 0};
			MAT4_TRANSLATE(model_block.mvp, backdrop->interface->mat, tmp);
			gfx_set_buffer_data(&backdrop->edge_uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
			gfx_bind_attributes_state(g_wow->device, &backdrop->edge_attributes_state, &backdrop->interface->input_layout);
			blp_texture_bind(backdrop->edge_texture, 0);
			gfx_bind_pipeline_state(g_wow->device, &backdrop->interface->pipeline_states[INTERFACE_BLEND_ALPHA]);
			gfx_bind_constant(g_wow->device, 1, &backdrop->edge_uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
			gfx_draw_indexed(g_wow->device, 48, 0);
		}
	}
	backdrop->dirty_size = false;
}
