#version 450

layout(location=0) in vec3 vs_position;
layout(location=1) in vec4 vs_color;
layout(location=2) in int vs_bone;

layout(location = 0) out fs_block
{
	vec4 fs_color;
};

struct Light
{
	vec4 ambient;
	vec4 diffuse;
	vec4 position;
	vec2 attenuation;
	vec2 data;
};

layout(set = 0, binding = 1, std140) uniform model_block /* same as m2::model_block */
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
	vec4 boned = bone_mats[vs_bone] * vec4(vs_position, 1);
	gl_Position = mvp * boned;
	fs_color = vs_color;
}
