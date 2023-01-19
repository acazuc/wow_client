#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <gfx/objects.h>

enum world_blend_state
{
	WORLD_BLEND_OPAQUE,
	WORLD_BLEND_ALPHA,
	WORLD_BLEND_NO_ALPHA_ADD,
	WORLD_BLEND_ADD,
	WORLD_BLEND_MOD,
	WORLD_BLEND_MOD2X,
	WORLD_BLEND_LAST
};

enum world_depth_stencil_state
{
	WORLD_DEPTH_STENCIL_NO_NO,
	WORLD_DEPTH_STENCIL_NO_R,
	WORLD_DEPTH_STENCIL_NO_W,
	WORLD_DEPTH_STENCIL_NO_RW,
	WORLD_DEPTH_STENCIL_R_NO,
	WORLD_DEPTH_STENCIL_R_R,
	WORLD_DEPTH_STENCIL_R_W,
	WORLD_DEPTH_STENCIL_R_RW,
	WORLD_DEPTH_STENCIL_W_NO,
	WORLD_DEPTH_STENCIL_W_R,
	WORLD_DEPTH_STENCIL_W_W,
	WORLD_DEPTH_STENCIL_W_RW,
	WORLD_DEPTH_STENCIL_RW_NO,
	WORLD_DEPTH_STENCIL_RW_R,
	WORLD_DEPTH_STENCIL_RW_W,
	WORLD_DEPTH_STENCIL_RW_RW,
	WORLD_DEPTH_STENCIL_LAST
};

enum world_rasterizer_state
{
	WORLD_RASTERIZER_CULLED,
	WORLD_RASTERIZER_UNCULLED,
	WORLD_RASTERIZER_LAST
};

struct graphics
{
	gfx_depth_stencil_state_t world_depth_stencil_states[WORLD_DEPTH_STENCIL_LAST];
	gfx_rasterizer_state_t world_rasterizer_states[WORLD_RASTERIZER_LAST];
	gfx_blend_state_t world_blend_states[WORLD_BLEND_LAST];
#ifdef WITH_DEBUG_RENDERING
	gfx_rasterizer_state_t collisions_triangles_rasterizer_state;
	gfx_rasterizer_state_t collisions_lines_rasterizer_state;
	gfx_pipeline_state_t wmo_collisions_triangles_pipeline_state;
	gfx_pipeline_state_t m2_collisions_triangles_pipeline_state;
	gfx_pipeline_state_t collisions_triangles_pipeline_state;
	gfx_pipeline_state_t wmo_collisions_lines_pipeline_state;
	gfx_pipeline_state_t m2_collisions_lines_pipeline_state;
	gfx_pipeline_state_t collisions_lines_pipeline_state;
	gfx_pipeline_state_t m2_bones_points_pipeline_state;
	gfx_pipeline_state_t m2_bones_lines_pipeline_state;
	gfx_pipeline_state_t wmo_portals_pipeline_state;
	gfx_pipeline_state_t wmo_lights_pipeline_state;
	gfx_pipeline_state_t m2_lights_pipeline_state;
	gfx_pipeline_state_t aabb_pipeline_state;
	gfx_input_layout_t m2_bones_points_input_layout;
	gfx_input_layout_t m2_bones_lines_input_layout;
	gfx_input_layout_t wmo_collisions_input_layout;
	gfx_input_layout_t m2_collisions_input_layout;
	gfx_input_layout_t wmo_portals_input_layout;
	gfx_input_layout_t collisions_input_layout;
	gfx_input_layout_t wmo_lights_input_layout;
	gfx_input_layout_t m2_lights_input_layout;
	gfx_input_layout_t aabb_input_layout;
#endif
	gfx_pipeline_state_t wmo_colored_pipeline_states[WORLD_RASTERIZER_LAST][WORLD_BLEND_LAST];
	gfx_pipeline_state_t mclq_water_pipeline_state;
	gfx_pipeline_state_t mclq_magma_pipeline_state;
	gfx_pipeline_state_t particles_pipeline_states[WORLD_BLEND_LAST];
	gfx_pipeline_state_t wmo_mliq_pipeline_state;
	gfx_pipeline_state_t skybox_pipeline_state;
	gfx_pipeline_state_t mcnk_pipeline_state;
	gfx_pipeline_state_t text_pipeline_state;
	gfx_pipeline_state_t wdl_pipeline_state;
	gfx_pipeline_state_t wmo_pipeline_states[WORLD_RASTERIZER_LAST][WORLD_BLEND_LAST];
	gfx_pipeline_state_t m2_pipeline_states[WORLD_RASTERIZER_LAST][WORLD_DEPTH_STENCIL_LAST][WORLD_BLEND_LAST];
	gfx_input_layout_t wmo_colored_input_layout;
	gfx_input_layout_t mclq_water_input_layout;
	gfx_input_layout_t mclq_magma_input_layout;
	gfx_input_layout_t particles_input_layout;
	gfx_input_layout_t wmo_mliq_input_layout;
	gfx_input_layout_t skybox_input_layout;
	gfx_input_layout_t mcnk_input_layout;
	gfx_input_layout_t text_input_layout;
	gfx_input_layout_t wdl_input_layout;
	gfx_input_layout_t wmo_input_layout;
	gfx_input_layout_t m2_input_layout;
};

bool graphics_init(struct graphics *graphics);
void graphics_clear(struct graphics *graphics);
bool graphics_build_world_rasterizer_states(struct graphics *graphics);

#endif
