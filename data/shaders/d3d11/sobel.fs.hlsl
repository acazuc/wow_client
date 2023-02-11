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

float3x3 sx = float3x3
(
	 1.0,  2.0,  1.0,
	 0.0,  0.0,  0.0,
	-1.0, -2.0, -1.0
);
float3x3 sy = float3x3
(
	1.0, 0.0, -1.0,
	2.0, 0.0, -2.0,
	1.0, 0.0, -1.0
);

float sobelPosition(float2 uv)
{
	return 0;
}

float sobelNormal(float2 uv)
{
	float4 normal = normal_tex.Sample(normal_sampler, uv);
	float3 samples[3];
	samples[0] = float3(
			dot(normal.rgb, normal_tex.Sample(normal_sampler, uv, int2(-1, -1)).rgb),
			dot(normal.rgb, normal_tex.Sample(normal_sampler, uv, int2(-1,  0)).rgb),
			dot(normal.rgb, normal_tex.Sample(normal_sampler, uv, int2(-1,  1)).rgb));
	samples[1] = float3(
			dot(normal.rgb, normal_tex.Sample(normal_sampler, uv, int2( 0, -1)).rgb),
			0,
			dot(normal.rgb, normal_tex.Sample(normal_sampler, uv, int2( 0,  1)).rgb));
	samples[2] = float3(
			dot(normal.rgb, normal_tex.Sample(normal_sampler, uv, int2( 1, -1)).rgb),
			dot(normal.rgb, normal_tex.Sample(normal_sampler, uv, int2( 1,  0)).rgb),
			dot(normal.rgb, normal_tex.Sample(normal_sampler, uv, int2( 1,  1)).rgb));
	float gx = dot(sx[0], samples[0]) + dot(sx[1], samples[1]) + dot(sx[2], samples[2]);
	float gy = dot(sy[0], samples[0]) + dot(sy[1], samples[1]) + dot(sy[2], samples[2]);
	return clamp(sqrt(gx * gx + gy * gy), 0, 1);
}

pixel_output main(pixel_input input)
{
	pixel_output output;
	float sobel = sobelNormal(input.uv);
	//float sobel = sobelPosition(input.uv);
	output.color = float4(float3(color_tex.Sample(color_sampler, input.uv).rgb * (1 - sobel)), 1);
	output.normal = normal_tex.Sample(normal_sampler, input.uv);
	output.position = position_tex.Sample(position_sampler, input.uv);
	return output;
}
