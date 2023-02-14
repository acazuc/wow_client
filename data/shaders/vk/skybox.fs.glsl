#version 450

layout(location = 0) in fs_block
{
	vec4 fs_color;
	vec2 fs_uv;
};

layout(set = 0, binding = 1, std140) uniform model_block
{
	mat4 mvp;
	vec4 sky_colors[6];
	vec4 clouds_colors[2];
	vec2 clouds_factors;
	float clouds_blend;
	float clouds_drift;
};

layout(location=0) out vec4 fragcolor;

layout(set = 1, binding = 0) uniform sampler2D clouds1;
layout(set = 1, binding = 1) uniform sampler2D clouds2;

void main()
{
	float clouds_alpha1 = texture(clouds1, fs_uv + vec2(clouds_drift, 0)).r;
	float clouds_alpha2 = texture(clouds2, fs_uv + vec2(clouds_drift, 0)).r;
	float clouds_alpha = mix(clouds_alpha1, clouds_alpha2, clouds_blend);
	fragcolor = mix(fs_color, clouds_colors[0], clamp((clouds_alpha - clouds_factors.x) / (clouds_factors.y - clouds_factors.x), 0, 1));
}
