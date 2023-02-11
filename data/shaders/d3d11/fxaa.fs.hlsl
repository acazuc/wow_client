struct pixel_input
{
	float4 out_position : SV_POSITION;
	float2 uv : FS_INPUT0;
};

struct pixel_output
{
	float4 color : SV_TARGET0;
	float4 normal : SV_TARGET1;
	float4 position : SV_TARGET2;
};

Texture2D color_tex : register(t0);
SamplerState color_sampler : register(s0);
Texture2D normal_tex : register(t1);
SamplerState normal_sampler : register(s1);
Texture2D position_tex : register(t2);
SamplerState position_sampler : register(s2);

#define FXAA_EDGE_THRESHOLD (1. / 8)
#define FXAA_THRESHOLD_MIN (1. / 16)
#define FXAA_THRESHOLD_MAX (3.)

float luma(float3 color)
{
	return dot(color, float3(.299, .587, .114));
	//return color.g * (0.587 / 0.299) + color.r;
}

pixel_output main(pixel_input input)
{
	pixel_output output;
	float4 NW;
	float4 NE;
	float4 SW;
	float4 SE;
	float4 M;
	NW.rgb = color_tex.Sample(color_sampler, input.uv, int2(-1, -1)).rgb;
	NE.rgb = color_tex.Sample(color_sampler, input.uv, int2( 1, -1)).rgb;
	SW.rgb = color_tex.Sample(color_sampler, input.uv, int2(-1,  1)).rgb;
	SE.rgb = color_tex.Sample(color_sampler, input.uv, int2( 1,  1)).rgb;
	M.rgb = color_tex.Sample(color_sampler, input.uv).rgb;
	NW.a = luma(NW.rgb);
	NE.a = luma(NE.rgb);
	SW.a = luma(SW.rgb);
	SE.a = luma(SE.rgb);
	M.a = luma(M.rgb);
	float luma_min = min(M.a, min(min(NW.a, NE.a), min(SW.a, SE.a)));
	float luma_max = max(M.a, max(max(NW.a, NE.a), max(SW.a, SE.a)));
	float2 dir = float2(-(NW.a + NE.a) + (SW.a + SE.a), (NW.a + SW.a) - (NE.a + SE.a));
	float reduce = max((NW.a + NE.a + SW.a + SE.a) * (.25 * FXAA_EDGE_THRESHOLD), FXAA_THRESHOLD_MIN);
	float rcp_min = 1. / (min(abs(dir.x), abs(dir.y)) + reduce);
	dir = clamp(dir * rcp_min, -FXAA_THRESHOLD_MAX, FXAA_THRESHOLD_MAX);
	int2 offset1 = int2(dir * (1. / 3 - .5));
	int2 offset2 = int2(dir * (2. / 3 - .5));
	float3 a1 = color_tex.Sample(color_sampler, input.uv, offset1).rgb;
	float3 a2 = color_tex.Sample(color_sampler, input.uv, offset2).rgb;
	float3 rgb1 = (a1 + a2) * .5;
	float3 rgb2 = rgb1 * .5;
	int2 offset3 = int2(dir * -.5);
	int2 offset4 = int2(dir *  .5);
	float3 b1 = float3(0, 0, 0);//color_tex.Sample(color_sampler, input.uv, clamp(offset3, int2(-1, -1), int2(1, 1))).rgb;
	float3 b2 = float3(0, 0, 0);//color_tex.Sample(color_sampler, input.uv, clamp(offset4, int2(-1, -1), int2(1, 1))).rgb;
	rgb2 += (b1 + b2) * .25;
	float luma2 = luma(rgb2);
	if (luma2 < luma_min || luma2 > luma_max)
		output.color = float4(rgb1, 1);
	else
		output.color = float4(rgb2, 1);
	output.normal = normal_tex.Sample(normal_sampler, input.uv);
	output.position = position_tex.Sample(position_sampler, input.uv);
	return output;
}
