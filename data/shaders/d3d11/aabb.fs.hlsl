cbuffer mesh_block : register(b0)
{
	float4x4 mvp;
	float4 color;
};

struct pixel_input
{
	float4 position : SV_POSITION;
};

struct pixel_output
{
	float4 color : SV_TARGET0;
};

pixel_output main(pixel_input input)
{
	pixel_output output;
	output.color = color;
	return output;
}
