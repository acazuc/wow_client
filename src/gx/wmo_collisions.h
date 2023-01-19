#ifndef GX_WMO_COLLISIONS_H
#define GX_WMO_COLLISIONS_H

#include <gfx/objects.h>

#include <stdbool.h>

struct gx_wmo_collisions_init_data;
struct wow_mopy_data;
struct wow_mobn_node;
struct wow_vec3f;
struct mat4f;

struct gx_wmo_collisions
{
	struct gx_wmo_collisions_init_data *init_data;
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t triangles_uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t lines_uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t vertexes_buffer;
	uint32_t indices_nb;
};

void gx_wmo_collisions_init(struct gx_wmo_collisions *collisions);
void gx_wmo_collisions_destroy(struct gx_wmo_collisions *collisions);
bool gx_wmo_collisions_load(struct gx_wmo_collisions *collisions, const uint16_t *mobr, uint32_t mobr_nb, const uint16_t *movi, const struct wow_vec3f *movt, const struct wow_mopy_data *mopy);
void gx_wmo_collisions_initialize(struct gx_wmo_collisions *collisions);
void gx_wmo_collisions_render(struct gx_wmo_collisions *collisions, const struct mat4f *mvp, bool triangles);

#endif
