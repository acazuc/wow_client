#version 330

layout(location=0) in vec3 vs_position;

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

void main()
{
	gl_Position = mvp * vec4(vs_position, 1);
}
