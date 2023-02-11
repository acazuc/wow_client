cbuffer model_block : register(b1)
{
	float4x4 mvp;
	float4 color;
	float alpha_test;
};

struct vertex_input
{
	float2 position : VS_INPUT0;
	float4 color : VS_INPUT1;
	float2 uv : VS_INPUT2;
};

struct pixel_input
{
	float4 out_position : SV_POSITION;
	float4 color : FS_INPUT0;
	float2 uv : FS_INPUT1;
};

pixel_input main(vertex_input input)
{
	pixel_input output;
	output.out_position = mul(input.position, mvp);
	output.color = input.color * color;
	output.uv = input.uv;
	return output;
}
