#version 330

in fs_block
{
	vec2 fs_uv;
};

layout(location=0) out vec4 fragcolor;
layout(location=1) out vec4 fragnormal;
layout(location=2) out vec4 fragposition;

layout(std140) uniform model_block
{
	mat4 mvp;
	vec2 screen_size;
};

uniform sampler2D color_tex;
uniform sampler2D normal_tex;
uniform sampler2D position_tex;

#define FXAA_EDGE_THRESHOLD (1. / 4)
#define FXAA_THRESHOLD_MIN (1. / 128)
#define FXAA_THRESHOLD_MAX (10)

float luma(vec3 color)
{
	return dot(color, vec3(.299, .587, .114));
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
	dir = min(vec2(FXAA_THRESHOLD_MAX), max(vec2(-FXAA_THRESHOLD_MAX), dir * rcp_min)) / screen_size;
	vec3 a1 = texture(color_tex, fs_uv + dir * (1. / 3 - .5)).rgb;
	vec3 a2 = texture(color_tex, fs_uv + dir * (2. / 3 - .5)).rgb;
	vec3 rgb1 = (a1 + a2) * .5;
	vec3 rgb2 = rgb1 * .5;
	vec3 b1 = texture(color_tex, fs_uv + dir * -.5).rgb;
	vec3 b2 = texture(color_tex, fs_uv + dir *  .5).rgb;
	rgb2 += (b1 + b2) * .25;
	float luma = luma(rgb2);
	if (luma < luma_min || luma > luma_max)
		fragcolor = vec4(rgb1, 1);
	else
		fragcolor = vec4(rgb2, 1);
}
