cbuffer model_block : register(b1)
{
	float4x4 p;
	float4x4 v;
	float4x4 mv;
	float4x4 mvp;
};

cbuffer scene_block : register(b2)
{
	float4 light_direction;
	float4 diffuse_color;
	float4 specular_color;
	float4 base_color;
	float4 final_color;
	float4 fog_color;
	float2 fog_range;
	float2 alphas;
};

struct pixel_input
{
	float4 out_position : SV_POSITION;
	float3 position : FS_INPUT0;
	float3 light_dir : FS_INPUT1;
	float3 diffuse : FS_INPUT2;
	float3 normal : FS_INPUT3;
	float alpha : FS_INPUT4;
	float2 uv : FS_INPUT5;
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
	float3 tex_col = tex.Sample(tex_sampler, input.uv).rgb;
	float specular_factor = pow(clamp(dot(normalize(-input.position), normalize(reflect(-input.light_dir, input.normal))), 0, 1), 10) * 20;
	float3 color = input.diffuse + tex_col * (1 + specular_color.xyz * specular_factor);
	float fog_factor = clamp((length(input.position) - fog_range.x) / (fog_range.y - fog_range.x), 0, 1);
	output.color = float4(lerp(color, fog_color.xyz, fog_factor), input.alpha);
	output.normal = float4(0, 1, 0, 0);
	output.position = float4(input.position, 1);
	return output;
}
