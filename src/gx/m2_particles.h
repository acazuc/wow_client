#ifndef GX_M2_PARTICLES_H
#define GX_M2_PARTICLES_H

#include <jks/array.h>
#include <jks/vec4.h>
#include <jks/vec3.h>

#include <gfx/objects.h>

#include <stdbool.h>

#define MAX_PARTICLES 1024 /* XXX for some reasons, my computer freeze if this value is too low... */

struct wow_m2_particle;
struct gx_m2_instance;
struct blp_texture;

struct m2_particle
{
	uint64_t created;
	uint64_t lifespan;
	float spin_random;
	struct vec4f position;
	struct vec4f velocity;
};

struct m2_particles_emitter
{
	struct wow_m2_particle *emitter;
	struct blp_texture *texture;
	gfx_attributes_state_t attributes_states[RENDER_FRAMES_COUNT];
	gfx_buffer_t vertexes_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	struct jks_array particles; /* struct m2_particle */
	struct jks_array vertexes; /* struct shader_particle_input */
	uint32_t pipeline_state;
	float alpha_test;
	uint64_t last_spawned;
	uint16_t emitter_type;
};

struct gx_m2_particles
{
	struct gx_m2_instance *parent;
	struct jks_array emitters; /* struct m2_particles_emitter */
	bool initialized;
};

struct gx_m2_particles *gx_m2_particles_new(struct gx_m2_instance *parent);
void gx_m2_particles_delete(struct gx_m2_particles *particles);
void gx_m2_particles_update(struct gx_m2_particles *particles);
void gx_m2_particles_render(struct gx_m2_particles *particles);

#endif
