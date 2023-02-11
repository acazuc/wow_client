cbuffer model_block : register(b1)
{
	float4x4 mvp;
};

struct vertex_input
{
	float2 position : VS_INPUT0;
	float2 uv : VS_INPUT1;
};

struct pixel_input
{
	float4 out_position : SV_POSITION;
	float2 uv : FS_INPUT0;
};

pixel_input main(vertex_input input)
{
	pixel_input output;
	output.out_position = mul(float4(input.position, 0, 1), mvp);
	output.uv = input.uv;
	return output;
}
