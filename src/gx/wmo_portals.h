#ifndef GX_WMO_PORTALS_H
#define GX_WMO_PORTALS_H

#include "shaders.h"

#include <jks/array.h>
#include <jks/mat4.h>

#include <gfx/objects.h>

struct gx_wmo_portals_init_data;
struct gx_wmo_instance;
struct wow_mopt_data;
struct wow_vec3f;

struct gx_wmo_portals
{
	struct gx_wmo_portals_init_data *init_data;
	struct jks_array portals; /* gx_wmo_portal */
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t vertexes_buffer;
	gfx_buffer_t indices_buffer;
};

void gx_wmo_portals_init(struct gx_wmo_portals *portals);
void gx_wmo_portals_destroy(struct gx_wmo_portals *portals);
bool gx_wmo_portals_load(struct gx_wmo_portals *portals, const struct wow_mopt_data *mopt, uint32_t mopt_nb, const struct wow_vec3f *mopv, uint32_t mopv_nb);
void gx_wmo_portals_initialize(struct gx_wmo_portals *portals);
void gx_wmo_portals_render(struct gx_wmo_portals *portals, struct gx_wmo_instance *instance);

#endif
