cbuffer model_block : register(b1)
{
	float4x4 p;
	float4x4 v;
	float4x4 mv;
	float4x4 mvp;
};

cbuffer scene_block : register(b2)
{
	float4 light_direction;
	float4 diffuse_color;
	float4 specular_color;
	float4 base_color;
	float4 final_color;
	float4 fog_color;
	float2 fog_range;
	float2 alphas;
};

struct vertex_input
{
	float3 position : VS_INPUT0;
	float depth : VS_INPUT1;
	float2 uv : VS_INPUT2;
};

struct pixel_input
{
	float4 out_position : SV_POSITION;
	float3 position : FS_INPUT0;
	float3 light_dir : FS_INPUT1;
	float3 diffuse : FS_INPUT2;
	float3 normal : FS_INPUT3;
	float alpha : FS_INPUT4;
	float2 uv : FS_INPUT5;
};

pixel_input main(vertex_input input)
{
	pixel_input output;
	float4 position_fixed = float4(input.position, 1);
	output.out_position = mul(position_fixed, mvp);
	output.position = mul(position_fixed, mv).xyz;
	output.normal = normalize(mul(float4(0, 1, 0, 0), mv).xyz);
	output.light_dir = normalize(mul(-light_direction, v).xyz);
	output.uv = input.uv;
	float tmp = min(1, input.depth * 4);
	output.diffuse = lerp(base_color.xyz, final_color.xyz, tmp);
	output.alpha = lerp(alphas.x, alphas.y, tmp);
	return output;
}
