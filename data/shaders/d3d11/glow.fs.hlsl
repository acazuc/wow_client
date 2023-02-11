cbuffer model_block : register(b1)
{
	float4x4 mvp;
	float factor;
};

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

pixel_output main(pixel_input input)
{
	pixel_output output;
	float3 texture_color = color_tex.Sample(color_sampler, input.uv).rgb;
	float3 tmp = texture_color * texture_color;
	output.color = float4(tmp * factor + texture_color, 1);
	output.normal = normal_tex.Sample(normal_sampler, input.uv);
	output.position = position_tex.Sample(position_sampler, input.uv);
	return output;
}
