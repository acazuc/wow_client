struct mesh_block_st
{
	float2 uv_offset0;
	float2 uv_offset1;
	float2 uv_offset2;
	float2 uv_offset3;
};

cbuffer mesh_block : register(b0)
{
	mesh_block_st mesh_blocks[256];
};

cbuffer model_block : register(b1)
{
	float4x4 v;
	float4x4 mv;
	float4x4 mvp;
	float offset_time;
};

cbuffer scene_block : register(b2)
{
	float4 light_direction;
	float4 ambient_color;
	float4 diffuse_color;
	float4 specular_color;
	float4 fog_color;
	float2 fog_range;
};

struct pixel_input
{
	float4 out_position : SV_POSITION;
	float3 position : PS_INPUT0;
	float3 light_dir : PS_INPUT1;
	float3 normal : PS_INPUT2;
	float2 uv0 : PS_INPUT3;
	float2 uv1 : PS_INPUT4;
	float2 uv2 : PS_INPUT5;
	float2 uv3 : PS_INPUT6;
	float2 uv4 : PS_INPUT7;
	nointerpolation int id : PS_INPUT8;
};

struct pixel_output
{
	float4 color : SV_TARGET0;
	float4 normal : SV_TARGET1;
	float4 position : SV_TARGET2;
};

Texture2DArray alpha_map : register(t0);
SamplerState alpha_map_sampler : register(s0);
Texture2D textures[8] : register(t1);
SamplerState samplers[8] : register(s1);

pixel_output main(pixel_input input)
{
	pixel_output output;
	float4 alpha = alpha_map.Sample(alpha_map_sampler, float3(input.uv0, input.id / 145));
	float3 t1 = textures[0].Sample(samplers[0], input.uv1).rgb;
	float3 t2 = textures[2].Sample(samplers[2], input.uv2).rgb;
	float3 t3 = textures[4].Sample(samplers[4], input.uv3).rgb;
	float3 t4 = textures[6].Sample(samplers[6], input.uv4).rgb;
	float3 diff_mix = lerp(lerp(lerp(t1, t2, alpha.r), t3, alpha.g), t4, alpha.b) * (alpha.a * .3 + .7);
	//diff_mix = alpha.rgb;
	float4 s1 = textures[1].Sample(samplers[1], input.uv1);
	float4 s2 = textures[3].Sample(samplers[3], input.uv2);
	float4 s3 = textures[5].Sample(samplers[5], input.uv3);
	float4 s4 = textures[7].Sample(samplers[7], input.uv4);
	float4 spec_mix = lerp(lerp(lerp(s1, s2, alpha.r), s3, alpha.g), s4, alpha.b);
	float specular_factor = pow(clamp(dot(normalize(-input.position), normalize(reflect(-input.light_dir, input.normal))), 0, 1), 10);
	specular_factor = specular_factor * alpha.a;
	float3 specular = clamp(specular_factor * specular_color.xyz, 0, 1);
	specular *= spec_mix.rgb * spec_mix.a;
	specular *= 2;
	float3 diffuse = diffuse_color.xyz * clamp(dot(input.normal, input.light_dir), 0, 1);
	float3 color = clamp(diffuse + ambient_color.xyz, 0, 1);
	float fog_factor = clamp((length(input.position) - fog_range.x) / (fog_range.y - fog_range.x), 0, 1);
	output.color = float4(lerp(diff_mix * color + specular, fog_color.xyz, fog_factor), 1);
	output.normal = float4(input.normal, 1);
	output.position = float4(input.position, 1);
	return output;
}
