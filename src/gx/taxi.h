#ifndef GX_TAXI_H
#define GX_TAXI_H

#include <gfx/objects.h>

#include <stddef.h>

struct gx_taxi
{
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t vertexes_buffer;
	gfx_buffer_t indices_buffer;
	size_t indices_nb;
	uint32_t mapid;
	bool initialized;
};

struct gx_taxi *gx_taxi_new(uint32_t mapid);
void gx_taxi_delete(struct gx_taxi *taxi);
void gx_taxi_render(struct gx_taxi *taxi);

#endif
