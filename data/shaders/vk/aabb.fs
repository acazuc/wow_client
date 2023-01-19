#version 330

layout(std140) uniform mesh_block
{
	mat4 mvp;
	vec4 color;
};

layout(location=0) out vec4 fragcolor;

void main()
{
	fragcolor = color;
}
