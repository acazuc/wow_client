#version 450

layout(location = 0) in fs_block
{
	vec2 fs_uv;
};

layout(location=0) out vec4 fragcolor;
layout(location=1) out vec4 fragnormal;
layout(location=2) out vec4 fragposition;

layout(set = 0, binding = 1, std140) uniform model_block
{
	mat4 mvp;
	float power;
};

layout(set = 1, binding = 0) uniform sampler2D color_tex;

void main()
{
	float other = power * -1;
	float self = power * 4 + 1;
	vec3 color = texture(color_tex, fs_uv).rgb * self;
	color += textureOffset(color_tex, fs_uv, ivec2( 1,  0)).rgb * other;
	color += textureOffset(color_tex, fs_uv, ivec2(-1,  0)).rgb * other;
	color += textureOffset(color_tex, fs_uv, ivec2( 0,  1)).rgb * other;
	color += textureOffset(color_tex, fs_uv, ivec2( 0, -1)).rgb * other;
	fragcolor = vec4(color, 1);
}
