#ifndef GX_SKYBOX_H
#define GX_SKYBOX_H

#include "simplex.h"

#include <jks/array.h>
#include <jks/vec3.h>

#include <gfx/objects.h>

#include <stdbool.h>

#define SKYBOX_INT_VALUES  18
#define SKYBOX_FLOAT_VALUES 6

#define SKYBOX_INT_DIFFUSE  0
#define SKYBOX_INT_AMBIENT  1
#define SKYBOX_INT_SKY0     2
#define SKYBOX_INT_SKY1     3
#define SKYBOX_INT_SKY2     4
#define SKYBOX_INT_SKY3     5
#define SKYBOX_INT_SKY4     6
#define SKYBOX_INT_FOG      7
#define SKYBOX_INT_UNK      8
#define SKYBOX_INT_SUN      9
#define SKYBOX_INT_SUN_HALO 10
#define SKYBOX_INT_CLOUD1   11
#define SKYBOX_INT_CLOUD2   12
#define SKYBOX_INT_CLOUD3   13
#define SKYBOX_INT_OCEAN1   14
#define SKYBOX_INT_OCEAN2   15
#define SKYBOX_INT_RIVER1   16
#define SKYBOX_INT_RIVER2   17

#define SKYBOX_FLOAT_FOG_END   0
#define SKYBOX_FLOAT_FOG_START 1
#define SKYBOX_FLOAT_GLOW      2
#define SKYBOX_FLOAT_CLOUD     3
#define SKYBOX_FLOAT_UNK1      4
#define SKYBOX_FLOAT_UNK2      5

struct gx_m2_instance;

struct gx_skybox
{
	float float_values[SKYBOX_FLOAT_VALUES];
	struct vec3f int_values[SKYBOX_INT_VALUES];
	struct jks_array entries; /* struct skybox_entry */
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t m2_uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t vertexes_buffer;
	gfx_buffer_t indices_buffer;
	gfx_texture_t clouds[2];
	struct simplex_noise clouds_noise;
	struct gx_m2_instance *skybox_m2;
	char *current_skybox;
	uint32_t default_skybox;
	bool has_default_skybox;
	uint32_t indices_nb;
	float ocean_min_alpha;
	float ocean_max_alpha;
	float river_min_alpha;
	float river_max_alpha;
	float glow;
	float inner_radius;
	float outer_radius;
};

struct gx_skybox *gx_skybox_new(uint32_t mapid);
void gx_skybox_delete(struct gx_skybox *skybox);
void gx_skybox_update(struct gx_skybox *skybox);
void gx_skybox_render(struct gx_skybox *skybox);

#endif
