#version 330

in fs_block
{
	vec4 fs_color;
};

layout(std140) uniform mesh_block
{
	vec4 color;
};

layout(location=0) out vec4 fragcolor;

void main()
{
	fragcolor = fs_color * color;
}
