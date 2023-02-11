cbuffer model_block : register(b1)
{
	float4x4 mvp;
	float4 sky_colors[6];
	float4 clouds_colors[2];
	float2 clouds_factors;
	float clouds_blend;
	float clouds_drift;
};

struct pixel_input
{
	float4 position : SV_POSITION;
	float4 color : PS_INPUT0;
	float2 uv : PS_INPUT1;
};

SamplerState clouds1_sampler : register(s0);
Texture2D clouds1 : register(t0);
SamplerState clouds2_sampler : register(s1);
Texture2D clouds2 : register(t1);

float4 main(pixel_input input) : SV_TARGET
{
	float clouds_alpha1 = clouds1.Sample(clouds1_sampler, input.uv + float2(clouds_drift, 0)).r;
	float clouds_alpha2 = clouds2.Sample(clouds2_sampler, input.uv + float2(clouds_drift, 0)).r;
	float clouds_alpha = lerp(clouds_alpha1, clouds_alpha2, clouds_blend);
	return lerp(input.color, clouds_colors[0], clamp((clouds_alpha - clouds_factors.x) / (clouds_factors.y - clouds_factors.x), 0, 1));
}
