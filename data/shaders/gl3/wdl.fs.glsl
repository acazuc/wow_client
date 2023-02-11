#version 330

in fs_block
{
	vec3 fs_position;
};

layout(std140) uniform model_block
{
	mat4 mvp;
	mat4 mv;
	vec4 color;
};

layout(location=0) out vec4 fragcolor;
layout(location=1) out vec4 fragnormal;
layout(location=2) out vec4 fragposition;

void main()
{
	fragcolor = color;
	fragnormal = vec4(0, 1, 0, 0);
	fragposition = vec4(0);//vec4(fs_position, 1);
}
