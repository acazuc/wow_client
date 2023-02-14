#version 450

layout(location=0) in vec4 vs_position;
layout(location=1) in vec4 vs_color;
layout(location=2) in vec2 vs_uv;

layout(location = 0) out fs_block
{
	vec3 fs_position;
	vec4 fs_color;
	vec2 fs_uv;
};

layout(set = 0, binding = 1, std140) uniform model_block
{
	mat4 mvp;
	mat4 mv;
	vec4 fog_color;
	float alpha_test;
};

layout(set = 0, binding = 2, std140) uniform scene_block
{
	vec2 fog_range;
};

void main()
{
	fs_position = (mv * vs_position).xyz;
	gl_Position = mvp * vs_position;
	fs_color = vs_color;
	fs_uv = vs_uv;
}
