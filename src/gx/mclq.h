#ifndef GX_MCLQ_H
#define GX_MCLQ_H

#ifdef WITH_DEBUG_RENDERING
# include "gx/aabb.h"
#endif

#include <jks/aabb.h>
#include <jks/vec3.h>
#include <jks/vec2.h>

#include <gfx/objects.h>

#include <stdbool.h>

#define GX_MCLQ_LIQUIDS_NB 4

struct gx_mclq_init_data;
struct wow_adt_file;
struct map_tile;

struct gx_mclq_batch_frame
{
	bool culled;
};

struct gx_mclq_batch
{
	struct gx_mclq_batch_frame render_frames[RENDER_FRAMES_COUNT];
#ifdef WITH_DEBUG_RENDERING
	struct gx_aabb gx_aabb;
#endif
	uint32_t indices_offset;
	uint16_t indices_nb;
	struct vec3f center;
	struct aabb aabb;
};

struct gx_mclq_liquid
{
	struct gx_mclq_init_data *init_data;
	struct gx_mclq_batch *batches;
	uint32_t batches_nb;
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t vertexes_buffer;
	gfx_buffer_t indices_buffer;
	gfx_buffer_t depths_buffer;
#ifdef WITH_DEBUG_RENDERING
	struct gx_aabb gx_aabb;
#endif
	struct aabb aabb;
	bool in_render_list;
};

struct gx_mclq_frame
{
	struct mat4f mvp;
	struct mat4f mv;
};

struct gx_mclq
{
	struct map_tile *parent;
	struct gx_mclq_frame render_frames[RENDER_FRAMES_COUNT];
	struct gx_mclq_liquid liquids[GX_MCLQ_LIQUIDS_NB];
	struct vec3f pos;
	struct mat4f m;
	uint8_t type;
	bool initialized;
};

struct gx_mclq *gx_mclq_new(struct map_tile *parent, struct wow_adt_file *file);
void gx_mclq_delete(struct gx_mclq *mclq);
int gx_mclq_initialize(struct gx_mclq *mclq);
void gx_mclq_cull(struct gx_mclq *mclq);
void gx_mclq_render(struct gx_mclq *mclq, uint8_t type);
#ifdef WITH_DEBUG_RENDERING
void gx_mclq_render_aabb(struct gx_mclq *mclq);
#endif

#endif
