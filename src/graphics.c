#include "graphics.h"
#include "shaders.h"
#include "wow.h"

#include <libwow/m2.h>

#include <gfx/device.h>

#include <stddef.h>

#ifdef WITH_DEBUG_RENDERING
static const gfx_input_layout_bind_t g_m2_bones_points_binds[] =
{
	{GFX_ATTR_R32G32B32_FLOAT   , sizeof(struct shader_m2_bones_input), offsetof(struct shader_m2_bones_input, position)},
	{GFX_ATTR_R32G32B32A32_FLOAT, sizeof(struct shader_m2_bones_input), offsetof(struct shader_m2_bones_input, color)},
	{GFX_ATTR_R32_UINT          , sizeof(struct shader_m2_bones_input), offsetof(struct shader_m2_bones_input, bone)},
};

static const gfx_input_layout_bind_t g_m2_bones_lines_binds[] =
{
	{GFX_ATTR_R32G32B32_FLOAT   , sizeof(struct shader_m2_bones_input), offsetof(struct shader_m2_bones_input, position)},
	{GFX_ATTR_R32G32B32A32_FLOAT, sizeof(struct shader_m2_bones_input), offsetof(struct shader_m2_bones_input, color)},
	{GFX_ATTR_R32_UINT          , sizeof(struct shader_m2_bones_input), offsetof(struct shader_m2_bones_input, bone)},
};

static const gfx_input_layout_bind_t g_wmo_collisions_binds[] =
{
	{GFX_ATTR_R32G32B32_FLOAT, sizeof(struct vec3f), 0},
};

static const gfx_input_layout_bind_t g_m2_collisions_binds[] =
{
	{GFX_ATTR_R32G32B32_FLOAT, sizeof(struct shader_m2_collisions_input), offsetof(struct shader_m2_collisions_input, position)},
};

static const gfx_input_layout_bind_t g_wmo_portals_binds[] =
{
	{GFX_ATTR_R32G32B32_FLOAT   , sizeof(struct shader_wmo_portals_input), offsetof(struct shader_wmo_portals_input, position)},
	{GFX_ATTR_R32G32B32A32_FLOAT, sizeof(struct shader_wmo_portals_input), offsetof(struct shader_wmo_portals_input, color)},
};

static const gfx_input_layout_bind_t g_collisions_binds[] =
{
	{GFX_ATTR_R32G32B32_FLOAT, sizeof(struct vec3f), 0},
};

static const gfx_input_layout_bind_t g_wmo_lights_binds[] =
{
	{GFX_ATTR_R32G32B32_FLOAT   , sizeof(struct shader_basic_input), offsetof(struct shader_basic_input, position)},
	{GFX_ATTR_R32G32B32A32_FLOAT, sizeof(struct shader_basic_input), offsetof(struct shader_basic_input, color)},
};

static const gfx_input_layout_bind_t g_m2_lights_binds[] =
{
	{GFX_ATTR_R32G32B32_FLOAT   , sizeof(struct shader_m2_lights_input), offsetof(struct shader_m2_lights_input, position)},
	{GFX_ATTR_R32G32B32A32_FLOAT, sizeof(struct shader_m2_lights_input), offsetof(struct shader_m2_lights_input, color)},
	{GFX_ATTR_R32_UINT          , sizeof(struct shader_m2_lights_input), offsetof(struct shader_m2_lights_input, bone)},
};

static const gfx_input_layout_bind_t g_aabb_binds[] =
{
	{GFX_ATTR_R32G32B32_FLOAT, sizeof(struct vec3f), 0},
};
#endif

static const gfx_input_layout_bind_t g_wmo_colored_binds[] =
{
	{GFX_ATTR_R32G32B32_FLOAT, sizeof(struct shader_wmo_input), offsetof(struct shader_wmo_input, position)},
	{GFX_ATTR_R32G32B32_FLOAT, sizeof(struct shader_wmo_input), offsetof(struct shader_wmo_input, norm)},
	{GFX_ATTR_R32G32_FLOAT   , sizeof(struct shader_wmo_input), offsetof(struct shader_wmo_input, uv)},
	{GFX_ATTR_R8G8B8A8_UNORM , sizeof(uint8_t) * 4            , 0},
};

static const gfx_input_layout_bind_t g_mclq_water_binds[] =
{
	{GFX_ATTR_R32G32B32_FLOAT, sizeof(struct shader_mclq_input), offsetof(struct shader_mclq_input, position)},
	{GFX_ATTR_R8_UNORM       , sizeof(uint8_t)                 , 0},
	{GFX_ATTR_R32G32_FLOAT   , sizeof(struct shader_mclq_input), offsetof(struct shader_mclq_input, uv)},
};

static const gfx_input_layout_bind_t g_mclq_magma_binds[] =
{
	{GFX_ATTR_R32G32B32_FLOAT, sizeof(struct shader_mclq_input), offsetof(struct shader_mclq_input, position)},
	{GFX_ATTR_R32G32_FLOAT   , sizeof(struct shader_mclq_input), offsetof(struct shader_mclq_input, uv)},
};

static const gfx_input_layout_bind_t g_particles_binds[] =
{
	{GFX_ATTR_R32G32B32A32_FLOAT, sizeof(struct shader_particle_input), offsetof(struct shader_particle_input, position)},
	{GFX_ATTR_R32G32B32A32_FLOAT, sizeof(struct shader_particle_input), offsetof(struct shader_particle_input, color)},
	{GFX_ATTR_R32G32_FLOAT      , sizeof(struct shader_particle_input), offsetof(struct shader_particle_input, uv)},
};

static const gfx_input_layout_bind_t g_wmo_mliq_binds[] =
{
	{GFX_ATTR_R32G32B32_FLOAT   , sizeof(struct shader_mliq_input), offsetof(struct shader_mliq_input, position)},
	{GFX_ATTR_R32G32B32A32_FLOAT, sizeof(struct shader_mliq_input), offsetof(struct shader_mliq_input, uv)},
};

static const gfx_input_layout_bind_t g_ribbons_binds[] =
{
	{GFX_ATTR_R32G32B32A32_FLOAT, sizeof(struct shader_ribbon_input), offsetof(struct shader_ribbon_input, position)},
	{GFX_ATTR_R32G32B32A32_FLOAT, sizeof(struct shader_ribbon_input), offsetof(struct shader_ribbon_input, color)},
	{GFX_ATTR_R32G32_FLOAT      , sizeof(struct shader_ribbon_input), offsetof(struct shader_ribbon_input, uv)},
};

static const gfx_input_layout_bind_t g_skybox_binds[] =
{
	{GFX_ATTR_R32G32B32_FLOAT, sizeof(struct shader_skybox_input), offsetof(struct shader_skybox_input, position)},
	{GFX_ATTR_R32_FLOAT      , sizeof(struct shader_skybox_input), offsetof(struct shader_skybox_input, colors[0])},
	{GFX_ATTR_R32_FLOAT      , sizeof(struct shader_skybox_input), offsetof(struct shader_skybox_input, colors[1])},
	{GFX_ATTR_R32_FLOAT      , sizeof(struct shader_skybox_input), offsetof(struct shader_skybox_input, colors[2])},
	{GFX_ATTR_R32_FLOAT      , sizeof(struct shader_skybox_input), offsetof(struct shader_skybox_input, colors[3])},
	{GFX_ATTR_R32_FLOAT      , sizeof(struct shader_skybox_input), offsetof(struct shader_skybox_input, colors[4])},
};

static const gfx_input_layout_bind_t g_mcnk_binds[] =
{
	{GFX_ATTR_R8G8B8A8_SNORM, sizeof(struct shader_mcnk_input), offsetof(struct shader_mcnk_input, norm)},
	{GFX_ATTR_R32G32_FLOAT  , sizeof(struct vec2f) * 2        , sizeof(struct vec2f) * 0},
	{GFX_ATTR_R32G32_FLOAT  , sizeof(struct vec2f) * 2        , sizeof(struct vec2f) * 1},
	{GFX_ATTR_R32_FLOAT     , sizeof(struct shader_mcnk_input), offsetof(struct shader_mcnk_input, y)},
};

static const gfx_input_layout_bind_t g_text_binds[] =
{
	{GFX_ATTR_R32G32_FLOAT, sizeof(struct shader_text_input), offsetof(struct shader_text_input, position)},
	{GFX_ATTR_R32G32_FLOAT, sizeof(struct shader_text_input), offsetof(struct shader_text_input, uv)},
};

static const gfx_input_layout_bind_t g_wdl_binds[] =
{
	{GFX_ATTR_R32G32B32_FLOAT, sizeof(struct vec3f), 0},
};

static const gfx_input_layout_bind_t g_wmo_binds[] =
{
	{GFX_ATTR_R32G32B32_FLOAT, sizeof(struct shader_wmo_input), offsetof(struct shader_wmo_input, position)},
	{GFX_ATTR_R32G32B32_FLOAT, sizeof(struct shader_wmo_input), offsetof(struct shader_wmo_input, norm)},
	{GFX_ATTR_R32G32_FLOAT   , sizeof(struct shader_wmo_input), offsetof(struct shader_wmo_input, uv)},
	{GFX_ATTR_DISABLED       , 0                              , 0},
};

static const gfx_input_layout_bind_t g_m2_binds[] =
{
	{GFX_ATTR_R8G8B8A8_UNORM , sizeof(struct wow_m2_vertex), offsetof(struct wow_m2_vertex, bone_weights)},
	{GFX_ATTR_R32G32B32_FLOAT, sizeof(struct wow_m2_vertex), offsetof(struct wow_m2_vertex, pos)},
	{GFX_ATTR_R32G32B32_FLOAT, sizeof(struct wow_m2_vertex), offsetof(struct wow_m2_vertex, normal)},
	{GFX_ATTR_R8G8B8A8_UINT  , sizeof(struct wow_m2_vertex), offsetof(struct wow_m2_vertex, bone_indices)},
	{GFX_ATTR_R32G32_FLOAT   , sizeof(struct wow_m2_vertex), offsetof(struct wow_m2_vertex, tex_coords[0])},
	{GFX_ATTR_R32G32_FLOAT   , sizeof(struct wow_m2_vertex), offsetof(struct wow_m2_vertex, tex_coords[1])},
};

bool graphics_build_world_rasterizer_states(struct graphics *graphics)
{
	gfx_delete_rasterizer_state(g_wow->device, &graphics->world_rasterizer_states[WORLD_RASTERIZER_CULLED]);
	gfx_delete_rasterizer_state(g_wow->device, &graphics->world_rasterizer_states[WORLD_RASTERIZER_UNCULLED]);
	gfx_create_rasterizer_state(g_wow->device, &graphics->world_rasterizer_states[WORLD_RASTERIZER_CULLED],
	                            (g_wow->render_opt & RENDER_OPT_MESH) ? GFX_FILL_LINE : GFX_FILL_SOLID, GFX_CULL_BACK, GFX_FRONT_CCW, true);
	gfx_create_rasterizer_state(g_wow->device, &graphics->world_rasterizer_states[WORLD_RASTERIZER_UNCULLED],
	                            (g_wow->render_opt & RENDER_OPT_MESH) ? GFX_FILL_LINE : GFX_FILL_SOLID, GFX_CULL_NONE, GFX_FRONT_CCW, true);
	return true;
}

bool graphics_init(struct graphics *graphics)
{
	for (size_t i = 0; i < WORLD_BLEND_LAST; ++i)
		graphics->world_blend_states[i] = GFX_BLEND_STATE_INIT();
	gfx_create_blend_state(g_wow->device, &graphics->world_blend_states[WORLD_BLEND_OPAQUE],
	                       false, GFX_BLEND_ONE      , GFX_BLEND_ZERO               , GFX_BLEND_ONE      , GFX_BLEND_ZERO,
	                       GFX_EQUATION_ADD, GFX_EQUATION_ADD, GFX_COLOR_MASK_ALL);
	gfx_create_blend_state(g_wow->device, &graphics->world_blend_states[WORLD_BLEND_ALPHA],
	                       true , GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE_MINUS_SRC_ALPHA, GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE_MINUS_SRC_ALPHA,
	                       GFX_EQUATION_ADD, GFX_EQUATION_ADD, GFX_COLOR_MASK_ALL);
	gfx_create_blend_state(g_wow->device, &graphics->world_blend_states[WORLD_BLEND_NO_ALPHA_ADD],
	                       true , GFX_BLEND_ONE      , GFX_BLEND_ONE                , GFX_BLEND_ONE      , GFX_BLEND_ONE,
	                       GFX_EQUATION_ADD, GFX_EQUATION_ADD, GFX_COLOR_MASK_ALL);
	gfx_create_blend_state(g_wow->device, &graphics->world_blend_states[WORLD_BLEND_ADD],
	                       true , GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE                , GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE,
	                       GFX_EQUATION_ADD, GFX_EQUATION_ADD, GFX_COLOR_MASK_ALL);
	gfx_create_blend_state(g_wow->device, &graphics->world_blend_states[WORLD_BLEND_MOD],
	                       true , GFX_BLEND_DST_COLOR, GFX_BLEND_ZERO               , GFX_BLEND_DST_ALPHA, GFX_BLEND_ZERO,
	                       GFX_EQUATION_ADD, GFX_EQUATION_ADD, GFX_COLOR_MASK_ALL);
	gfx_create_blend_state(g_wow->device, &graphics->world_blend_states[WORLD_BLEND_MOD2X],
	                       true , GFX_BLEND_DST_COLOR, GFX_BLEND_SRC_COLOR          , GFX_BLEND_DST_ALPHA, GFX_BLEND_SRC_ALPHA,
	                       GFX_EQUATION_ADD, GFX_EQUATION_ADD, GFX_COLOR_MASK_ALL);
	for (size_t i = 0; i < WORLD_DEPTH_STENCIL_LAST; ++i)
		graphics->world_depth_stencil_states[i] = GFX_DEPTH_STENCIL_STATE_INIT();
	gfx_create_depth_stencil_state(g_wow->device, &graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_NO_NO], false, false, GFX_CMP_LEQUAL, false,  0, GFX_CMP_NEVER   , 0,  0, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP);
	gfx_create_depth_stencil_state(g_wow->device, &graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_NO_R] , false, false, GFX_CMP_LEQUAL, true ,  0, GFX_CMP_NOTEQUAL, 1, -1, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP);
	gfx_create_depth_stencil_state(g_wow->device, &graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_NO_W] , false, false, GFX_CMP_LEQUAL, true , -1, GFX_CMP_ALWAYS  , 1, -1, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_REPLACE);
	gfx_create_depth_stencil_state(g_wow->device, &graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_NO_RW], false, false, GFX_CMP_LEQUAL, true , -1, GFX_CMP_NOTEQUAL, 1, -1, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_REPLACE);
	gfx_create_depth_stencil_state(g_wow->device, &graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_R_NO] , false, true , GFX_CMP_LEQUAL, false,  0, GFX_CMP_NEVER   , 0,  0, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP);
	gfx_create_depth_stencil_state(g_wow->device, &graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_R_R]  , false, true , GFX_CMP_LEQUAL, true ,  0, GFX_CMP_NOTEQUAL, 1, -1, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP);
	gfx_create_depth_stencil_state(g_wow->device, &graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_R_W]  , false, true , GFX_CMP_LEQUAL, true , -1, GFX_CMP_ALWAYS  , 1, -1, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_REPLACE);
	gfx_create_depth_stencil_state(g_wow->device, &graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_R_RW] , false, true , GFX_CMP_LEQUAL, true , -1, GFX_CMP_NOTEQUAL, 1, -1, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_REPLACE);
	gfx_create_depth_stencil_state(g_wow->device, &graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_W_NO] , true , false, GFX_CMP_LEQUAL, false,  0, GFX_CMP_NEVER   , 0,  0, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP);
	gfx_create_depth_stencil_state(g_wow->device, &graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_W_R]  , true , false, GFX_CMP_LEQUAL, true ,  0, GFX_CMP_NOTEQUAL, 1, -1, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP);
	gfx_create_depth_stencil_state(g_wow->device, &graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_W_W]  , true , false, GFX_CMP_LEQUAL, true , -1, GFX_CMP_ALWAYS  , 1, -1, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_REPLACE);
	gfx_create_depth_stencil_state(g_wow->device, &graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_W_RW] , true , false, GFX_CMP_LEQUAL, true , -1, GFX_CMP_NOTEQUAL, 1, -1, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_REPLACE);
	gfx_create_depth_stencil_state(g_wow->device, &graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_RW_NO], true , true , GFX_CMP_LEQUAL, false,  0, GFX_CMP_NEVER   , 0,  0, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP);
	gfx_create_depth_stencil_state(g_wow->device, &graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_RW_R] , true , true , GFX_CMP_LEQUAL, true ,  0, GFX_CMP_NOTEQUAL, 1, -1, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP);
	gfx_create_depth_stencil_state(g_wow->device, &graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_RW_W] , true , true , GFX_CMP_LEQUAL, true , -1, GFX_CMP_ALWAYS  , 1, -1, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_REPLACE);
	gfx_create_depth_stencil_state(g_wow->device, &graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_RW_RW], true , true , GFX_CMP_LEQUAL, true , -1, GFX_CMP_NOTEQUAL, 1, -1, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_REPLACE);
	for (size_t i = 0; i < WORLD_RASTERIZER_LAST; ++i)
		graphics->world_rasterizer_states[i] = GFX_RASTERIZER_STATE_INIT();
	graphics_build_world_rasterizer_states(graphics);
#ifdef WITH_DEBUG_RENDERING
	graphics->collisions_triangles_rasterizer_state = GFX_RASTERIZER_STATE_INIT();
	graphics->collisions_lines_rasterizer_state = GFX_RASTERIZER_STATE_INIT();
	gfx_create_rasterizer_state(g_wow->device, &graphics->collisions_triangles_rasterizer_state,
	                            GFX_FILL_SOLID, GFX_CULL_NONE, GFX_FRONT_CCW, true);
	gfx_create_rasterizer_state(g_wow->device, &graphics->collisions_lines_rasterizer_state,
	                            GFX_FILL_LINE, GFX_CULL_NONE, GFX_FRONT_CCW, true);
	graphics->m2_bones_points_input_layout = GFX_INPUT_LAYOUT_INIT();
	graphics->m2_bones_lines_input_layout = GFX_INPUT_LAYOUT_INIT();
	graphics->wmo_collisions_input_layout = GFX_INPUT_LAYOUT_INIT();
	graphics->m2_collisions_input_layout = GFX_INPUT_LAYOUT_INIT();
	graphics->wmo_portals_input_layout = GFX_INPUT_LAYOUT_INIT();
	graphics->collisions_input_layout = GFX_INPUT_LAYOUT_INIT();
	graphics->wmo_lights_input_layout = GFX_INPUT_LAYOUT_INIT();
	graphics->m2_lights_input_layout = GFX_INPUT_LAYOUT_INIT();
	graphics->aabb_input_layout = GFX_INPUT_LAYOUT_INIT();
	gfx_create_input_layout(g_wow->device, &graphics->m2_bones_points_input_layout, g_m2_bones_points_binds, sizeof(g_m2_bones_points_binds) / sizeof(*g_m2_bones_points_binds), &g_wow->shaders->m2_bones);
	gfx_create_input_layout(g_wow->device, &graphics->m2_bones_lines_input_layout, g_m2_bones_lines_binds, sizeof(g_m2_bones_lines_binds) / sizeof(*g_m2_bones_lines_binds), &g_wow->shaders->m2_bones);
	gfx_create_input_layout(g_wow->device, &graphics->wmo_collisions_input_layout, g_wmo_collisions_binds, sizeof(g_wmo_collisions_binds) / sizeof(*g_wmo_collisions_binds), &g_wow->shaders->basic);
	gfx_create_input_layout(g_wow->device, &graphics->m2_collisions_input_layout, g_m2_collisions_binds, sizeof(g_m2_collisions_binds) / sizeof(*g_m2_collisions_binds), &g_wow->shaders->basic);
	gfx_create_input_layout(g_wow->device, &graphics->wmo_portals_input_layout, g_wmo_portals_binds, sizeof(g_wmo_portals_binds) / sizeof(*g_wmo_portals_binds), &g_wow->shaders->basic);
	gfx_create_input_layout(g_wow->device, &graphics->collisions_input_layout, g_collisions_binds, sizeof(g_collisions_binds) / sizeof(*g_collisions_binds), &g_wow->shaders->aabb);
	gfx_create_input_layout(g_wow->device, &graphics->wmo_lights_input_layout, g_wmo_lights_binds, sizeof(g_wmo_lights_binds) / sizeof(*g_wmo_lights_binds), &g_wow->shaders->basic);
	gfx_create_input_layout(g_wow->device, &graphics->m2_lights_input_layout, g_m2_lights_binds, sizeof(g_m2_lights_binds) / sizeof(*g_m2_lights_binds), &g_wow->shaders->basic);
	gfx_create_input_layout(g_wow->device, &graphics->aabb_input_layout, g_aabb_binds, sizeof(g_aabb_binds) / sizeof(*g_aabb_binds), &g_wow->shaders->aabb);
	graphics->wmo_collisions_triangles_pipeline_state = GFX_PIPELINE_STATE_INIT();
	graphics->m2_collisions_triangles_pipeline_state = GFX_PIPELINE_STATE_INIT();
	graphics->collisions_triangles_pipeline_state = GFX_PIPELINE_STATE_INIT();
	graphics->wmo_collisions_lines_pipeline_state = GFX_PIPELINE_STATE_INIT();
	graphics->m2_collisions_lines_pipeline_state = GFX_PIPELINE_STATE_INIT();
	graphics->collisions_lines_pipeline_state = GFX_PIPELINE_STATE_INIT();
	graphics->m2_bones_points_pipeline_state = GFX_PIPELINE_STATE_INIT();
	graphics->m2_bones_lines_pipeline_state = GFX_PIPELINE_STATE_INIT();
	graphics->wmo_portals_pipeline_state = GFX_PIPELINE_STATE_INIT();
	graphics->wmo_lights_pipeline_state = GFX_PIPELINE_STATE_INIT();
	graphics->m2_lights_pipeline_state = GFX_PIPELINE_STATE_INIT();
	graphics->aabb_pipeline_state = GFX_PIPELINE_STATE_INIT();
	gfx_create_pipeline_state(g_wow->device,
		&graphics->wmo_collisions_triangles_pipeline_state,
		&g_wow->shaders->wmo_collisions,
		&graphics->collisions_triangles_rasterizer_state,
		&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_R_NO],
		&graphics->world_blend_states[WORLD_BLEND_ALPHA],
		&graphics->wmo_collisions_input_layout,
		GFX_PRIMITIVE_TRIANGLES);
	gfx_create_pipeline_state(g_wow->device,
		&graphics->m2_collisions_triangles_pipeline_state,
		&g_wow->shaders->m2_collisions,
		&graphics->collisions_triangles_rasterizer_state,
		&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_R_NO],
		&graphics->world_blend_states[WORLD_BLEND_ALPHA],
		&graphics->m2_collisions_input_layout,
		GFX_PRIMITIVE_TRIANGLES);
	gfx_create_pipeline_state(g_wow->device,
		&graphics->collisions_triangles_pipeline_state,
		&g_wow->shaders->aabb,
		&graphics->collisions_triangles_rasterizer_state,
		&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_NO_NO],
		&graphics->world_blend_states[WORLD_BLEND_ALPHA],
		&graphics->collisions_input_layout,
		GFX_PRIMITIVE_TRIANGLES);
	gfx_create_pipeline_state(g_wow->device,
		&graphics->wmo_collisions_lines_pipeline_state,
		&g_wow->shaders->wmo_collisions,
		&graphics->collisions_lines_rasterizer_state,
		&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_R_NO],
		&graphics->world_blend_states[WORLD_BLEND_ALPHA],
		&graphics->wmo_collisions_input_layout,
		GFX_PRIMITIVE_TRIANGLES);
	gfx_create_pipeline_state(g_wow->device,
		&graphics->m2_collisions_lines_pipeline_state,
		&g_wow->shaders->m2_collisions,
		&graphics->collisions_lines_rasterizer_state,
		&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_R_NO],
		&graphics->world_blend_states[WORLD_BLEND_ALPHA],
		&graphics->m2_collisions_input_layout,
		GFX_PRIMITIVE_TRIANGLES);
	gfx_create_pipeline_state(g_wow->device,
		&graphics->collisions_lines_pipeline_state,
		&g_wow->shaders->aabb,
		&graphics->collisions_lines_rasterizer_state,
		&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_NO_NO],
		&graphics->world_blend_states[WORLD_BLEND_ALPHA],
		&graphics->collisions_input_layout,
		GFX_PRIMITIVE_TRIANGLES);
	gfx_create_pipeline_state(g_wow->device,
		&graphics->m2_bones_points_pipeline_state,
		&g_wow->shaders->m2_bones,
		&graphics->world_rasterizer_states[WORLD_RASTERIZER_UNCULLED],
		&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_NO_NO],
		&graphics->world_blend_states[WORLD_BLEND_ALPHA],
		&graphics->m2_bones_points_input_layout,
		GFX_PRIMITIVE_POINTS);
	gfx_create_pipeline_state(g_wow->device,
		&graphics->m2_bones_lines_pipeline_state,
		&g_wow->shaders->m2_bones,
		&graphics->world_rasterizer_states[WORLD_RASTERIZER_UNCULLED],
		&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_NO_NO],
		&graphics->world_blend_states[WORLD_BLEND_ALPHA],
		&graphics->m2_bones_lines_input_layout,
		GFX_PRIMITIVE_LINES);
	gfx_create_pipeline_state(g_wow->device,
		&graphics->wmo_portals_pipeline_state,
		&g_wow->shaders->wmo_portals,
		&graphics->world_rasterizer_states[WORLD_RASTERIZER_UNCULLED],
		&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_R_NO],
		&graphics->world_blend_states[WORLD_BLEND_ALPHA],
		&graphics->wmo_portals_input_layout,
		GFX_PRIMITIVE_TRIANGLES);
	gfx_create_pipeline_state(g_wow->device,
		&graphics->wmo_lights_pipeline_state,
		&g_wow->shaders->basic,
		&graphics->world_rasterizer_states[WORLD_RASTERIZER_UNCULLED],
		&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_NO_NO],
		&graphics->world_blend_states[WORLD_BLEND_ALPHA],
		&graphics->wmo_lights_input_layout,
		GFX_PRIMITIVE_POINTS);
	gfx_create_pipeline_state(g_wow->device,
		&graphics->m2_lights_pipeline_state,
		&g_wow->shaders->m2_lights,
		&graphics->world_rasterizer_states[WORLD_RASTERIZER_UNCULLED],
		&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_NO_NO],
		&graphics->world_blend_states[WORLD_BLEND_ALPHA],
		&graphics->m2_lights_input_layout,
		GFX_PRIMITIVE_POINTS);
	gfx_create_pipeline_state(g_wow->device,
		&graphics->aabb_pipeline_state,
		&g_wow->shaders->aabb,
		&graphics->world_rasterizer_states[WORLD_RASTERIZER_UNCULLED],
		&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_R_NO],
		&graphics->world_blend_states[WORLD_BLEND_ALPHA],
		&graphics->aabb_input_layout,
		GFX_PRIMITIVE_LINES);
#endif
	graphics->wmo_colored_input_layout = GFX_INPUT_LAYOUT_INIT();
	graphics->mclq_water_input_layout = GFX_INPUT_LAYOUT_INIT();
	graphics->mclq_magma_input_layout = GFX_INPUT_LAYOUT_INIT();
	graphics->particles_input_layout = GFX_INPUT_LAYOUT_INIT();
	graphics->wmo_mliq_input_layout = GFX_INPUT_LAYOUT_INIT();
	graphics->ribbons_input_layout = GFX_INPUT_LAYOUT_INIT();
	graphics->skybox_input_layout = GFX_INPUT_LAYOUT_INIT();
	graphics->mcnk_input_layout = GFX_INPUT_LAYOUT_INIT();
	graphics->text_input_layout = GFX_INPUT_LAYOUT_INIT();
	graphics->wdl_input_layout = GFX_INPUT_LAYOUT_INIT();
	graphics->wmo_input_layout = GFX_INPUT_LAYOUT_INIT();
	graphics->m2_input_layout = GFX_INPUT_LAYOUT_INIT();
	gfx_create_input_layout(g_wow->device, &graphics->wmo_colored_input_layout, g_wmo_colored_binds, sizeof(g_wmo_colored_binds) / sizeof(*g_wmo_colored_binds), &g_wow->shaders->wmo);
	gfx_create_input_layout(g_wow->device, &graphics->mclq_water_input_layout, g_mclq_water_binds, sizeof(g_mclq_water_binds) / sizeof(*g_mclq_water_binds), &g_wow->shaders->mclq_water);
	gfx_create_input_layout(g_wow->device, &graphics->mclq_magma_input_layout, g_mclq_magma_binds, sizeof(g_mclq_magma_binds) / sizeof(*g_mclq_magma_binds), &g_wow->shaders->mclq_magma);
	gfx_create_input_layout(g_wow->device, &graphics->particles_input_layout, g_particles_binds, sizeof(g_particles_binds) / sizeof(*g_particles_binds), &g_wow->shaders->particle);
	gfx_create_input_layout(g_wow->device, &graphics->wmo_mliq_input_layout, g_wmo_mliq_binds, sizeof(g_wmo_mliq_binds) / sizeof(*g_wmo_mliq_binds), &g_wow->shaders->mliq);
	gfx_create_input_layout(g_wow->device, &graphics->ribbons_input_layout, g_ribbons_binds, sizeof(g_ribbons_binds) / sizeof(*g_ribbons_binds), &g_wow->shaders->ribbon);
	gfx_create_input_layout(g_wow->device, &graphics->skybox_input_layout, g_skybox_binds, sizeof(g_skybox_binds) / sizeof(*g_skybox_binds), &g_wow->shaders->skybox);
	gfx_create_input_layout(g_wow->device, &graphics->mcnk_input_layout, g_mcnk_binds, sizeof(g_mcnk_binds) / sizeof(*g_mcnk_binds), &g_wow->shaders->mcnk);
	gfx_create_input_layout(g_wow->device, &graphics->text_input_layout, g_text_binds, sizeof(g_text_binds) / sizeof(*g_text_binds), &g_wow->shaders->text);
	gfx_create_input_layout(g_wow->device, &graphics->wdl_input_layout, g_wdl_binds, sizeof(g_wdl_binds) / sizeof(*g_wdl_binds), &g_wow->shaders->wdl);
	gfx_create_input_layout(g_wow->device, &graphics->wmo_input_layout, g_wmo_binds, sizeof(g_wmo_binds) / sizeof(*g_wmo_binds), &g_wow->shaders->wmo);
	gfx_create_input_layout(g_wow->device, &graphics->m2_input_layout, g_m2_binds, sizeof(g_m2_binds) / sizeof(*g_m2_binds), &g_wow->shaders->m2);
	graphics->mclq_water_pipeline_state = GFX_PIPELINE_STATE_INIT();
	graphics->mclq_magma_pipeline_state = GFX_PIPELINE_STATE_INIT();
	graphics->wmo_mliq_pipeline_state = GFX_PIPELINE_STATE_INIT();
	graphics->skybox_pipeline_state = GFX_PIPELINE_STATE_INIT();
	graphics->mcnk_pipeline_state = GFX_PIPELINE_STATE_INIT();
	graphics->text_pipeline_state = GFX_PIPELINE_STATE_INIT();
	graphics->wdl_pipeline_state = GFX_PIPELINE_STATE_INIT();
	gfx_create_pipeline_state(g_wow->device,
		&graphics->mclq_water_pipeline_state,
		&g_wow->shaders->mclq_water,
		&graphics->world_rasterizer_states[WORLD_RASTERIZER_CULLED],
		&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_RW_NO],
		&graphics->world_blend_states[WORLD_BLEND_ALPHA],
		&graphics->mclq_water_input_layout,
		GFX_PRIMITIVE_TRIANGLES);
	gfx_create_pipeline_state(g_wow->device,
		&graphics->mclq_magma_pipeline_state,
		&g_wow->shaders->mclq_magma,
		&graphics->world_rasterizer_states[WORLD_RASTERIZER_CULLED],
		&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_RW_NO],
		&graphics->world_blend_states[WORLD_BLEND_ALPHA],
		&graphics->mclq_magma_input_layout,
		GFX_PRIMITIVE_TRIANGLES);
	gfx_create_pipeline_state(g_wow->device,
		&graphics->wmo_mliq_pipeline_state,
		&g_wow->shaders->mliq,
		&graphics->world_rasterizer_states[WORLD_RASTERIZER_CULLED],
		&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_R_NO],
		&graphics->world_blend_states[WORLD_BLEND_ALPHA],
		&graphics->wmo_mliq_input_layout,
		GFX_PRIMITIVE_TRIANGLES);
	gfx_create_pipeline_state(g_wow->device,
		&graphics->skybox_pipeline_state,
		&g_wow->shaders->skybox,
		&graphics->world_rasterizer_states[WORLD_RASTERIZER_CULLED],
		&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_NO_R],
		&graphics->world_blend_states[WORLD_BLEND_ALPHA],
		&graphics->skybox_input_layout,
		GFX_PRIMITIVE_TRIANGLES);
	gfx_create_pipeline_state(g_wow->device,
		&graphics->mcnk_pipeline_state,
		&g_wow->shaders->mcnk,
		&graphics->world_rasterizer_states[WORLD_RASTERIZER_CULLED],
		&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_RW_W],
		&graphics->world_blend_states[WORLD_BLEND_ALPHA],
		&graphics->mcnk_input_layout,
		GFX_PRIMITIVE_TRIANGLES);
	gfx_create_pipeline_state(g_wow->device,
		&graphics->text_pipeline_state,
		&g_wow->shaders->text,
		&graphics->world_rasterizer_states[WORLD_RASTERIZER_UNCULLED],
		&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_R_NO],
		&graphics->world_blend_states[WORLD_BLEND_ALPHA],
		&graphics->text_input_layout,
		GFX_PRIMITIVE_TRIANGLES);
	gfx_create_pipeline_state(g_wow->device,
		&graphics->wdl_pipeline_state,
		&g_wow->shaders->wdl,
		&graphics->world_rasterizer_states[WORLD_RASTERIZER_CULLED],
		&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_NO_RW],
		&graphics->world_blend_states[WORLD_BLEND_ALPHA],
		&graphics->wdl_input_layout,
		GFX_PRIMITIVE_TRIANGLES);
	for (enum world_rasterizer_state rasterizer = 0; rasterizer < WORLD_RASTERIZER_LAST; ++rasterizer)
	{
		for (enum world_blend_state blend = 0; blend < WORLD_BLEND_LAST; ++blend)
		{
			graphics->wmo_colored_pipeline_states[rasterizer][blend] = GFX_PIPELINE_STATE_INIT();
			gfx_create_pipeline_state(g_wow->device,
				&graphics->wmo_colored_pipeline_states[rasterizer][blend],
				&g_wow->shaders->wmo,
				&graphics->world_rasterizer_states[rasterizer],
				&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_RW_W],
				&graphics->world_blend_states[blend],
				&graphics->wmo_colored_input_layout,
				GFX_PRIMITIVE_TRIANGLES);
		}
	}
	for (enum world_blend_state blend = 0; blend < WORLD_BLEND_LAST; ++blend)
	{
		graphics->particles_pipeline_states[blend] = GFX_PIPELINE_STATE_INIT();
		gfx_create_pipeline_state(g_wow->device,
			&graphics->particles_pipeline_states[blend],
			&g_wow->shaders->particle,
			&graphics->world_rasterizer_states[WORLD_RASTERIZER_UNCULLED],
			&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_R_NO],
			&graphics->world_blend_states[blend],
			&graphics->particles_input_layout,
			GFX_PRIMITIVE_TRIANGLES);
	}
	for (enum world_rasterizer_state rasterizer = 0; rasterizer < WORLD_RASTERIZER_LAST; ++rasterizer)
	{
		for (enum world_blend_state blend = 0; blend < WORLD_BLEND_LAST; ++blend)
		{
			graphics->wmo_pipeline_states[rasterizer][blend] = GFX_PIPELINE_STATE_INIT();
			gfx_create_pipeline_state(g_wow->device,
				&graphics->wmo_pipeline_states[rasterizer][blend],
				&g_wow->shaders->wmo,
				&graphics->world_rasterizer_states[rasterizer],
				&graphics->world_depth_stencil_states[WORLD_DEPTH_STENCIL_RW_W],
				&graphics->world_blend_states[blend],
				&graphics->wmo_input_layout,
				GFX_PRIMITIVE_TRIANGLES);
		}
	}
	for (enum world_rasterizer_state rasterizer = 0; rasterizer < WORLD_RASTERIZER_LAST; ++rasterizer)
	{
		for (enum world_depth_stencil_state depth_stencil = 0; depth_stencil < WORLD_DEPTH_STENCIL_LAST; ++depth_stencil)
		{
			for (enum world_blend_state blend = 0; blend < WORLD_BLEND_LAST; ++blend)
			{
				graphics->m2_pipeline_states[rasterizer][depth_stencil][blend] = GFX_PIPELINE_STATE_INIT();
				gfx_create_pipeline_state(g_wow->device,
					&graphics->m2_pipeline_states[rasterizer][depth_stencil][blend],
					&g_wow->shaders->m2,
					&graphics->world_rasterizer_states[rasterizer],
					&graphics->world_depth_stencil_states[depth_stencil],
					&graphics->world_blend_states[blend],
					&graphics->m2_input_layout,
					GFX_PRIMITIVE_TRIANGLES);
				graphics->ribbons_pipeline_states[rasterizer][depth_stencil][blend] = GFX_PIPELINE_STATE_INIT();
				gfx_create_pipeline_state(g_wow->device,
					&graphics->ribbons_pipeline_states[rasterizer][depth_stencil][blend],
					&g_wow->shaders->ribbon,
					&graphics->world_rasterizer_states[rasterizer],
					&graphics->world_depth_stencil_states[depth_stencil],
					&graphics->world_blend_states[blend],
					&graphics->ribbons_input_layout,
					GFX_PRIMITIVE_LINE_STRIP);
			}
		}
	}
	return true;
}

void graphics_clear(struct graphics *graphics)
{
	for (size_t i = 0; i < WORLD_DEPTH_STENCIL_LAST; ++i)
		gfx_delete_depth_stencil_state(g_wow->device, &graphics->world_depth_stencil_states[i]);
	for (size_t i = 0; i < WORLD_RASTERIZER_LAST; ++i)
		gfx_delete_rasterizer_state(g_wow->device, &graphics->world_rasterizer_states[i]);
	for (size_t i = 0; i < WORLD_BLEND_LAST; ++i)
		gfx_delete_blend_state(g_wow->device, &graphics->world_blend_states[i]);
#ifdef WITH_DEBUG_RENDERING
	gfx_delete_rasterizer_state(g_wow->device, &graphics->collisions_triangles_rasterizer_state);
	gfx_delete_rasterizer_state(g_wow->device, &graphics->collisions_lines_rasterizer_state);
	gfx_delete_input_layout(g_wow->device, &graphics->m2_bones_points_input_layout);
	gfx_delete_input_layout(g_wow->device, &graphics->m2_bones_lines_input_layout);
	gfx_delete_input_layout(g_wow->device, &graphics->wmo_collisions_input_layout);
	gfx_delete_input_layout(g_wow->device, &graphics->m2_collisions_input_layout);
	gfx_delete_input_layout(g_wow->device, &graphics->wmo_portals_input_layout);
	gfx_delete_input_layout(g_wow->device, &graphics->collisions_input_layout);
	gfx_delete_input_layout(g_wow->device, &graphics->wmo_lights_input_layout);
	gfx_delete_input_layout(g_wow->device, &graphics->m2_lights_input_layout);
	gfx_delete_input_layout(g_wow->device, &graphics->aabb_input_layout);
	gfx_delete_pipeline_state(g_wow->device, &graphics->wmo_collisions_triangles_pipeline_state);
	gfx_delete_pipeline_state(g_wow->device, &graphics->m2_collisions_triangles_pipeline_state);
	gfx_delete_pipeline_state(g_wow->device, &graphics->collisions_triangles_pipeline_state);
	gfx_delete_pipeline_state(g_wow->device, &graphics->wmo_collisions_lines_pipeline_state);
	gfx_delete_pipeline_state(g_wow->device, &graphics->m2_collisions_lines_pipeline_state);
	gfx_delete_pipeline_state(g_wow->device, &graphics->collisions_lines_pipeline_state);
	gfx_delete_pipeline_state(g_wow->device, &graphics->m2_bones_points_pipeline_state);
	gfx_delete_pipeline_state(g_wow->device, &graphics->m2_bones_lines_pipeline_state);
	gfx_delete_pipeline_state(g_wow->device, &graphics->wmo_portals_pipeline_state);
	gfx_delete_pipeline_state(g_wow->device, &graphics->wmo_lights_pipeline_state);
	gfx_delete_pipeline_state(g_wow->device, &graphics->m2_lights_pipeline_state);
	gfx_delete_pipeline_state(g_wow->device, &graphics->aabb_pipeline_state);
#endif
	gfx_delete_input_layout(g_wow->device, &graphics->wmo_colored_input_layout);
	gfx_delete_input_layout(g_wow->device, &graphics->mclq_water_input_layout);
	gfx_delete_input_layout(g_wow->device, &graphics->mclq_magma_input_layout);
	gfx_delete_input_layout(g_wow->device, &graphics->particles_input_layout);
	gfx_delete_input_layout(g_wow->device, &graphics->wmo_mliq_input_layout);
	gfx_delete_input_layout(g_wow->device, &graphics->ribbons_input_layout);
	gfx_delete_input_layout(g_wow->device, &graphics->skybox_input_layout);
	gfx_delete_input_layout(g_wow->device, &graphics->mcnk_input_layout);
	gfx_delete_input_layout(g_wow->device, &graphics->wdl_input_layout);
	gfx_delete_input_layout(g_wow->device, &graphics->wmo_input_layout);
	gfx_delete_input_layout(g_wow->device, &graphics->m2_input_layout);
	gfx_delete_pipeline_state(g_wow->device, &graphics->mclq_water_pipeline_state);
	gfx_delete_pipeline_state(g_wow->device, &graphics->mclq_magma_pipeline_state);
	gfx_delete_pipeline_state(g_wow->device, &graphics->wmo_mliq_pipeline_state);
	gfx_delete_pipeline_state(g_wow->device, &graphics->skybox_pipeline_state);
	gfx_delete_pipeline_state(g_wow->device, &graphics->mcnk_pipeline_state);
	gfx_delete_pipeline_state(g_wow->device, &graphics->wdl_pipeline_state);
	for (enum world_rasterizer_state rasterizer = 0; rasterizer < WORLD_RASTERIZER_LAST; ++rasterizer)
	{
		for (enum world_blend_state blend = 0; blend < WORLD_BLEND_LAST; ++blend)
		{
			gfx_delete_pipeline_state(g_wow->device, &graphics->wmo_colored_pipeline_states[rasterizer][blend]);
		}
	}
	for (enum world_blend_state blend = 0; blend < WORLD_BLEND_LAST; ++blend)
	{
		gfx_delete_pipeline_state(g_wow->device, &graphics->particles_pipeline_states[blend]);
	}
	for (enum world_rasterizer_state rasterizer = 0; rasterizer < WORLD_RASTERIZER_LAST; ++rasterizer)
	{
		for (enum world_blend_state blend = 0; blend < WORLD_BLEND_LAST; ++blend)
		{
			gfx_delete_pipeline_state(g_wow->device, &graphics->wmo_pipeline_states[rasterizer][blend]);
		}
	}
	for (enum world_rasterizer_state rasterizer = 0; rasterizer < WORLD_RASTERIZER_LAST; ++rasterizer)
	{
		for (enum world_depth_stencil_state depth_stencil = 0; depth_stencil < WORLD_DEPTH_STENCIL_LAST; ++depth_stencil)
		{
			for (enum world_blend_state blend = 0; blend < WORLD_BLEND_LAST; ++blend)
			{
				gfx_delete_pipeline_state(g_wow->device, &graphics->m2_pipeline_states[rasterizer][depth_stencil][blend]);
				gfx_delete_pipeline_state(g_wow->device, &graphics->ribbons_pipeline_states[rasterizer][depth_stencil][blend]);
			}
		}
	}
}
