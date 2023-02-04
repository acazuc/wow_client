#ifndef GX_M2_RIBBONS_H
#define GX_M2_RIBBONS_H

#include <jks/array.h>

#include <gfx/objects.h>

#define MAX_RIBBONS 512

struct gx_m2_ribbon_point
{
};

struct gx_m2_ribbons_emitter
{
	gfx_attributes_state_t attributes_states[RENDER_FRAMES_COUNT];
	gfx_buffer_t vertexes_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	struct jks_array vertexes; /* struct shader_ribbon_input */
	struct jks_array points; /* struct gx_m2_ribbon_point */
	uint32_t pipeline_state;
	uint64_t last_spawned;
};

struct gx_m2_ribbons
{
	struct gx_m2_instance *parent;
	struct jks_array emitters; /* struct gx_m2_ribbons_emitter */
	bool initialized;
};

struct gx_m2_ribbons *gx_m2_ribbons_new(struct gx_m2_instance *parent);
void gx_m2_ribbons_delete(struct gx_m2_ribbons *ribbons);
void gx_m2_ribbons_update(struct gx_m2_ribbons *ribbons);
void gx_m2_ribbons_render(struct gx_m2_ribbons *ribbons);

#endif
