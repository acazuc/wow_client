#version 450

layout(location = 0) in fs_block
{
	vec3 fs_position;
	vec3 fs_light_dir;
	vec3 fs_diffuse;
	vec3 fs_normal;
	vec2 fs_uv;
};

layout(set = 1, binding = 1) uniform sampler2D tex;

layout(set = 0, binding = 0, std140) uniform mesh_block
{
	float alpha;
	int type;
};

layout(set = 0, binding = 1, std140) uniform model_block
{
	mat4 mvp;
	mat4 mv;
	mat4 v;
};

layout(set = 0, binding = 2, std140) uniform scene_block
{
	vec4 light_direction;
	vec4 specular_color;
	vec4 diffuse_color;
	vec4 final_color;
	vec4 base_color;
	vec4 fog_color;
	vec2 fog_range;
};

layout(location=0) out vec4 fragcolor;
layout(location=1) out vec4 fragnormal;
layout(location=2) out vec4 fragposition;

void main()
{
	vec3 color;
	vec3 tex_col = texture(tex, fs_uv).rgb;
	if (type <= 1 || type == 4 || type == 8)
	{
		float specular_factor = pow(clamp(dot(normalize(-fs_position), normalize(reflect(-fs_light_dir, fs_normal))), 0, 1), 10) * 20;
		color = fs_diffuse.xyz + tex_col * (1 + specular_color.xyz * specular_factor);
	}
	else
	{
		color = tex_col;
	}
	float fog_factor = clamp((length(fs_position) - fog_range.x) / (fog_range.y - fog_range.x), 0, 1);
	fragcolor = vec4(mix(color, fog_color.xyz, fog_factor), alpha);
	fragnormal = vec4(fs_normal, 1);
	fragposition = vec4(fs_position, 1);
}
