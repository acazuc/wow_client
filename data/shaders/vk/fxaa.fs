#version 330

in fs_block
{
	vec2 fs_uv;
};

layout(location=0) out vec4 fragcolor;
layout(location=1) out vec4 fragnormal;
layout(location=2) out vec4 fragposition;

uniform sampler2D color_tex;
uniform sampler2D normal_tex;
uniform sampler2D position_tex;

#define FXAA_EDGE_THRESHOLD (1. / 8)
#define FXAA_THRESHOLD_MIN (1. / 16)
#define FXAA_THRESHOLD_MAX (3.)

float luma(vec3 color)
{
	return dot(color, vec3(.299, .587, .114));
	//return color.g * (0.587 / 0.299) + color.r;
}

void main()
{
	fragnormal = texture(normal_tex, fs_uv);
	fragposition = texture(position_tex, fs_uv);
	vec4 NW;
	vec4 NE;
	vec4 SW;
	vec4 SE;
	vec4 M;
	NW.rgb = textureOffset(color_tex, fs_uv, ivec2(-1, -1)).rgb;
	NE.rgb = textureOffset(color_tex, fs_uv, ivec2( 1, -1)).rgb;
	SW.rgb = textureOffset(color_tex, fs_uv, ivec2(-1,  1)).rgb;
	SE.rgb = textureOffset(color_tex, fs_uv, ivec2( 1,  1)).rgb;
	M.rgb = texture(color_tex, fs_uv).rgb;
	NW.a = luma(NW.rgb);
	NE.a = luma(NE.rgb);
	SW.a = luma(SW.rgb);
	SE.a = luma(SE.rgb);
	M.a = luma(M.rgb);
	float luma_min = min(M.a, min(min(NW.a, NE.a), min(SW.a, SE.a)));
	float luma_max = max(M.a, max(max(NW.a, NE.a), max(SW.a, SE.a)));
	vec2 dir = vec2(-(NW.a + NE.a) + (SW.a + SE.a), (NW.a + SW.a) - (NE.a + SE.a));
	float reduce = max((NW.a + NE.a + SW.a + SE.a) * (.25 * FXAA_EDGE_THRESHOLD), FXAA_THRESHOLD_MIN);
	float rcp_min = 1. / (min(abs(dir.x), abs(dir.y)) + reduce);
	dir = clamp(dir * rcp_min, -FXAA_THRESHOLD_MAX, FXAA_THRESHOLD_MAX);
	vec3 a1 = textureOffset(color_tex, fs_uv, ivec2(dir * (1. / 3 - .5))).rgb;
	vec3 a2 = textureOffset(color_tex, fs_uv, ivec2(dir * (2. / 3 - .5))).rgb;
	vec3 rgb1 = (a1 + a2) * .5;
	vec3 rgb2 = rgb1 * .5;
	vec3 b1 = textureOffset(color_tex, fs_uv, ivec2(dir * -.5)).rgb;
	vec3 b2 = textureOffset(color_tex, fs_uv, ivec2(dir *  .5)).rgb;
	rgb2 += (b1 + b2) * .25;
	float luma = luma(rgb2);
	if (luma < luma_min || luma > luma_max)
		fragcolor = vec4(rgb1, 1);
	else
		fragcolor = vec4(rgb2, 1);
}
