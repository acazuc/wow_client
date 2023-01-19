cbuffer model_block : register(b1)
{
	float4x4 v;
	float4x4 mv;
	float4x4 mvp;
};

cbuffer scene_block : register(b2)
{
	float4 diffuse_color;
	float4 fog_color;
	float2 fog_range;
};

struct vertex_input
{
	float3 position : VS_INPUT0;
	float2 uv : VS_INPUT1;
};

struct pixel_input
{
	float4 out_position : SV_POSITION;
	float3 position : FS_INPUT0;
	float3 diffuse : FS_INPUT1;
	float3 normal : FS_INPUT2;
	float2 uv : FS_INPUT3;
};

pixel_input main(vertex_input input)
{
	pixel_input output;
	float4 position_fixed = float4(input.position, 1);
	output.out_position = mul(position_fixed, mvp);
	output.position = mul(position_fixed, mv).xyz;
	output.normal = normalize(mul(float4(0, 1, 0, 0), mv).xyz);
	output.uv = input.uv;
	return output;
}
