#version 330

in fs_block
{
	vec4 fs_color;
	vec2 fs_uv;
};

layout (std140) uniform model_block
{
	mat4 mvp;
	vec4 sky_colors[6];
	vec4 clouds_colors[2];
	vec2 clouds_factors;
	float clouds_blend;
	float clouds_drift;
};

layout(location=0) out vec4 fragcolor;

uniform sampler2D clouds1;
uniform sampler2D clouds2;

void main()
{
#if 0
	fragcolor = vec4(fs_uv.r, 0, 0, 1);
#else
	fragcolor = fs_color;
#if 1
	float clouds_alpha1 = texture(clouds1, fs_uv + vec2(clouds_drift, 0)).r;
	float clouds_alpha2 = texture(clouds2, fs_uv + vec2(clouds_drift, 0)).r;
	float clouds_alpha = mix(clouds_alpha1, clouds_alpha2, clouds_blend);
	fragcolor = mix(fragcolor, clouds_colors[0], clamp((clouds_alpha - clouds_factors.x) / (clouds_factors.y - clouds_factors.x), 0, 1));
#endif
#endif
}
