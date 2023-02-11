struct pixel_input
{
	float4 position : SV_POSITION;
	float4 color : FS_INPUT0;
	float2 uv : FS_INPUT1;
};

float4 main(pixel_input input) : SV_TARGET
{
	return input.color;
}
