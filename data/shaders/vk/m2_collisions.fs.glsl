#version 450

layout(location=0) out vec4 fragcolor;

layout(set = 0, binding = 0, std140) uniform mesh_block
{
	vec4 color;
};

void main()
{
	fragcolor = color;
	//vec4(.5, .5, 1, .3);
}
