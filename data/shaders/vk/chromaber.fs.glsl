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
	vec2 screen_size;
	float size;
	float power;
};

layout(set = 1, binding = 0) uniform sampler2D color_tex;

#define PIXEL_SIZE (vec2(1. / 1920, 1. / 1080))

#define R_FACTOR 0
#define G_FACTOR -0.5
#define B_FACTOR -1

void main()
{
	vec2 factor = (fs_uv - .5) * 2;
	factor = sign(factor) * pow(factor, vec2(2));
	vec2 offset = size * factor / screen_size;
	vec3 color = texture(color_tex, fs_uv).rgb;
	color.r = color.r * (1 - power) + power * texture(color_tex, fs_uv + R_FACTOR * offset).r;
	color.g = color.g * (1 - power) + power * texture(color_tex, fs_uv + G_FACTOR * offset).g;
	color.b = color.b * (1 - power) + power * texture(color_tex, fs_uv + B_FACTOR * offset).b;
	fragcolor = vec4(color, 1);
}
