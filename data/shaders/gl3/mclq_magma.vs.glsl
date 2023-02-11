#version 330

layout(location=0) in vec3 vs_position;
layout(location=1) in vec2 vs_uv;

out fs_block
{
	vec3 fs_position;
	vec3 fs_diffuse;
	vec3 fs_normal;
	vec2 fs_uv;
};

layout(std140) uniform model_block
{
	mat4 v;
	mat4 mv;
	mat4 mvp;
};

layout(std140) uniform scene_block
{
	vec4 diffuse_color;
	vec4 fog_color;
	vec2 fog_range;
};

void main()
{
	vec4 position_fixed = vec4(vs_position, 1);
	gl_Position =  mvp * position_fixed;
	fs_position = (mv * position_fixed).xyz;
	fs_normal = normalize((mv * vec4(0, 1, 0, 0)).xyz);
	fs_uv = vs_uv;
}
