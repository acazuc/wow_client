#ifndef GX_MCNK_H
#define GX_MCNK_H

#ifdef WITH_DEBUG_RENDERING
# include "gx/aabb.h"
#endif

#include <jks/frustum.h>
#include <jks/array.h>
#include <jks/aabb.h>
#include <jks/vec3.h>
#include <jks/vec2.h>

#include <gfx/objects.h>

#include <stdbool.h>

#define GX_MCNK_BATCHES_NB 256

struct gx_m2_instance;
struct wow_adt_file;
struct blp_texture;
struct map_tile;
struct gx_m2;

struct gx_mcnk_batch_frame
{
	float distance_to_camera;
	bool culled;
};

struct gx_mcnk_batch
{
	struct gx_mcnk_batch_frame render_frames[RENDER_FRAMES_COUNT];
	uint32_t indices_offsets[3];
	uint16_t indices_nbs[3];
	struct jks_array doodads_to_aabb; /* uint32_t */
	struct jks_array wmos_to_aabb; /* uint32_t */
	struct gx_mcnk_ground_effect *ground_effect;
	uint16_t textures[4];
	struct vec2f layers_animations[4];
#ifdef WITH_DEBUG_RENDERING
	struct gx_aabb doodads_gx_aabb;
	struct gx_aabb wmos_gx_aabb;
	struct gx_aabb gx_aabb;
#endif
	enum frustum_result doodads_frustum_result;
	enum frustum_result wmos_frustum_result;
	enum frustum_result frustum_result;
};

struct gx_mcnk_frame
{
	uint8_t to_render[GX_MCNK_BATCHES_NB];
	uint32_t to_render_nb;
	struct mat4f mvp;
	struct mat4f mv;
};

struct gx_mcnk
{
	struct map_tile *parent;
	struct gx_mcnk_frame render_frames[RENDER_FRAMES_COUNT];
	struct mcnk_init_data *init_data;
	struct gx_mcnk_batch batches[GX_MCNK_BATCHES_NB];
	struct gx_m2 *ground_effect_doodads;
	struct blp_texture **specular_textures;
	struct blp_texture **diffuse_textures;
	uint32_t textures_nb;
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t batches_uniform_buffer;
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t vertexes_buffer;
	gfx_buffer_t indices_buffer;
	gfx_texture_t alpha_texture;
#ifdef WITH_DEBUG_RENDERING
	struct gx_aabb doodads_gx_aabb;
	struct gx_aabb wmos_gx_aabb;
	struct gx_aabb gx_aabb;
#endif
	struct mat4f m;
	enum frustum_result doodads_frustum_result;
	enum frustum_result wmos_frustum_result;
	uint32_t batch_uniform_padded_size;
	uint8_t layers;
	float distance_to_camera;
	bool has_doodads_aabb;
	bool has_wmos_aabb;
	bool initialized;
	bool holes;
	bool in_objects_render_list; /* XXX should be atomic for multi-threaded culling */
	bool in_render_list;
};

struct gx_mcnk *gx_mcnk_new(struct map_tile *parent, struct wow_adt_file *file);
void gx_mcnk_delete(struct gx_mcnk *mcnk);
int gx_mcnk_initialize(struct gx_mcnk *mcnk);
void gx_mcnk_cull(struct gx_mcnk *mcnk);
void gx_mcnk_add_objects_to_render(struct gx_mcnk *mcnk);
void gx_mcnk_render(struct gx_mcnk *mcnk);
#ifdef WITH_DEBUG_RENDERING
void gx_mcnk_render_aabb(struct gx_mcnk *mcnk);
#endif

#endif
