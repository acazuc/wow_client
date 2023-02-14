#version 450

layout(location=0) in vec3 vs_position;

layout(set = 0, binding = 0, std140) uniform mesh_block
{
	mat4 mvp;
	vec4 color;
};

void main()
{
	gl_Position = mvp * vec4(vs_position, 1);
}
