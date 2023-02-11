cbuffer mesh_block : register(b0)
{
	float4 emissive_color;
	int4 combiners;
	int4 settings;
	float alpha_test;
};

cbuffer model_block : register(b1)
{
	float4x4 v;
	float4x4 mv;
	float4x4 mvp;
};

cbuffer scene_block : register(b2)
{
	float4 light_direction;
	float4 specular_color;
	float4 diffuse_color;
	float4 ambient_color;
	float4 fog_color;
	float2 fog_range;
};

struct pixel_input
{
	float4 out_position : SV_POSITION;
	float3 position : FS_INPUT0;
	float3 light_dir : FS_INPUT1;
	float3 normal : FS_INPUT2;
	float4 color : FS_INPUT3;
	float3 light : FS_INPUT4;
	float2 uv1 : FS_INPUT5;
	float2 uv2 : FS_INPUT6;
};

struct pixel_output
{
	float4 color : SV_TARGET0;
	float4 normal : SV_TARGET1;
	float4 position : SV_TARGET2;
};

Texture2D tex1 : register(t0);
SamplerState tex1_sampler : register(s0);

pixel_output main(pixel_input input)
{
	pixel_output output;
	float4 input_color = float4(lerp(input.color.xyz, input.light, input.color.a), 1);
	float4 tex1_color = tex1.Sample(tex1_sampler, input.uv1);
	float4 output_color = input_color;
	switch (combiners.y)
	{
		case 0: //Diffuse
		{
			output_color = tex1_color * input_color;
			//output_color = float4(.25, .25, .25, 1) * float4(float3(length(input_color.rgb)), 1);
			break;
		}
		case 1: //Specular
		{
			float specular_factor = pow(clamp(dot(normalize(-input.position), normalize(reflect(-input.light_dir, input.normal))), 0, 1), 10);
			output_color = tex1_color * input_color;
			output_color.rgb += float3(specular_factor * specular_color.xyz);
			//output_color = float4(1, 1, 1, 1) * float4(float3(length(input_color.rgb)), 1);
			break;
		}
		case 2: //Metal
		{
			float specular_factor = pow(clamp(dot(normalize(-input.position), normalize(reflect(-input.light_dir, input.normal))), 0, 1), 10);
			output_color = tex1_color * input_color;
			output_color.rgb += float3(specular_factor * specular_color.xyz * tex1_color.a * 4);
			//output_color = float4(0, 1, 1, 1) * float4(float3(length(input_color.rgb)), 1);
			break;
		}
		case 3: //Env
		{
			//2 textures
			output_color = tex1_color * input_color;
			//output_color = float4(0, 0, 1, 1) * float4(float3(length(input_color.rgb)), 1);
			break;
		}
		case 4: //Opaque
		{
			output_color.rgb = input_color.rgb * tex1_color.rgb;
			//output_color = float4(1, 0, 1, 1) * float4(float3(length(input_color.rgb)), 1);
			break;
		}
		case 5: //EnvMetal
		{
			//2 textures
			float specular_factor = pow(clamp(dot(normalize(-input.position), normalize(reflect(-input.light_dir, input.normal))), 0, 1), 10);
			output_color = tex1_color * input_color + float4(float3(specular_factor * specular_color.xyz * tex1_color.rgb), 1);
			//output_color = float4(1, 1, 0, 1) * float4(float3(length(input_color.rgb)), 1);
			break;
		}
		case 6: //MapObjTwoLayerDiffuse
		{
			float len = length(input_color.rgb);
			output_color = float4(1, 0, 0, 1) * float4(len, len, len, 1);
			break;
		}
		default:
		{
			float len = length(input_color.rgb);
			output_color = float4(0, 1, 0, 1) * float4(len, len, len, 1);
			break;
		}
	}
	if (output_color.a < alpha_test)
		discard;
	if (settings.z == 0)
	{
		float fog_factor = clamp((length(input.position) - fog_range.x) / (fog_range.y - fog_range.x), 0, 1);
		output_color = float4(lerp(output_color.rgb, fog_color.xyz, fog_factor), output_color.a);
	}
	output.color = output_color;
	output.normal = float4(input.normal, 1);
	output.position = float4(input.position, 0);
	return output;
}
