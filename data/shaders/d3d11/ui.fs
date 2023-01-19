cbuffer model_block : register(b1)
{
	float4x4 mvp;
	float4 color;
	float alpha_test;
};

struct pixel_input
{
	float4 out_position : SV_POSITION;
	float4 color : FS_INPUT0;
	float2 uv : FS_INPUT1;
};

struct pixel_output
{
	float4 color : SV_TARGET0;
};

Texture2D tex : register(t0);
SamplerState tex_sampler : register(s0);

pixel_output main(pixel_input input)
{
	pixel_output output;
	output.color = input.color * tex.Sample(tex_sampler, input.uv);
	//if (output.color.a < alpha_test)
	//	discard;
	return output;
}
