#version 450

layout(location=0) in vec3 vs_position;
layout(location=1) in vec3 vs_normal;
layout(location=2) in vec2 vs_uv;
layout(location=3) in vec4 vs_color;

layout(location = 0) out fs_block
{
	vec3 fs_position;
	vec3 fs_light_dir;
	vec3 fs_normal;
	vec4 fs_color;
	vec3 fs_light;
	vec2 fs_uv1;
	vec2 fs_uv2;
};

layout(set = 0, binding = 0, std140) uniform mesh_block
{
	vec4 emissive_color;
	ivec4 combiners;
	ivec4 settings;
	float alpha_test;
};

layout(set = 0, binding = 1, std140) uniform model_block
{
	mat4 v;
	mat4 mv;
	mat4 mvp;
};

layout(set = 0, binding = 2, std140) uniform scene_block
{
	vec4 light_direction;
	vec4 specular_color;
	vec4 diffuse_color;
	vec4 ambient_color;
	vec4 fog_color;
	vec2 fog_range;
};

void main()
{
	vec4 position_fixed = vec4(vs_position.x, vs_position.y, vs_position.z, 1);
	vec4 normal_fixed = vec4(vs_normal.x, vs_normal.y, vs_normal.z, 0);
	gl_Position =  mvp * position_fixed;
	fs_position = (mv * position_fixed).xyz;
	switch (combiners.x)
	{
		case 0:
			fs_uv1 = vs_uv;
			break;
		case 1:
			break;
		case 2:
			break;
	}
	if (settings.x != 0)
		fs_color = vs_color.bgra;
	else
		fs_color = vec4(1);
	fs_normal = normalize((mv * normal_fixed).xyz);
	if (settings.y == 0)
	{
		fs_light_dir = normalize((v * -light_direction).xyz);
		float diffuse_factor = clamp(dot(fs_normal, fs_light_dir), 0, 1);
		fs_light = ambient_color.xyz + diffuse_color.xyz * diffuse_factor + emissive_color.xyz;
	}
	else
	{
		fs_light = vec3(1);
	}
}
