cbuffer scene_block : register(b2)
{
	float4 diffuse_color;
	float4 fog_color;
	float2 fog_range;
};

struct pixel_input
{
	float4 out_position : SV_POSITION;
	float3 position : FS_INPUT0;
	float3 diffuse : FS_INPUT1;
	float3 normal : FS_INPUT2;
	float2 uv : FS_INPUT3;
};

struct pixel_output
{
	float4 color : SV_TARGET0;
	float4 normal : SV_TARGET1;
	float4 position : SV_TARGET2;
};

Texture2D tex : register(t0);
SamplerState tex_sampler : register(s0);

pixel_output main(pixel_input input)
{
	pixel_output output;
	float3 color = tex.Sample(tex_sampler, input.uv).rgb;
	float fog_factor = clamp((length(input.position) - fog_range.x) / (fog_range.y - fog_range.x), 0, 1);
	output.color = float4(lerp(color, fog_color.xyz, fog_factor), 1);
	output.normal = float4(input.normal, 1);
	output.position = float4(input.position, 1);
	return output;
}
