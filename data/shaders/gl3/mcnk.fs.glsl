#version 330

in fs_block
{
	vec3 fs_position;
	vec3 fs_light_dir;
	vec3 fs_normal;
	vec2 fs_uv0;
	vec2 fs_uv1;
	vec2 fs_uv2;
	vec2 fs_uv3;
	vec2 fs_uv4;
	flat int fs_id;
};

uniform sampler2DArray alpha_map;
uniform sampler2D textures[8];

struct mesh_block_st
{
	vec2 uv_offset0;
	vec2 uv_offset1;
	vec2 uv_offset2;
	vec2 uv_offset3;
};

layout(std140) uniform mesh_block
{
	mesh_block_st mesh_blocks[256];
};

layout(std140) uniform model_block
{
	mat4 v;
	mat4 mv;
	mat4 mvp;
	float offset_time;
};

layout(std140) uniform scene_block
{
	vec4 light_direction;
	vec4 ambient_color;
	vec4 diffuse_color;
	vec4 specular_color;
	vec4 fog_color;
	vec2 fog_range;
};

layout(location=0) out vec4 fragcolor;
layout(location=1) out vec4 fragnormal;
layout(location=2) out vec4 fragposition;

void main()
{
	vec4 alpha = texture(alpha_map, vec3(fs_uv0, fs_id / 145));
	vec3 t1 = texture(textures[0], fs_uv1).rgb;
	vec3 t2 = texture(textures[2], fs_uv2).rgb;
	vec3 t3 = texture(textures[4], fs_uv3).rgb;
	vec3 t4 = texture(textures[6], fs_uv4).rgb;
	vec3 diff_mix = mix(mix(mix(t1, t2, alpha.r), t3, alpha.g), t4, alpha.b) * (alpha.a * .3 + .7);
	//diff_mix = alpha.rgb;
	vec4 s1 = texture(textures[1], fs_uv1);
	vec4 s2 = texture(textures[3], fs_uv2);
	vec4 s3 = texture(textures[5], fs_uv3);
	vec4 s4 = texture(textures[7], fs_uv4);
	vec4 spec_mix = mix(mix(mix(s1, s2, alpha.r), s3, alpha.g), s4, alpha.b);
	float specular_factor = pow(clamp(dot(normalize(-fs_position), normalize(reflect(-fs_light_dir, fs_normal))), 0, 1), 10);
	specular_factor = specular_factor * alpha.a;
	vec3 specular = clamp(specular_factor * specular_color.xyz, 0, 1);
	specular *= spec_mix.rgb * spec_mix.a;
	specular *= 2;
	vec3 diffuse = diffuse_color.xyz * clamp(dot(fs_normal, fs_light_dir), 0, 1);
	vec3 color = clamp(diffuse + ambient_color.xyz, 0, 1);
	float fog_factor = clamp((length(fs_position) - fog_range.x) / (fog_range.y - fog_range.x), 0, 1);
	fragcolor = vec4(mix(diff_mix * color + specular, fog_color.xyz, fog_factor), 1);
	fragnormal = vec4(fs_normal, 1);
	fragposition = vec4(fs_position, 1);
}
