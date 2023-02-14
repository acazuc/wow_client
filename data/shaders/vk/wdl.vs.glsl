#version 450

layout(location=0) in vec3 vs_position;

layout(location = 0) out fs_block
{
	vec3 fs_position;
};

layout(set = 0, binding = 1, std140) uniform model_block
{
	mat4 mvp;
	mat4 mv;
	vec4 color;
};

void main()
{
	vec4 position_fixed = vec4(vs_position, 1);
	fs_position = (mv * position_fixed).xyz;
	gl_Position = mvp * position_fixed;
}
