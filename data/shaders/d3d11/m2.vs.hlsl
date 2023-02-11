struct Light
{
	float4 ambient;
	float4 diffuse;
	float4 position;
	float2 attenuation;
	float2 data;
};

cbuffer mesh_block : register(b0)
{
	float4x4 tex1_matrix;
	float4x4 tex2_matrix;
	int4 combiners;
	int4 settings;
	float4 color;
	float3 fog_color;
	float1 alpha_test;
};

cbuffer model_block : register(b1)
{
	float4x4 v;
	float4x4 mv;
	float4x4 mvp;
	int4 lights_count;
	Light lights[4];
	float4x4 bone_mats[256];
};

cbuffer scene_block : register(b2)
{
	float4 light_direction;
	float4 specular_color;
	float4 diffuse_color;
	float4 ambient_color;
	float2 fog_range;
};

struct vertex_input
{
	float4 bone_weights : VS_INPUT0;
	float3 position : VS_INPUT1;
	float3 normal : VS_INPUT2;
	uint4 bones : VS_INPUT3;
	float2 uv1 : VS_INPUT4;
	float2 uv2 : VS_INPUT5;
};

struct pixel_input
{
	float4 out_position : SV_POSITION;
	float4 position_fixed : FS_INPUT0;
	float4 normal_fixed : FS_INPUT1;
	float3 position : FS_INPUT2;
	float3 diffuse : FS_INPUT3;
	float3 normal : FS_INPUT4;
	float2 uv1 : FS_INPUT5;
	float2 uv2 : FS_INPUT6;
};

# define M2_VERTEX_DIFFUSE                   0x1
# define M2_VERTEX_DIFFUSE_ENV_MAP           0x2
# define M2_VERTEX_DIFFUSE_2TEX              0x3
# define M2_VERTEX_DIFFUSE_ENV_MAP0_2TEX     0x4
# define M2_VERTEX_DIFFUSE_ENV_MAP_2TEX      0x5
# define M2_VERTEX_DIFFUSE_DUAL_ENV_MAP_2TEX 0x6

float2 diffuse_coord(float2 uv, row_major float4x4 mat)
{
	return mul(float4(uv - .5, 0, 1), mat).xy;
}

float2 env_coord(float3 position, float3 normal)
{
	return .5 + .5 * reflect(normalize(position), normal).xy;
}

pixel_input main(vertex_input input)
{
	pixel_input output;
	float4 position_fixed = float4(input.position.xyz, 1);
	float4 normal_fixed = float4(input.normal.xyz, 0);
	if (settings.z != 0)
	{
		float4x4 bone_mat =
		    input.bone_weights.x * bone_mats[input.bones.x];
		  + input.bone_weights.y * bone_mats[input.bones.y]
		  + input.bone_weights.z * bone_mats[input.bones.z]
		  + input.bone_weights.w * bone_mats[input.bones.w];
		position_fixed = mul(position_fixed, bone_mat);
		normal_fixed = mul(normal_fixed, bone_mat);
	}
	output.normal_fixed = normalize(normal_fixed);
	output.out_position = mul(position_fixed, mvp);
	output.position = mul(position_fixed, mv).xyz;
	output.normal = normalize(mul(output.normal_fixed, mv).xyz);
	switch (combiners.x)
	{
		case M2_VERTEX_DIFFUSE:
			output.uv1 = diffuse_coord(input.uv1, tex1_matrix);
			break;
		case M2_VERTEX_DIFFUSE_ENV_MAP:
			output.uv1 = env_coord(output.position, output.normal);
			break;
		case M2_VERTEX_DIFFUSE_2TEX:
			output.uv1 = diffuse_coord(input.uv1, tex1_matrix);
			output.uv2 = diffuse_coord(input.uv2, tex2_matrix);
			break;
		case M2_VERTEX_DIFFUSE_ENV_MAP0_2TEX:
			output.uv1 = env_coord(output.position, output.normal);
			output.uv2 = diffuse_coord(input.uv2, tex2_matrix);
			break;
		case M2_VERTEX_DIFFUSE_ENV_MAP_2TEX:
			output.uv1 = diffuse_coord(input.uv1, tex1_matrix);
			output.uv2 = env_coord(output.position, output.normal);
			break;
		case M2_VERTEX_DIFFUSE_DUAL_ENV_MAP_2TEX:
			output.uv1 = env_coord(output.position, output.normal);
			output.uv2 = output.uv1;
			break;
		default:
			output.uv1 = diffuse_coord(input.uv1, tex1_matrix);
			output.uv2 = diffuse_coord(input.uv2, tex2_matrix);
			break;
	}
	if (settings.y == 0)
	{
		float3 light_dir = normalize((mul(-light_direction, v)).xyz);
		output.diffuse = diffuse_color.xyz * clamp(dot(output.normal, light_dir), 0, 1);
		/*for (int i = 0; i < lights_count.x; ++i)
		{
			output.diffuse += lights[i].ambient.rgb * lights[i].ambient.a;
		}*/
	}
	return output;
}
