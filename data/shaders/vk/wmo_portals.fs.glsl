#version 450

layout(location = 0) in fs_block
{
	vec4 fs_color;
};

layout(set = 0, binding = 0, std140) uniform mesh_block
{
	vec4 color;
};

layout(location=0) out vec4 fragcolor;

void main()
{
	fragcolor = fs_color * color;
}
