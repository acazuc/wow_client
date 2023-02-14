#version 450

layout(set = 0, binding = 0, std140) uniform mesh_block
{
	mat4 mvp;
	vec4 color;
};

layout(location=0) out vec4 fragcolor;

void main()
{
	fragcolor = color;
}
