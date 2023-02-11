cbuffer model_block : register(b1)
{
	float4x4 mvp;
	float4 colors[6];
};

struct vertex_input
{
	float3 position : VS_INPUT0;
	float color0 : VS_INPUT1;
	float color1 : VS_INPUT2;
	float color2 : VS_INPUT3;
	float color3 : VS_INPUT4;
	float color4 : VS_INPUT5;
};

struct pixel_input
{
	float4 position : SV_POSITION;
	float4 color : PS_INPUT0;
};

pixel_input main(vertex_input input)
{
	pixel_input output;
	output.position = mul(float4(input.position, 1), mvp);
	output.color = colors[0];
	output.color = lerp(output.color, colors[1], input.color0);
	output.color = lerp(output.color, colors[2], input.color1);
	output.color = lerp(output.color, colors[3], input.color2);
	output.color = lerp(output.color, colors[4], input.color3);
	output.color = lerp(output.color, colors[5], input.color4);
	return output;
}
