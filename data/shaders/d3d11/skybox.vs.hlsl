cbuffer model_block : register(b1)
{
	float4x4 mvp;
	float4 sky_colors[6];
	float4 clouds_colors[2];
	float2 clouds_factors;
	float clouds_blend;
	float clouds_drift;
};

struct vertex_input
{
	float3 position : VS_INPUT0;
	float color0 : VS_INPUT1;
	float color1 : VS_INPUT2;
	float color2 : VS_INPUT3;
	float color3 : VS_INPUT4;
	float color4 : VS_INPUT5;
	float2 uv : VS_INPUT6;
};

struct pixel_input
{
	float4 position : SV_POSITION;
	float4 color : PS_INPUT0;
	float2 uv : PS_INPUT1;
};

pixel_input main(vertex_input input)
{
	pixel_input output;
	output.position = mul(float4(input.position, 1), mvp);
	output.color = sky_colors[0];
	output.color = lerp(output.color, sky_colors[1], input.color0);
	output.color = lerp(output.color, sky_colors[2], input.color1);
	output.color = lerp(output.color, sky_colors[3], input.color2);
	output.color = lerp(output.color, sky_colors[4], input.color3);
	output.color = lerp(output.color, sky_colors[5], input.color4);
	output.uv = input.uv;
	return output;
}
