struct Light
{
	float4 ambient;
	float4 diffuse;
	float4 position;
	float2 attenuation;
	float2 data;
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

struct vertex_input
{
	float3 position : VS_INPUT0;
	float4 color : VS_INPUT1;
	int bone : VS_INPUT2;
};

struct pixel_input
{
	float4 out_position : SV_POSITION;
	float4 color : FS_INPUT0;
};

pixel_input main(vertex_input input)
{
	pixel_input output;
	float4 boned = float4(input.position.x, input.position.y, input.position.z, 1);
	if (input.bone != -1)
		boned = mul(boned, bone_mats[input.bone]);
	output.out_position = mul(boned, mvp);
	output.color = input.color;
	return output;
}
