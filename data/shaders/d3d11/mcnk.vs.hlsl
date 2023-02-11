struct mesh_block_st
{
	float2 uv_offset0;
	float2 uv_offset1;
	float2 uv_offset2;
	float2 uv_offset3;
};

cbuffer mesh_block : register(b0)
{
	mesh_block_st mesh_blocks[256];
};

cbuffer model_block : register(b1)
{
	float4x4 v;
	float4x4 mv;
	float4x4 mvp;
	float offset_time;
};

cbuffer scene_block : register(b2)
{
	float4 light_direction;
	float4 ambient_color;
	float4 diffuse_color;
	float4 specular_color;
	float4 fog_color;
	float2 fog_range;
};

struct vertex_input
{
	uint id : SV_VertexID;
	float3 normal : VS_INPUT0;
	float2 xz : VS_INPUT1;
	float2 uv : VS_INPUT2;
	float y : VS_INPUT3;
};

struct pixel_input
{
	float4 out_position : SV_POSITION;
	float3 position : PS_INPUT0;
	float3 light_dir : PS_INPUT1;
	float3 normal : PS_INPUT2;
	float2 uv0 : PS_INPUT3;
	float2 uv1 : PS_INPUT4;
	float2 uv2 : PS_INPUT5;
	float2 uv3 : PS_INPUT6;
	float2 uv4 : PS_INPUT7;
	nointerpolation int id : PS_INPUT8;
};

pixel_input main(vertex_input input)
{
	pixel_input output;
	float4 position_fixed = float4(input.xz.x, input.y, input.xz.y, 1);
	float4 normal_fixed = float4(input.normal, 0);
	output.out_position =  mul(position_fixed, mvp);
	output.position = mul(position_fixed, mv).xyz;
	output.normal = normalize(mul(normal_fixed, mv).xyz);
	output.light_dir = normalize(mul(-light_direction, v).xyz);
	output.uv0 = input.uv;
	int offset = input.id / 145;
	output.uv1 = -(input.uv + mesh_blocks[offset].uv_offset0 * offset_time).yx * 8;
	output.uv2 = -(input.uv + mesh_blocks[offset].uv_offset1 * offset_time).yx * 8;
	output.uv3 = -(input.uv + mesh_blocks[offset].uv_offset2 * offset_time).yx * 8;
	output.uv4 = -(input.uv + mesh_blocks[offset].uv_offset3 * offset_time).yx * 8;
	output.id = input.id;
	return output;
}
