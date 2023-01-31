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
	vec4 clouds_color;
	float clouds_blend;
	float clouds_factor;
};

layout(location=0) out vec4 fragcolor;

uniform sampler2D clouds1;
uniform sampler2D clouds2;

void main()
{
	float clouds_alpha = (texture(clouds1, fs_uv).r * (1 - clouds_blend) + texture(clouds2, fs_uv).r * clouds_blend);
#if 0
	fragcolor = vec4(fs_uv.r, 0, 0, 1);
#else
	fragcolor = mix(fs_color, clouds_color, clouds_alpha * clouds_factor);
#endif
}
