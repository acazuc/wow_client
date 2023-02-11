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
uniform sampler2D ssao_tex;

const float weight = .5;

#define LINEAR_SAMPLING

void main()
{
	fragnormal = texture(normal_tex, fs_uv);
	fragposition = texture(position_tex, fs_uv);
	//fragcolor = texture(ssao_tex, fs_uv);
	//return;
#ifdef LINEAR_SAMPLING
	float result = 0;
	result += textureOffset(ssao_tex, fs_uv, ivec2(-2, -2)).r;
	result += textureOffset(ssao_tex, fs_uv, ivec2(-2, -1)).r;
	result += textureOffset(ssao_tex, fs_uv, ivec2(-2,  0)).r;
	result += textureOffset(ssao_tex, fs_uv, ivec2(-2,  1)).r;
	result += textureOffset(ssao_tex, fs_uv, ivec2(-1, -2)).r;
	result += textureOffset(ssao_tex, fs_uv, ivec2(-1, -1)).r;
	result += textureOffset(ssao_tex, fs_uv, ivec2(-1,  0)).r;
	result += textureOffset(ssao_tex, fs_uv, ivec2(-1,  1)).r;
	result += textureOffset(ssao_tex, fs_uv, ivec2( 0, -2)).r;
	result += textureOffset(ssao_tex, fs_uv, ivec2( 0, -1)).r;
	result += textureOffset(ssao_tex, fs_uv, ivec2( 0,  0)).r;
	result += textureOffset(ssao_tex, fs_uv, ivec2( 0,  1)).r;
	result += textureOffset(ssao_tex, fs_uv, ivec2( 1, -2)).r;
	result += textureOffset(ssao_tex, fs_uv, ivec2( 1, -1)).r;
	result += textureOffset(ssao_tex, fs_uv, ivec2( 1,  0)).r;
	result += textureOffset(ssao_tex, fs_uv, ivec2( 1,  1)).r;
	result *= 0.0625;
#else
	float result = texture(ssao_tex, fs_uv).r;
#endif
	//fragcolor = vec4(vec3(result), 1);
	//result = 1 - pow(1 - result, 3);
	//fragcolor = vec4(vec3(mix(1, result, weight)), 1);
	fragcolor = vec4(texture(color_tex, fs_uv).rgb * mix(1, result, weight), 1);
	//fragcolor = vec4(texture(color_tex, fs_uv).rgb * clamp(1 - (1 - result) * weight, 0, 1), 1);
	//fragcolor = vec4(texture(color_tex, fs_uv).rgb * result, 1);
	//fragcolor = vec4(abs(texture(normal_tex, fs_uv).rgb), 1);
}
