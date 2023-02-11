cbuffer mesh_block : register(b0)
{
	float4x4 mvp;
	float4 color;
};

struct vertex_input
{
	float3 position : VS_INPUT0;
};

struct pixel_input
{
	float4 position : SV_POSITION;
};

pixel_input main(vertex_input input)
{
	pixel_input output;
	output.position = mul(float4(input.position, 1), mvp);
	return output;
}
