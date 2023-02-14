#version 450

layout(location=0) in vec3 vs_position;
layout(location=1) in vec2 vs_uv;

layout(location = 0) out fs_block
{
	vec4 fs_position;
	vec2 fs_uv;
};

layout(set = 0, binding = 1, std140) uniform model_block
{
	mat4 mvp;
	vec4 color;
};

void main()
{
	gl_Position = mvp * vec4(vs_position, 1);
	fs_position = gl_Position;
	fs_uv = vs_uv;
}
