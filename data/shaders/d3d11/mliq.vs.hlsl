cbuffer model_block : register(b1)
{
	matrix mvp;
};

struct vertex_input
{
	float2 position : VS_INPUT0;
	float2 uv : VS_INPUT1;
};

struct pixel_input
{
	float2 uv : FS_INPUT0;
};

pixel_input main(vertex_input input)
{
	pixel_input output;
	output.uv = input.uv;
	return output;
}
