#version 450

layout(location=0) in vec3 vs_position;
layout(location=1) in float vs_color0;
layout(location=2) in float vs_color1;
layout(location=3) in float vs_color2;
layout(location=4) in float vs_color3;
layout(location=5) in float vs_color4;
layout(location=6) in vec2 vs_uv;

layout(location = 0) out fs_block
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

void main()
{
	gl_Position = mvp * vec4(vs_position, 1);
	vec4 ret = sky_colors[0];
	ret = mix(ret, sky_colors[1], vs_color0);
	ret = mix(ret, sky_colors[2], vs_color1);
	ret = mix(ret, sky_colors[3], vs_color2);
	ret = mix(ret, sky_colors[4], vs_color3);
	ret = mix(ret, sky_colors[5], vs_color4);
	fs_color = ret;
	fs_uv = vs_uv;
}
