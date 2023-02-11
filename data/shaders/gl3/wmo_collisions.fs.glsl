#version 330

layout(std140) uniform model_block /* same as wmo::model_block */
{
	mat4 v;
	mat4 mv;
	mat4 mvp;
};

layout(std140) uniform mesh_block
{
	vec4 color;
};

layout(location=0) out vec4 fragcolor;

void main()
{
	fragcolor = color;
}
