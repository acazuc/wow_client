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

static float weight = .75;

Texture2D color_tex : register(t0);
SamplerState color_sampler : register(s0);
Texture2D normal_tex : register(t1);
SamplerState normal_sampler : register(s1);
Texture2D position_tex : register(t2);
SamplerState position_sampler : register(s2);
Texture2D ssao_tex : register(t3);
SamplerState ssao_sampler : register(s3);

pixel_output main(pixel_input input)
{
	pixel_output output;
	output.normal = normal_tex.Sample(normal_sampler, input.uv);
	output.position = position_tex.Sample(position_sampler, input.uv);
#ifdef LINEAR_SAMPLING
	float result = 0;
	result += ssao_tex.Sample(ssao_sampler, input.uv, int2(-2, -2)).r;
	result += ssao_tex.Sample(ssao_sampler, input.uv, int2(-2, -1)).r;
	result += ssao_tex.Sample(ssao_sampler, input.uv, int2(-2,  0)).r;
	result += ssao_tex.Sample(ssao_sampler, input.uv, int2(-2,  1)).r;
	result += ssao_tex.Sample(ssao_sampler, input.uv, int2(-1, -2)).r;
	result += ssao_tex.Sample(ssao_sampler, input.uv, int2(-1, -1)).r;
	result += ssao_tex.Sample(ssao_sampler, input.uv, int2(-1,  0)).r;
	result += ssao_tex.Sample(ssao_sampler, input.uv, int2(-1,  1)).r;
	result += ssao_tex.Sample(ssao_sampler, input.uv, int2( 0, -2)).r;
	result += ssao_tex.Sample(ssao_sampler, input.uv, int2( 0, -1)).r;
	result += ssao_tex.Sample(ssao_sampler, input.uv, int2( 0,  0)).r;
	result += ssao_tex.Sample(ssao_sampler, input.uv, int2( 0,  1)).r;
	result += ssao_tex.Sample(ssao_sampler, input.uv, int2( 1, -2)).r;
	result += ssao_tex.Sample(ssao_sampler, input.uv, int2( 1, -1)).r;
	result += ssao_tex.Sample(ssao_sampler, input.uv, int2( 1,  0)).r;
	result += ssao_tex.Sample(ssao_sampler, input.uv, int2( 1,  1)).r;
	result *= 0.0625;
#else
	float result = ssao_tex.Sample(ssao_sampler, input.uv).r;
#endif
	output.color = float4(color_tex.Sample(color_sampler, input.uv).rgb * lerp(1, result, weight), 1);
	return output;
}
