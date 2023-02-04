#ifndef SHADERS_H
#define SHADERS_H

#include <jks/mat4.h>
#include <jks/vec4.h>
#include <jks/vec3.h>
#include <jks/vec2.h>

#include <gfx/objects.h>

struct shader_wmo_collisions_mesh_block
{
	struct vec4f color;
};

struct shader_aabb_mesh_block
{
	struct mat4f mvp;
	struct vec4f color;
};

struct shader_basic_model_block
{
	struct mat4f mvp;
};

struct shader_wmo_portals_model_block
{
	struct mat4f mvp;
};

struct shader_wmo_portals_mesh_block
{
	struct vec4f color;
};

struct shader_bloom_blur_model_block
{
	struct mat4f mvp;
	int32_t horizontal;
};

struct shader_bloom_merge_model_block
{
	struct mat4f mvp;
	float factor;
};

struct shader_bloom_model_block
{
	struct mat4f mvp;
	float threshold;
};

struct shader_cel_model_block
{
	struct mat4f mvp;
	float cel;
};

struct shader_fxaa_model_block
{
	struct mat4f mvp;
	struct vec2f screen_size;
};

struct shader_fsaa_model_block
{
	struct mat4f mvp;
};

struct shader_glow_model_block
{
	struct mat4f mvp;
	float factor;
};

struct shader_gui_model_block
{
	struct mat4f mvp;
};

struct shader_m2_light_block
{
	struct vec4f ambient;
	struct vec4f diffuse;
	struct vec4f position;
	struct vec2f attenuation;
	struct vec2f data;
};

struct shader_m2_mesh_block
{
	struct mat4f tex1_matrix;
	struct mat4f tex2_matrix;
	struct vec4i combiners;
	struct vec4i settings;
	struct vec4f color;
	struct vec3f fog_color;
	float alpha_test;
};

struct shader_m2_model_block
{
	struct mat4f v;
	struct mat4f mv;
	struct mat4f mvp;
	struct vec4i light_count;
	struct shader_m2_light_block lights[4];
	struct mat4f bone_mats[256]; /* must be at the end for offsetof() */
};

struct shader_m2_scene_block
{
	struct vec4f light_direction;
	struct vec4f specular_color;
	struct vec4f diffuse_color;
	struct vec4f ambient_color;
	struct vec2f fog_range;
};

struct shader_mclq_magma_model_block
{
	struct mat4f v;
	struct mat4f mv;
	struct mat4f mvp;
};

struct shader_mclq_magma_scene_block
{
	struct vec4f diffuse_color;
	struct vec4f fog_color;
	struct vec2f fog_range;
};

struct shader_mclq_water_model_block
{
	struct mat4f p;
	struct mat4f v;
	struct mat4f mv;
	struct mat4f mvp;
};

struct shader_mclq_water_scene_block
{
	struct vec4f light_direction;
	struct vec4f diffuse_color;
	struct vec4f specular_color;
	struct vec4f base_color;
	struct vec4f final_color;
	struct vec4f fog_color;
	struct vec2f fog_range;
	struct vec2f alphas;
};

struct shader_mcnk_scene_block
{
	struct vec4f light_direction;
	struct vec4f ambient_color;
	struct vec4f diffuse_color;
	struct vec4f specular_color;
	struct vec4f fog_color;
	struct vec4f fog_range;
};

struct shader_mcnk_model_block
{
	struct mat4f v;
	struct mat4f mv;
	struct mat4f mvp;
	float offset_time;
};

struct shader_mcnk_mesh_block
{
	struct vec2f uv_offsets[4];
};

struct shader_mliq_mesh_block
{
	float alpha;
	int32_t type;
};

struct shader_mliq_model_block
{
	struct mat4f mvp;
	struct mat4f mv;
	struct mat4f v;
};

struct shader_mliq_scene_block
{
	struct vec4f light_direction;
	struct vec4f specular_color;
	struct vec4f diffuse_color;
	struct vec4f final_color;
	struct vec4f base_color;
	struct vec4f fog_color;
	struct vec2f fog_range;
};

struct shader_skybox_model_block
{
	struct mat4f mvp;
	struct vec4f sky_colors[6];
	struct vec4f clouds_colors[2];
	struct vec2f clouds_factors;
	float clouds_blend;
	float clouds_drift;
};

struct shader_sobel_model_block
{
	struct mat4f mvp;
};

struct shader_ssao_denoiser_model_block
{
	struct mat4f mvp;
};

struct shader_ssao_model_block
{
	struct mat4f mvp;
	struct mat4f p;
	struct vec2f window_size;
	float radius;
};

struct shader_ui_model_block
{
	struct mat4f mvp;
	struct vec4f color;
	struct vec4f uv_transform;
	float alpha_test;
	int32_t use_mask;
};

struct shader_wdl_model_block
{
	struct mat4f mvp;
	struct mat4f mv;
	struct vec4f color;
};

struct shader_wmo_mesh_block
{
	struct vec4f emissive_color;
	struct vec4i combiners;
	struct vec4i settings;
	float alpha_test;
};

struct shader_wmo_model_block
{
	struct mat4f v;
	struct mat4f mv;
	struct mat4f mvp;
};

struct shader_wmo_scene_block
{
	struct vec4f light_direction;
	struct vec4f specular_color;
	struct vec4f diffuse_color;
	struct vec4f ambient_color;
	struct vec4f fog_color;
	struct vec2f fog_range;
};

struct shader_chromaber_model_block
{
	struct mat4f mvp;
	struct vec2f screen_size;
	float size;
	float power;
};

struct shader_sharpen_model_block
{
	struct mat4f mvp;
	float power;
};

struct shader_particle_model_block
{
	struct mat4f mvp;
	struct mat4f mv;
	struct vec4f fog_color;
	float alpha_test;
};

struct shader_particle_scene_block
{
	struct vec2f fog_range;
};

struct shader_text_model_block
{
	struct mat4f mvp;
	struct vec4f color;
};

struct shader_m2_collisions_mesh_block
{
	struct vec4f color;
};

struct shader_m2_collisions_input
{
	struct vec3f position;
};

struct shader_wmo_portals_input
{
	struct vec3f position;
	struct vec4f color;
};

struct shader_m2_lights_input
{
	struct vec3f position;
	struct vec4f color;
	uint32_t bone;
};

struct shader_m2_bones_input
{
	struct vec3f position;
	struct vec4f color;
	uint32_t bone;
};

struct shader_particle_input
{
	struct vec4f position;
	struct vec4f color;
	struct vec2f uv;
};

struct shader_skybox_input
{
	struct vec3f position;
	float colors[5];
	struct vec2f uv;
};

struct shader_ribbon_input
{
	struct vec4f position;
	struct vec4f color;
	struct vec2f uv;
};

struct shader_basic_input
{
	struct vec3f position;
	struct vec4f color;
};

struct shader_mcnk_input
{
	struct vec4b norm;
	float y;
};

struct shader_mclq_input
{
	struct vec3f position;
	struct vec2f uv;
};

struct shader_text_input
{
	struct vec2f position;
	struct vec2f uv;
};

struct shader_mliq_input
{
	struct vec3f position;
	struct vec4f uv;
};

struct shader_gui_input
{
	struct vec2f position;
	struct vec4f color;
};

struct shader_wmo_input
{
	struct vec3f position;
	struct vec3f norm;
	struct vec2f uv;
};

struct shader_ui_input
{
	struct vec2f position;
	struct vec4f color;
	struct vec2f uv;
};

struct shaders
{
	gfx_shader_state_t wmo_collisions;
	gfx_shader_state_t ssao_denoiser;
	gfx_shader_state_t m2_collisions;
	gfx_shader_state_t wmo_portals;
	gfx_shader_state_t bloom_merge;
	gfx_shader_state_t bloom_blur;
	gfx_shader_state_t mclq_water;
	gfx_shader_state_t mclq_magma;
	gfx_shader_state_t chromaber;
	gfx_shader_state_t m2_lights;
	gfx_shader_state_t m2_bones;
	gfx_shader_state_t particle;
	gfx_shader_state_t sharpen;
	gfx_shader_state_t skybox;
	gfx_shader_state_t ribbon;
	gfx_shader_state_t basic;
	gfx_shader_state_t sobel;
	gfx_shader_state_t bloom;
	gfx_shader_state_t glow;
	gfx_shader_state_t mliq;
	gfx_shader_state_t ssao;
	gfx_shader_state_t fxaa;
	gfx_shader_state_t aabb;
	gfx_shader_state_t mcnk;
	gfx_shader_state_t text;
	gfx_shader_state_t fsaa;
	gfx_shader_state_t gui;
	gfx_shader_state_t wdl;
	gfx_shader_state_t wmo;
	gfx_shader_state_t cel;
	gfx_shader_state_t m2;
	gfx_shader_state_t ui;
};

bool shaders_build(struct shaders *shaders);
void shaders_clean(struct shaders *shaders);

#endif
