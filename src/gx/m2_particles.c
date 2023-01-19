#include "gx/m2_particles.h"
#include "gx/frame.h"
#include "gx/m2.h"

#include "graphics.h"
#include "shaders.h"
#include "camera.h"
#include "memory.h"
#include "cache.h"
#include "blp.h"
#include "log.h"
#include "wow.h"

#include <jks/mat4.h>
#include <jks/mat3.h>

#include <gfx/device.h>

#include <libwow/m2.h>

#include <stdlib.h>
#include <math.h>

#define MAX_PARTICLES 512

MEMORY_DECL(GX);

static void destroy_emitter(void *ptr)
{
	struct m2_particles_emitter *emitter = ptr;
	if (emitter->texture)
		cache_unref_by_ref_blp(g_wow->cache, emitter->texture);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		gfx_delete_attributes_state(g_wow->device, &emitter->attributes_states[i]);
		gfx_delete_buffer(g_wow->device, &emitter->vertexes_buffers[i]);
		gfx_delete_buffer(g_wow->device, &emitter->uniform_buffers[i]);
	}
	jks_array_destroy(&emitter->particles);
	jks_array_destroy(&emitter->vertexes);
}

struct gx_m2_particles *gx_m2_particles_new(struct gx_m2_instance *parent)
{
	struct gx_m2_particles *particles = mem_malloc(MEM_GX, sizeof(*particles));
	if (!particles)
	{
		LOG_ERROR("malloc failed");
		return NULL;
	}
	particles->parent = parent;
	jks_array_init(&particles->emitters, sizeof(struct m2_particles_emitter), destroy_emitter, &jks_array_memory_fn_GX);
	if (!jks_array_reserve(&particles->emitters, parent->parent->particles_nb))
	{
		LOG_ERROR("failed to reserve particle emitters");
		goto err;
	}
	for (size_t i = 0; i < parent->parent->particles_nb; ++i)
	{
		struct m2_particles_emitter *emitter = jks_array_grow(&particles->emitters, 1);
		if (!emitter)
		{
			LOG_ERROR("failed to grow emitter array");
			goto err;
		}
		struct wow_m2_particle *particle = &parent->parent->particles[i];
		struct wow_m2_texture *texture = &parent->parent->textures[particle->texture];
		for (size_t j = 0; j < RENDER_FRAMES_COUNT; ++j)
		{
			emitter->attributes_states[j] = GFX_ATTRIBUTES_STATE_INIT();
			emitter->vertexes_buffers[j] = GFX_BUFFER_INIT();
			emitter->uniform_buffers[j] = GFX_BUFFER_INIT();
		}
		if (!texture->type)
		{
			if (!cache_ref_by_key_blp(g_wow->cache, texture->filename, &emitter->texture))
			{
				LOG_ERROR("failed to get texture %s", texture->filename);
				emitter->texture = NULL;
			}
			blp_texture_ask_load(emitter->texture);
		}
		else
		{
			LOG_ERROR("unsupported texture type for particle: %d", (int)texture->type);
			emitter->texture = NULL;
		}
		emitter->emitter = &parent->parent->particles[i];
		emitter->last_spawned = g_wow->frametime;
		jks_array_init(&emitter->particles, sizeof(struct m2_particle), NULL, &jks_array_memory_fn_GX);
		jks_array_init(&emitter->vertexes, sizeof(struct shader_particle_input), NULL, &jks_array_memory_fn_GX);
		uint8_t blending_type;
		if (parent->parent->version >= 263)
			blending_type = emitter->emitter->blending_type2;
		else
			blending_type = emitter->emitter->blending_type;
		enum world_blend_state blend_state;
		switch (blending_type)
		{
			case 0:
				blend_state = WORLD_BLEND_OPAQUE;
				emitter->alpha_test = 0;
				break;
			case 1:
				blend_state = WORLD_BLEND_OPAQUE;
				emitter->alpha_test = 224 / 255.;
				break;
			case 2:
				blend_state = WORLD_BLEND_ALPHA;
				emitter->alpha_test = 0;
				break;
			case 3:
				blend_state = WORLD_BLEND_ADD;
				emitter->alpha_test = 0;
				break;
			case 4:
				blend_state = WORLD_BLEND_ADD;
				emitter->alpha_test = 0;
				break;
			default:
				LOG_INFO("unsupported blending: %d", (int)emitter->emitter->blending_type);
				blend_state = WORLD_BLEND_ALPHA;
				emitter->alpha_test = 0;
				break;
		}
		emitter->pipeline_state = blend_state;
	}
	particles->initialized = false;
	return particles;

err:
	gx_m2_particles_delete(particles);
	return NULL;
}

void gx_m2_particles_delete(struct gx_m2_particles *particles)
{
	if (!particles)
		return;
	jks_array_destroy(&particles->emitters);
	mem_free(MEM_GX, particles);
}

static struct vec3f get_gravity(struct gx_m2_particles *particles, struct wow_m2_particle *emitter)
{
	struct vec3f gravity;
	if (emitter->flags & 0x800000)
	{
		/* XXX */
		VEC3_SET(gravity, 0, -((float*)emitter->gravity.values)[0], 0);
	}
	else
	{
		float g;
		if (!m2_instance_get_track_value_float(particles->parent, &emitter->gravity, &g))
			g = 0;
		VEC3_SET(gravity, 0, -g, 0);
	}
	return gravity;
}

static struct vec3f update_acceleration(struct gx_m2_particles *particles, struct wow_m2_particle *emitter, struct m2_particle *particle)
{
	struct vec3f gravity = get_gravity(particles, emitter);
	struct vec3f abs_vel;
	struct vec3f force;
	struct vec3f ret;
	float force_factor = .5 * emitter->drag;
	VEC3_FN1(abs_vel, particle->velocity, fabs);
	VEC3_MUL(force, abs_vel, particle->velocity);
	VEC3_MULV(force, force, force_factor);
	VEC3_SUB(ret, gravity, force);
	return ret;
}

static void update_position(struct gx_m2_particles *particles, struct wow_m2_particle *emitter, struct m2_particle *particle)
{
	struct vec3f tmp1;
	struct vec3f tmp2;
	float dt = (g_wow->frametime - g_wow->lastframetime) / 1000000000.f;
	float tmpf = dt * dt * .5;
	VEC3_MULV(tmp1, particle->acceleration, tmpf);
	VEC3_MULV(tmp2, particle->velocity, dt);
	VEC3_ADD(particle->position, particle->position, tmp1);
	VEC3_ADD(particle->position, particle->position, tmp2);
	struct vec3f new_acceleration = update_acceleration(particles, emitter, particle);
	VEC3_ADD(tmp1, particle->acceleration, new_acceleration);
	tmpf = dt * .5;
	VEC3_MULV(tmp2, tmp1, tmpf);
	VEC3_ADD(particle->velocity, particle->velocity, tmp2);
}

static void update_particles(struct gx_m2_particles *particles, struct m2_particles_emitter *emitter)
{
	float dt = (g_wow->frametime - g_wow->lastframetime) / 1000000000.f;
	for (size_t i = 0; i < emitter->particles.size; ++i)
	{
		struct m2_particle *particle = JKS_ARRAY_GET(&emitter->particles, i, struct m2_particle);
		if (g_wow->frametime - particle->created > particle->lifespan)
		{
			jks_array_erase(&emitter->particles, i);
			i--;
			continue;
		}
		update_position(particles, emitter->emitter, particle);
		if (g_wow->frametime - particle->created < emitter->emitter->wind_time)
		{
			particle->position.x -= emitter->emitter->wind_vector.x * dt;
			particle->position.y -= emitter->emitter->wind_vector.z * dt;
			particle->position.z += emitter->emitter->wind_vector.y * dt;
		}
	}
	for (size_t i = 0; i < emitter->particles.size; ++i)
	{
		struct m2_particle *particle = JKS_ARRAY_GET(&emitter->particles, i, struct m2_particle);
		struct shader_particle_input *vertex = JKS_ARRAY_GET(&emitter->vertexes, i, struct shader_particle_input);
		vertex->position = particle->position;
		float lifetime = (g_wow->frametime - particle->created) / (float)particle->lifespan;
		struct vec4f color1;
		struct vec4f color2;
		float scale1;
		float scale2;
		float a;
		vertex->scale = particles->parent->scale;
		if (emitter->emitter->twinkle_percent != 0 && emitter->emitter->twinkle_scale_min != emitter->emitter->twinkle_scale_max)
		{
			float len = 1000000000 / emitter->emitter->twinkle_speed * 2;
			float p = fmod(g_wow->frametime - particle->created, len) / len;
			if (p <= .5)
				vertex->scale *= (2 * p) * (emitter->emitter->twinkle_scale_max - emitter->emitter->twinkle_scale_min) + emitter->emitter->twinkle_scale_min;
			else
				vertex->scale *= (2 * (p - .5)) * (emitter->emitter->twinkle_scale_min - emitter->emitter->twinkle_scale_max) + emitter->emitter->twinkle_scale_max;
		}
		if (lifetime <= emitter->emitter->mid_point)
		{
			a = lifetime / emitter->emitter->mid_point;
			VEC4_DIVV(color1, emitter->emitter->color_values[0], 255.);
			VEC4_DIVV(color2, emitter->emitter->color_values[1], 255.);
			scale1 = emitter->emitter->scale_values[0];
			scale2 = emitter->emitter->scale_values[1];
		}
		else
		{
			a = (lifetime - emitter->emitter->mid_point) / (1 - emitter->emitter->mid_point);
			VEC4_DIVV(color1, emitter->emitter->color_values[1], 255.);
			VEC4_DIVV(color2, emitter->emitter->color_values[2], 255.);
			scale1 = emitter->emitter->scale_values[1];
			scale2 = emitter->emitter->scale_values[2];
		}
		struct vec4f tmp1;
		struct vec4f tmp2;
		VEC4_SUB(tmp1, color2, color1);
		VEC4_MULV(tmp2, tmp1, a);
		VEC4_ADD(vertex->color, tmp2, color1);
		vertex->scale *= scale1 + (scale2 - scale1) * a;
		vertex->uv.x = 0;
		vertex->uv.y = 0;
		vertex->uv.z = 1.f / emitter->emitter->texture_dimensions_columns;
		vertex->uv.w = 1.f / emitter->emitter->texture_dimensions_rows;
		if (emitter->emitter->spin == 0)
		{
			vertex->matrix.x = 1;
			vertex->matrix.y = 0;
			vertex->matrix.z = 0;
			vertex->matrix.w = 1;
		}
		else
		{
			float t = emitter->emitter->spin * lifetime * M_PI * 2;
			float c = cos(t);
			float s = sin(t);
			vertex->matrix.x = c;
			vertex->matrix.y = s;
			vertex->matrix.z = -s;
			vertex->matrix.w = c;
		}
		/* XXX: per-emitter uniform (spin, texture dimensions) ? */
	}
}

static void create_particle(struct gx_m2_particles *particles, struct m2_particles_emitter *emitter)
{
	struct m2_particle *particle = jks_array_grow(&emitter->particles, 1);
	if (!particle)
	{
		LOG_ERROR("failed to grow particle array");
		return;
	}
	if (!jks_array_grow(&emitter->vertexes, 1))
	{
		jks_array_resize(&emitter->particles, emitter->particles.size - 1);
		LOG_ERROR("failed to grow vertexes array");
		return;
	}
	struct vec4f tmp1;
	struct vec4f tmp2;
	tmp1.x = 0;
	tmp1.y = 0;
	tmp1.z = 0;
	tmp1.w = 1;
	VEC3_SETV(particle->acceleration, 0);
	float speed;
	if (!m2_instance_get_track_value_float(particles->parent, &emitter->emitter->emission_speed, &speed))
		speed = 0;
	float speed_variation;
	if (!m2_instance_get_track_value_float(particles->parent, &emitter->emitter->speed_variation, &speed_variation))
		speed_variation = 0;
	float vertical_range;
	if (!m2_instance_get_track_value_float(particles->parent, &emitter->emitter->vertical_range, &vertical_range))
		vertical_range = 0;
	vertical_range /= 2;
	float horizontal_range;
	if (!m2_instance_get_track_value_float(particles->parent, &emitter->emitter->horizontal_range, &horizontal_range))
		horizontal_range = 0;
	struct vec4f velocity;
	switch (emitter->emitter->emitter_type)
	{
		case 1:
			VEC4_SET(velocity, 0, speed * (1 - speed_variation * (rand() / (float)RAND_MAX)), 0, 0);
			break;
		case 2:
		{
			float min;
			float max;
			if (!m2_instance_get_track_value_float(particles->parent, &emitter->emitter->emission_area_length, &min))
				min = 0;
			if (!m2_instance_get_track_value_float(particles->parent, &emitter->emitter->emission_area_width, &max))
				max = 0;
			float diff = max - min;
			float rnd = rand() / (float)RAND_MAX * M_PI * 2;
			struct vec3f add;
			add.x = cos(rnd);
			add.y = 0;
			add.z = sin(rnd);
			if (rand() > RAND_MAX / 2)
				add.x = -add.x;
			if (rand() > RAND_MAX / 2)
				add.z = -add.z;
			VEC4_SET(velocity, -add.x, 0, -add.z, 0);
			float spd = -(speed * (1 - speed_variation * (rand() / (float)RAND_MAX)));
			VEC3_MULV(velocity, velocity, spd);
			float len = (rand() / (float)RAND_MAX) * diff + min;
			VEC3_MULV(add, add, len);
			VEC3_ADD(tmp1, tmp1, add);
			break;
		}
		default:
			//LOG_INFO("unhandled emitter type: %d", emitter->emitter->emitter_type);
			return;
	}
	particle->created = g_wow->frametime;
	struct mat4f orientation;
	struct mat4f tmp_mat;
	MAT4_IDENTITY(orientation);
	MAT4_ROTATEZ(float, tmp_mat, orientation, vertical_range);
	MAT4_ROTATEY(float, orientation, tmp_mat, horizontal_range);
	MAT4_VEC4_MUL(tmp2, orientation, tmp1);
	tmp1 = tmp2;
	tmp1.x += emitter->emitter->position.x;
	tmp1.y += emitter->emitter->position.z;
	tmp1.z -= emitter->emitter->position.y;
	MAT4_VEC4_MUL(tmp2, orientation, velocity);
	velocity = tmp2;
	if (emitter->emitter->bone != (uint16_t)-1 && emitter->emitter->bone < particles->parent->parent->bones_nb)
	{
		struct mat4f *bone_mat = JKS_ARRAY_GET(&particles->parent->render_frames[g_wow->cull_frame_id].bone_mats, emitter->emitter->bone, struct mat4f);
		MAT4_VEC4_MUL(tmp2, *bone_mat, velocity);
		velocity = tmp2;
		MAT4_VEC4_MUL(tmp2, *bone_mat, tmp1);
		tmp1 = tmp2;
	}
	MAT4_VEC4_MUL(particle->position, particles->parent->m, tmp1);
	MAT4_VEC4_MUL(particle->velocity, particles->parent->m, velocity);
	float lifespan;
	if (!m2_instance_get_track_value_float(particles->parent, &emitter->emitter->lifespan, &lifespan))
		lifespan = 0;
	particle->lifespan = lifespan * 1000000000;
}

static void render_particles(struct m2_particles_emitter *emitter)
{
	if (!emitter->particles.size)
		return;
	gfx_set_buffer_data(&emitter->vertexes_buffers[g_wow->draw_frame_id], emitter->vertexes.data, sizeof(struct shader_particle_input) * emitter->particles.size, 0);
	gfx_bind_attributes_state(g_wow->device, &emitter->attributes_states[g_wow->draw_frame_id], &g_wow->graphics->particles_input_layout);
	gfx_bind_pipeline_state(g_wow->device, &g_wow->graphics->particles_pipeline_states[emitter->pipeline_state]);
	struct shader_particle_model_block model_block;
	model_block.alpha_test = emitter->alpha_test;
	gfx_set_buffer_data(&emitter->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
	gfx_bind_constant(g_wow->device, 1, &emitter->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	blp_texture_bind(emitter->texture, 0);
	gfx_draw(g_wow->device, emitter->particles.size, 0);
}

static void initialize(struct gx_m2_particles *particles)
{
	for (size_t i = 0; i < particles->emitters.size; ++i)
	{
		struct m2_particles_emitter *emitter = JKS_ARRAY_GET(&particles->emitters, i, struct m2_particles_emitter);
		for (size_t j = 0; j < RENDER_FRAMES_COUNT; ++j)
		{
			gfx_attribute_bind_t binds[] =
			{
				{&emitter->vertexes_buffers[j], sizeof(struct shader_particle_input), offsetof(struct shader_particle_input, position)},
				{&emitter->vertexes_buffers[j], sizeof(struct shader_particle_input), offsetof(struct shader_particle_input, color)},
				{&emitter->vertexes_buffers[j], sizeof(struct shader_particle_input), offsetof(struct shader_particle_input, uv)},
				{&emitter->vertexes_buffers[j], sizeof(struct shader_particle_input), offsetof(struct shader_particle_input, matrix)},
				{&emitter->vertexes_buffers[j], sizeof(struct shader_particle_input), offsetof(struct shader_particle_input, scale)},
			};
			gfx_create_buffer(g_wow->device, &emitter->vertexes_buffers[j], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_particle_input) * MAX_PARTICLES, GFX_BUFFER_STREAM);
			gfx_create_buffer(g_wow->device, &emitter->uniform_buffers[j], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_particle_model_block), GFX_BUFFER_STREAM);
			gfx_create_attributes_state(g_wow->device, &emitter->attributes_states[j], binds, sizeof(binds) / sizeof(*binds), NULL, 0);
		}
	}
	particles->initialized = true;
}

void gx_m2_particles_update(struct gx_m2_particles *particles)
{
	for (size_t i = 0; i < particles->emitters.size; ++i)
	{
		struct m2_particles_emitter *emitter = JKS_ARRAY_GET(&particles->emitters, i, struct m2_particles_emitter);
		update_particles(particles, emitter);
		float rate;
		if (!m2_instance_get_track_value_float(particles->parent, &emitter->emitter->emission_rate, &rate))
			rate = 0;
		if (rate > 0)
		{
			if (rate < 0)
				rate = -rate;
			uint64_t delay = 1000000000.f / rate;
			size_t j = 0;
			while (g_wow->frametime - emitter->last_spawned > delay)
			{
				if (j == 5)
				{
					/* skip if too much particles must be created (avoiding freeze) */
					emitter->last_spawned = g_wow->frametime;
					break;
				}
				while (emitter->particles.size >= MAX_PARTICLES)
					jks_array_erase(&emitter->particles, 0);
				create_particle(particles, emitter);
				emitter->last_spawned += delay;
				++j;
			}
		}
	}
}

void gx_m2_particles_render(struct gx_m2_particles *particles)
{
	if (!particles->initialized)
		initialize(particles);
	for (size_t i = 0; i < particles->emitters.size; ++i)
	{
		struct m2_particles_emitter *emitter = JKS_ARRAY_GET(&particles->emitters, i, struct m2_particles_emitter);
		render_particles(emitter);
	}
}
