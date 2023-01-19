#version 330

layout(location=0) in vec2 vs_position;
layout(location=1) in vec4 vs_color;
layout(location=2) in vec2 vs_uv;

out fs_block
{
	vec4 fs_color;
	vec2 fs_uv;
};

layout(std140) uniform model_block
{
	mat4 mvp;
};

void main()
{
	fs_uv = vs_uv;
	fs_color = vs_color;
	gl_Position = mvp * vec4(vs_position, 0, 1);
}
