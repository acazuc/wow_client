#version 450

layout(location=0) in vec2 vs_position;
layout(location=1) in vec2 vs_uv;

layout(location = 0) out fs_block
{
	vec2 fs_uv;
};

layout(set = 0, binding = 1, std140) uniform model_block
{
	mat4 mvp;
	int horizontal;
};

void main()
{
	fs_uv = vs_uv;
	gl_Position = mvp * vec4(vs_position, 0, 1);
}
