struct vertex_input
{
	float3 position : VS_INPUT0;
};

struct pixel_input
{
	float4 out_position : SV_POSITION;
	float3 position : FS_INPUT0;
};

cbuffer model_block : register(b1)
{
	float4x4 mvp;
	float4x4 mv;
	float4 color;
};

pixel_input main(vertex_input input)
{
	pixel_input output;
	float4 position_fixed = float4(input.position, 1);
	output.out_position = mul(position_fixed, mvp);
	output.position = mul(position_fixed, mv).xyz;
	return output;
}
