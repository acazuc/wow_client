struct pixel_input
{
	float4 out_position : SV_POSITION;
	float3 position : FS_INPUT0;
};

struct pixel_output
{
	float4 color : SV_TARGET0;
	float4 normal : SV_TARGET1;
	float4 position : SV_TARGET2;
};

cbuffer model_block : register(b1)
{
	float4x4 mvp;
	float4x4 mv;
	float4 color;
};

pixel_output main(pixel_input input)
{
	pixel_output output;
	output.color = color;
	output.normal = float4(0, 1, 0, 0);
	output.position = float4(0, 0, 0, 0); //float4(input.position, 1);
	return output;
}
