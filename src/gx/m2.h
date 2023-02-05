#ifndef GX_M2_H
#define GX_M2_H

#ifdef WITH_DEBUG_RENDERING
# include "gx/m2_collisions.h"
# include "gx/m2_lights.h"
# include "gx/m2_bones.h"
# include "gx/aabb.h"
#endif

#include <jks/array.h>
#include <jks/aabb.h>
#include <jks/vec2.h>

#include <gfx/objects.h>

#include <libwow/m2.h>

#include <stdbool.h>

struct gx_m2_particles;
struct gx_m2_ribbons;
struct blp_texture;

struct gx_m2_render_params
{
	struct vec3f fog_color;
	float aspect;
};

struct gx_m2_instance;
struct gx_m2_texture;
struct gx_m2_profile;
struct gx_m2_batch;
struct gx_m2;

struct gx_m2_texture
{
	struct blp_texture *texture;
	uint32_t transform;
	uint32_t flags;
	uint8_t type;
	bool has_transform;
	bool initialized;
};

struct gx_m2_batch
{
	struct gx_m2_profile *parent;
	uint16_t combiners[2];
	uint32_t pipeline_state;
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	struct gx_m2_texture textures[2];
	struct vec3f fog_color;
	uint16_t skin_section_id;
	uint16_t color_transform;
	uint16_t material;
	uint16_t texture_weight;
	uint32_t indices_offset;
	uint32_t indices_nb;
	uint8_t priority_plane;
	uint8_t flags;
	uint8_t id;
	float alpha_test;
	bool has_color_transform;
	bool has_texture_weight;
	bool fog_override;
	bool blending;
};

struct gx_m2_profile
{
	struct jks_array batches; /* gx_m2_batch_t */
	struct jks_array transparent_batches; /* uint8_t */
	struct jks_array opaque_batches; /* uint8_t */
	struct gx_m2 *parent;
	bool initialized;
};

struct gx_m2_frame
{
	struct jks_array to_render; /* gx_m2_instance_t* */
};

struct gx_m2
{
	struct gx_m2_frame render_frames[RENDER_FRAMES_COUNT];
	struct jks_array instances; /* gx_m2_instance_t */
	struct jks_array profiles; /* gx_m2_profile_t */
	struct wow_m2_playable_animation *playable_animations;
	uint32_t playable_animations_nb;
	struct wow_m2_texture_transform *texture_transforms;
	uint32_t texture_transforms_nb;
	struct wow_m2_texture_weight *texture_weights;
	uint32_t texture_weights_nb;
	struct wow_m2_attachment *attachments;
	uint32_t attachments_nb;
	struct wow_m2_sequence *sequences;
	uint32_t sequences_nb;
	struct wow_m2_particle *particles;
	uint32_t particles_nb;
	struct wow_m2_ribbon *ribbons;
	uint32_t ribbons_nb;
	struct wow_vec3f *collision_vertexes;
	uint32_t collision_vertexes_nb;
	struct wow_vec3f *collision_normals;
	uint32_t collision_normals_nb;
	uint16_t *collision_triangles;
	uint32_t collision_triangles_nb;
	uint16_t *attachment_lookups;
	uint32_t attachment_lookups_nb;
	uint16_t *key_bone_lookups;
	uint32_t key_bone_lookups_nb;
	uint16_t *sequence_lookups;
	uint32_t sequence_lookups_nb;
	uint32_t *global_sequences;
	uint32_t global_sequences_nb;
	uint16_t *bone_lookups;
	uint32_t bone_lookups_nb;
	struct wow_m2_material *materials;
	uint32_t materials_nb;
	struct wow_m2_texture *textures;
	uint32_t textures_nb;
	struct wow_m2_vertex *vertexes;
	uint32_t vertexes_nb;
	struct wow_m2_camera *cameras;
	uint32_t cameras_nb;
	struct wow_m2_color *colors;
	uint32_t colors_nb;
	struct wow_m2_light *lights;
	uint32_t lights_nb;
	struct wow_m2_bone *bones;
	uint32_t bones_nb;
	struct jks_array indices; /* uint16_t */
	bool load_asked;
	bool invalid;
	bool loading;
	bool loaded;
	bool skybox;
	gfx_buffer_t vertexes_buffer;
	gfx_buffer_t indices_buffer;
#ifdef WITH_DEBUG_RENDERING
	struct gx_m2_collisions gx_collisions;
	struct gx_m2_lights gx_lights;
	struct gx_m2_bones gx_bones;
#endif
	gfx_attributes_state_t attributes_state;
	char *filename;
	struct aabb caabb;
	struct aabb aabb;
	float collision_sphere_radius;
	uint32_t flags;
	uint32_t version;
	float render_distance;
	bool has_transparent_batches;
	bool has_billboard_bones;
	bool has_opaque_batches;
	bool in_render_list;
	bool initialized;
};

struct gx_m2 *gx_m2_new(const char *filename);
void gx_m2_delete(struct gx_m2 *m2);
bool gx_m2_ask_load(struct gx_m2 *m2);
bool gx_m2_ask_unload(struct gx_m2 *m2);
int gx_m2_load(struct gx_m2 *m2, struct wow_m2_file *file);
bool gx_m2_post_load(struct gx_m2 *m2);
int gx_m2_initialize(struct gx_m2 *m2);
void gx_m2_render(struct gx_m2 *m2, bool transparent);
#ifdef WITH_DEBUG_RENDERING
void gx_m2_render_aabb(struct gx_m2 *m2);
void gx_m2_render_bones_points(struct gx_m2 *m2);
void gx_m2_render_bones_lines(struct gx_m2 *m2);
void gx_m2_render_lights(struct gx_m2 *m2);
void gx_m2_render_collisions(struct gx_m2 *m2, bool triangles);
#endif
void gx_m2_add_to_render(struct gx_m2 *m2);
void gx_m2_clear_update(struct gx_m2 *m2);

struct gx_m2_light
{
	struct vec3f position;
	struct vec3f color;
	struct vec2f attenuation;
};

struct m2_local_lighting
{
	struct vec4f light_direction;
	struct vec4f ambient_color;
	struct vec4f diffuse_color;
	struct gx_m2_light lights[4];
	uint32_t lights_count;
	gfx_buffer_t uniform_buffer;
};

struct gx_m2_instance_frame
{
	struct jks_array bone_mats; /* struct mat4f */
	struct mat4f mvp;
	struct mat4f mv;
	float distance_to_camera;
	bool culled;
};

struct gx_m2_instance
{
	struct gx_m2 *parent;
	struct gx_m2_instance_frame render_frames[RENDER_FRAMES_COUNT];
	struct m2_local_lighting *local_lighting;
	struct gx_m2_particles *gx_particles;
	struct gx_m2_ribbons *gx_ribbons;
	struct jks_array lights; /* gx_m2_light_t */
	struct jks_array enabled_batches; /* uint16_t */
	struct jks_array bone_calc; /* bitmask as uint8_t */
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
#ifdef WITH_DEBUG_RENDERING
	struct gx_aabb gx_caabb;
	struct gx_aabb gx_aabb;
#endif
	struct blp_texture *monster_textures[3];
	struct blp_texture *skin_extra_texture;
	struct blp_texture *object_texture;
	struct blp_texture *skin_texture;
	struct blp_texture *hair_texture;
	struct aabb caabb;
	struct aabb aabb;
	struct mat4f m_inv;
	struct mat4f m;
	struct vec3f pos;
	struct wow_m2_sequence *prev_sequence;
	struct wow_m2_sequence *sequence;
	uint64_t prev_sequence_started;
	uint32_t prev_sequence_time;
	uint32_t prev_sequence_id;
	uint64_t sequence_started;
	uint32_t sequence_time;
	uint32_t sequence_id;
	uint32_t camera;
	float render_distance_max;
	float sequence_speed;
	float scale;
	bool update_calculated;
	bool in_render_list;
	bool enable_lights;
	bool bones_calculated;
	bool bypass_batches;
};

struct gx_m2_instance *gx_m2_instance_new(struct gx_m2 *parent);
struct gx_m2_instance *gx_m2_instance_new_filename(const char *filename);
void gx_m2_instance_delete(struct gx_m2_instance *instance);
void gx_m2_instance_gc(struct gx_m2_instance *instance);
void gx_m2_instance_calc_remaining_bones(struct gx_m2_instance *instance);
void gx_m2_instance_calc_bones(struct gx_m2_instance *instance);
void gx_m2_instance_clear_update(struct gx_m2_instance *instance);
void gx_m2_instance_force_update(struct gx_m2_instance *instance, struct gx_m2_render_params *params);
void gx_m2_instance_update(struct gx_m2_instance *instance, bool bypass_frustum);
void gx_m2_instance_render(struct gx_m2_instance *instance, bool transparent, struct gx_m2_render_params *params);
void gx_m2_instance_render_particles(struct gx_m2_instance *instance);
void gx_m2_instance_render_ribbons(struct gx_m2_instance *instance);
void gx_m2_instance_calculate_distance_to_camera(struct gx_m2_instance *instance);
void gx_m2_instance_add_to_render(struct gx_m2_instance *instance, bool bypass_frustum);
void gx_m2_instance_update_aabb(struct gx_m2_instance *instance);
void gx_m2_instance_on_parent_loaded(struct gx_m2_instance *instance);
void gx_m2_instance_set_skin_extra_texture(struct gx_m2_instance *instance, struct blp_texture *texture);
void gx_m2_instance_set_skin_texture(struct gx_m2_instance *instance, struct blp_texture *texture);
void gx_m2_instance_set_hair_texture(struct gx_m2_instance *instance, struct blp_texture *texture);
void gx_m2_instance_set_monster_texture(struct gx_m2_instance *instance, int idx, struct blp_texture *texture);
void gx_m2_instance_set_object_texture(struct gx_m2_instance *instance, struct blp_texture *texture);
void gx_m2_instance_set_sequence(struct gx_m2_instance *instance, uint32_t sequence);
void gx_m2_instance_enable_batch(struct gx_m2_instance *instance, uint16_t batch);
void gx_m2_instance_enable_batches(struct gx_m2_instance *instance, uint16_t start, uint16_t end);
void gx_m2_instance_disable_batch(struct gx_m2_instance *instance, uint16_t batch);
void gx_m2_instance_disable_batches(struct gx_m2_instance *instance, uint16_t start, uint16_t end);
void gx_m2_instance_clear_batches(struct gx_m2_instance *instance);
void gx_m2_instance_set_mat(struct gx_m2_instance *instance, struct mat4f *mat);

bool m2_get_track_value_vec4f(struct gx_m2 *m2, struct wow_m2_track *track, struct vec4f *val, struct wow_m2_sequence *sequence, uint32_t t);
bool m2_get_track_value_vec3f(struct gx_m2 *m2, struct wow_m2_track *track, struct vec3f *val, struct wow_m2_sequence *sequence, uint32_t t);
bool m2_get_track_value_float(struct gx_m2 *m2, struct wow_m2_track *track, float *val, struct wow_m2_sequence *sequence, uint32_t t);
bool m2_get_track_value_uint8(struct gx_m2 *m2, struct wow_m2_track *track, uint8_t *val, struct wow_m2_sequence *sequence, uint32_t t);
bool m2_get_track_value_int16(struct gx_m2 *m2, struct wow_m2_track *track, int16_t *val, struct wow_m2_sequence *sequence, uint32_t t);
bool m2_get_track_value_quat16(struct gx_m2 *m2, struct wow_m2_track *track, struct vec4f *val, struct wow_m2_sequence *sequence, uint32_t t);

bool m2_instance_get_track_value_vec4f(struct gx_m2_instance *instance, struct wow_m2_track *track, struct vec4f *val);
bool m2_instance_get_track_value_vec3f(struct gx_m2_instance *instance, struct wow_m2_track *track, struct vec3f *val);
bool m2_instance_get_track_value_float(struct gx_m2_instance *instance, struct wow_m2_track *track, float *val);
bool m2_instance_get_track_value_uint8(struct gx_m2_instance *instance, struct wow_m2_track *track, uint8_t *val);
bool m2_instance_get_track_value_int16(struct gx_m2_instance *instance, struct wow_m2_track *track, int16_t *val);
bool m2_instance_get_track_value_quat16(struct gx_m2_instance *instance, struct wow_m2_track *track, struct vec4f *val);

#endif
