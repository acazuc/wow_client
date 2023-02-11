#version 330

layout(location=0) out vec4 fragcolor;

layout(std140) uniform mesh_block
{
	vec4 color;
};

void main()
{
	fragcolor = color;
	//vec4(.5, .5, 1, .3);
}
