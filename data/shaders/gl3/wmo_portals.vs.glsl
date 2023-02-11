#version 330

layout(location=0) in vec3 vs_position;
layout(location=1) in vec4 vs_color;

out fs_block
{
	vec4 fs_color;
};

layout(std140) uniform model_block
{
	mat4 mvp;
};

void main()
{
	gl_Position = mvp * vec4(vs_position, 1);
	fs_color = vs_color;
}
