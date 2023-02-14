#version 450

layout(set = 0, binding = 1, std140) uniform model_block /* same as wmo::model_block */
{
	mat4 v;
	mat4 mv;
	mat4 mvp;
};

layout(set = 0, binding = 0, std140) uniform mesh_block
{
	vec4 color;
};

layout(location=0) out vec4 fragcolor;

void main()
{
	fragcolor = color;
}
