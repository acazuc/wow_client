#version 450

layout(location = 0) in fs_block
{
	vec3 fs_position;
	vec3 fs_light_dir;
	vec3 fs_diffuse;
	vec3 fs_normal;
	float fs_alpha;
	vec2 fs_uv;
};

layout(set = 1, binding = 0) uniform sampler2D tex;

layout(set = 0, binding = 1, std140) uniform model_block
{
	mat4 p;
	mat4 v;
	mat4 mv;
	mat4 mvp;
};

layout(set = 0, binding = 2, std140) uniform scene_block
{
	vec4 light_direction;
	vec4 diffuse_color;
	vec4 specular_color;
	vec4 base_color;
	vec4 final_color;
	vec4 fog_color;
	vec2 fog_range;
	vec2 alphas;
};

layout(location=0) out vec4 fragcolor;
layout(location=1) out vec4 fragnormal;
layout(location=2) out vec4 fragposition;

void main()
{
	vec3 tex_col = texture(tex, fs_uv).rgb;
	float specular_factor = pow(clamp(dot(normalize(-fs_position), normalize(reflect(-fs_light_dir, fs_normal))), 0, 1), 10) * 20;
	vec3 color = fs_diffuse + tex_col * (1 + specular_color.xyz * specular_factor);
	float fog_factor = clamp((length(fs_position) - fog_range.x) / (fog_range.y - fog_range.x), 0, 1);
	fragcolor = vec4(mix(color, fog_color.xyz, fog_factor), fs_alpha);
	fragnormal = vec4(0, 1, 0, 0);
	fragposition = vec4(fs_position, 1);
}
