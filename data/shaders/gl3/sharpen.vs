#version 330

layout(location=0) in vec2 vs_position;
layout(location=1) in vec2 vs_uv;

out fs_block
{
	vec2 fs_uv;
};

layout(std140) uniform model_block
{
	mat4 mvp;
	float power;
};

void main()
{
	fs_uv = vs_uv;
	gl_Position = mvp * vec4(vs_position, 0, 1);
}
