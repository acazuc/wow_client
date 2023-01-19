cbuffer mesh_block : register(b0)
{
	float4 emissive_color;
	int4 combiners;
	int4 settings;
	float alpha_test;
};

cbuffer model_block : register(b1)
{
	float4x4 v;
	float4x4 mv;
	float4x4 mvp;
};

cbuffer scene_block : register(b2)
{
	float4 light_direction;
	float4 specular_color;
	float4 diffuse_color;
	float4 ambient_color;
	float4 fog_color;
	float2 fog_range;
};

struct vertex_input
{
	float3 position : VS_INPUT0;
	float3 normal : VS_INPUT1;
	float2 uv : VS_INPUT2;
	float4 color : VS_INPUT3;
};

struct pixel_input
{
	float4 out_position : SV_POSITION;
	float3 position : FS_INPUT0;
	float3 light_dir : FS_INPUT1;
	float3 normal : FS_INPUT2;
	float4 color : FS_INPUT3;
	float3 light : FS_INPUT4;
	float2 uv1 : FS_INPUT5;
	float2 uv2 : FS_INPUT6;
};

pixel_input main(vertex_input input)
{
	pixel_input output;
	float4 position_fixed = float4(input.position.x, input.position.z, -input.position.y, 1);
	float4 normal_fixed = float4(input.normal.x, input.normal.z, -input.normal.y, 0);
	output.out_position = mul(position_fixed, mvp);
	output.position = mul(position_fixed, mv).xyz;
	output.uv1 = input.uv;
	switch (combiners.x)
	{
		case 0:
			output.uv1 = input.uv;
			break;
		case 1:
			break;
		case 2:
			break;
	}
	if (settings.x != 0)
		output.color = input.color.bgra;
	else
		output.color = float4(1, 1, 1, 1);
	output.normal = normalize(mul(normal_fixed, mv).xyz);
	if (settings.y == 0)
	{
		output.light_dir = normalize(mul(-light_direction, v).xyz);
		float diffuse_factor = clamp(dot(output.normal, output.light_dir), 0, 1);
		output.light = ambient_color.xyz + diffuse_color.xyz * diffuse_factor + emissive_color.xyz;
	}
	else
	{
		output.light = float3(1, 1, 1);
	}
	return output;
}
