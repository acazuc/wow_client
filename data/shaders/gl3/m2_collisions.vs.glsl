#version 330

layout(location=0) in vec3 vs_position;

struct Light
{
	vec4 ambient;
	vec4 diffuse;
	vec4 position;
	vec2 attenuation;
	vec2 data;
};

layout(std140) uniform model_block /* same as m2::model_block */
{
	mat4 v;
	mat4 mv;
	mat4 mvp;
	ivec4 lights_count;
	Light lights[4];
	mat4 bone_mats[256];
};

void main()
{
	vec4 fixed_position = vec4(vs_position, 1);
	gl_Position = mvp * fixed_position;
}
