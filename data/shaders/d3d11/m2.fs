struct Light
{
	float4 ambient;
	float4 diffuse;
	float4 position;
	float2 attenuation;
	float2 data;
};

cbuffer mesh_block : register(b0)
{
	float4x4 tex1_matrix;
	float4x4 tex2_matrix;
	int4 combiners;
	int4 settings;
	float4 color;
	float3 fog_color;
	float alpha_test;
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

cbuffer scene_block : register(b2)
{
	float4 light_direction;
	float4 specular_color;
	float4 diffuse_color;
	float4 ambient_color;
	float2 fog_range;
};

struct pixel_input
{
	float4 out_position : SV_POSITION;
	float4 position_fixed : FS_INPUT0;
	float4 normal_fixed : FS_INPUT1;
	float3 position : FS_INPUT2;
	float3 diffuse : FS_INPUT3;
	float3 normal : FS_INPUT4;
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
Texture2D tex2 : register(t1);
SamplerState tex1_sampler : register(s0);
SamplerState tex2_sampler : register(s1);

# define M2_FRAGMENT_OPAQUE                  0x1
# define M2_FRAGMENT_DIFFUSE                 0x2
# define M2_FRAGMENT_DECAL                   0x3
# define M2_FRAGMENT_ADD                     0x4
# define M2_FRAGMENT_DIFFUSE2X               0x5
# define M2_FRAGMENT_FADE                    0x6
# define M2_FRAGMENT_OPAQUE_OPAQUE           0x7
# define M2_FRAGMENT_OPAQUE_MOD              0x8
# define M2_FRAGMENT_OPAQUE_DECAL            0x9
# define M2_FRAGMENT_OPAQUE_ADD              0xa
# define M2_FRAGMENT_OPAQUE_MOD2X            0xb
# define M2_FRAGMENT_OPAQUE_FADE             0xc
# define M2_FRAGMENT_DIFFUSE_2TEX_OPAQUE     0xd
# define M2_FRAGMENT_DIFFUSE_2TEX            0xe
# define M2_FRAGMENT_DIFFUSE_2TEX_DECAL      0xf
# define M2_FRAGMENT_DIFFUSE_2TEX_ADD        0x10
# define M2_FRAGMENT_DIFFUSE_2TEX_2X         0x11
# define M2_FRAGMENT_DIFFUSE_2TEX_FADE       0x12
# define M2_FRAGMENT_DECAL_OPAQUE            0x13
# define M2_FRAGMENT_DECAL_MOD               0x14
# define M2_FRAGMENT_DECAL_DECAL             0x15
# define M2_FRAGMENT_DECAL_ADD               0x16
# define M2_FRAGMENT_DECAL_MOD2X             0x17
# define M2_FRAGMENT_DECAL_FADE              0x18
# define M2_FRAGMENT_ADD_OPAQUE              0x19
# define M2_FRAGMENT_ADD_MOD                 0x1a
# define M2_FRAGMENT_ADD_DECAL               0x1b
# define M2_FRAGMENT_ADD_ADD                 0x1c
# define M2_FRAGMENT_ADD_MOD2X               0x1d
# define M2_FRAGMENT_ADD_FADE                0x1e
# define M2_FRAGMENT_DIFFUSE_2TEX_OPAQUE_2X  0x1f
//# define M2_FRAGMENT_DIFFUSE_2TEX_2X         0x20
# define M2_FRAGMENT_DIFFUSE_2TEX_DECAL_2X   0x21
# define M2_FRAGMENT_DIFFUSE_2TEX_ADD_2X     0x22
# define M2_FRAGMENT_DIFFUSE_2TEX_4X         0x23
# define M2_FRAGMENT_DIFFUSE_2TEX_FADE_2X    0x24

pixel_output main(pixel_input input)
{
	pixel_output output;
	float4 input_color;
	if (settings.y == 0)
	{
		input_color = float4(input.diffuse + ambient_color.xyz, 1);
		for (int i = 0; i < lights_count.x; ++i)
		{
			float1 diffuse_factor;
			float3 attenuated;
			if (lights[i].data.y == 0)
			{
				float3 light_dir = lights[i].position.xyz;
				diffuse_factor = clamp(dot(input.normal_fixed.xyz, normalize(light_dir)), 0, 1);
				attenuated = lights[i].diffuse.rgb;
			}
			else
			{
				float3 light_dir = lights[i].position.xyz - input.position_fixed.xyz;
				diffuse_factor = clamp(dot(input.normal_fixed.xyz, normalize(light_dir)), 0, 1);
				float1 len = length(light_dir);
				float1 attenuation;
				//attenuation = 0;
				//attenuation = 1 / (((lights[i].attenuation.x + lights[i].attenuation.y * len) * len));
				//attenuation = 1 - ((lights[i].attenuation.x + lights[i].attenuation.y * len) * len);
				attenuation = 1 - clamp((len - lights[i].attenuation.x) / (lights[i].attenuation.y - lights[i].attenuation.x), 0, 1);
				attenuated = lights[i].diffuse.rgb * attenuation;
				attenuated = attenuated * attenuated;
			}
			input_color.rgb += lights[i].ambient.rgb * lights[i].ambient.a + /*lights[i].diffuse.a * */ diffuse_factor * attenuated/* * lights[i].data.x*/;
		}
	}
	else
	{
		input_color = float4(1, 1, 1, 1);
	}
	input_color *= color;
	float4 tex1_color = tex1.Sample(tex1_sampler, input.uv1);
	float4 output_color = input_color;
	switch (combiners.y)
	{
		case M2_FRAGMENT_OPAQUE:
			output_color.rgb = input_color.rgb * tex1_color.rgb;
			output_color.a = input_color.a;
			break;
		case M2_FRAGMENT_DIFFUSE:
			output_color = input_color * tex1_color;
			break;
		case M2_FRAGMENT_DECAL:
			output_color.rgb = lerp(input_color.rgb, tex1_color.rgb, input_color.a);
			output_color.a = input_color.a;
			break;
		case M2_FRAGMENT_ADD:
			output_color = input_color + tex1_color;
			break;
		case M2_FRAGMENT_DIFFUSE2X:
			output_color = input_color * tex1_color * 2;
			break;
		case M2_FRAGMENT_FADE:
			output_color.rgb = lerp(tex1_color.rgb, input_color.rgb, input_color.a);
			output_color.a = input_color.a;
			break;
		case M2_FRAGMENT_OPAQUE_OPAQUE:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color.rgb = input_color.rgb * tex1_color.rgb * tex2_color.rgb;
			output_color.a = input_color.a;
			break;
		}
		case M2_FRAGMENT_OPAQUE_MOD:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color.rgb = input_color.rgb * tex1_color.rgb * tex2_color.rgb;
			output_color.a = input_color.a * tex2_color.a;
			break;
		}
		case M2_FRAGMENT_OPAQUE_DECAL:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color.rgb = (input_color.rgb * tex1_color.rgb - tex2_color.rgb) * tex1_color.a + tex2_color.rgb;
			output_color.a = input_color.a;
			break;
		}
		case M2_FRAGMENT_OPAQUE_ADD:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color.rgb = input_color.rgb * tex1_color.rgb + tex2_color.rgb;
			output_color.a = input_color.a * tex2_color.a;
			break;
		}
		case M2_FRAGMENT_OPAQUE_MOD2X:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color.rgb = input_color.rgb * tex1_color.rgb * tex2_color.rgb * 2;
			output_color.a = input_color.a * tex2_color.a * 2;
			break;
		}
		case M2_FRAGMENT_OPAQUE_FADE:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color.rgb = (tex2_color.rgb - tex1_color.rgb * input_color.rgb) * input_color.a + tex1_color.rgb;
			output_color.a = input_color.a;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_OPAQUE:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color.rgb = input_color.rgb * tex1_color.rgb * tex2_color.rgb;
			output_color.a = input_color.a * tex1_color.a;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color = input_color * tex1_color * tex2_color;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_DECAL:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			float4 tmp = input_color * tex1_color;
			tmp.rgb -= tex2_color.rgb;
			output_color.rgb = tmp.rgb * tmp.a + tex2_color.rgb;
			output_color.a = tmp.a;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_ADD:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color = input_color * tex1_color + tex2_color;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_2X:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color = input_color * tex1_color * tex2_color * 2;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_FADE:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			float4 tmp = tex1_color * input_color;
			output_color.rgb = lerp(tex2_color.rgb, tmp.rgb, input_color.a);
			output_color.a = tmp.a;
			break;
		}
		case M2_FRAGMENT_DECAL_OPAQUE:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color.rgb = lerp(input_color.rgb, tex1_color.rgb, input_color.a) * tex2_color.rgb;
			output_color.a = input_color.a;
			break;
		}
		case M2_FRAGMENT_DECAL_MOD:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color.rgb = ((input_color.rgb - tex1_color.rgb) * input_color.a + tex1_color.rgb) * tex2_color.rgb;
			output_color.a = input_color.a * tex2_color.a;
			break;
		}
		case M2_FRAGMENT_DECAL_DECAL:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color.rgb = tex1_color.a * (lerp(input_color.rgb, tex1_color.rgb, input_color.a) - tex2_color.rgb) * tex2_color.rgb;
			output_color.a = input_color.a;
			break;
		}
		case M2_FRAGMENT_DECAL_ADD:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color.rgb = tex2_color.rgb + lerp(input_color.rgb, tex1_color.rgb, input_color.a);
			output_color.a = input_color.a + tex2_color.a;
			break;
		}
		case M2_FRAGMENT_DECAL_MOD2X:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color.rgb = tex2_color.rgb * lerp(input_color.rgb, tex1_color.rgb, input_color.a);
			output_color.a = input_color.a * tex2_color.a;
			output_color = output_color * 2;
			break;
		}
		case M2_FRAGMENT_DECAL_FADE:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			float3 tmp = (input_color.rgb - tex1_color.rgb) * input_color.a + tex1_color.rgb;
			output_color.rgb = lerp(tex2_color.rgb, tmp, input_color.a);
			output_color.a = input_color.a;
			break;
		}
		case M2_FRAGMENT_ADD_OPAQUE:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color.rgb = (input_color.rgb + tex1_color.rgb) * tex2_color.rgb;
			output_color.a = input_color.a + tex1_color.a;
			break;
		}
		case M2_FRAGMENT_ADD_MOD:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color = (input_color + tex1_color) * tex2_color;
			break;
		}
		case M2_FRAGMENT_ADD_DECAL:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color.rgb = (input_color.rgb + tex1_color.rgb - tex2_color.rgb) * tex1_color.a + tex2_color.rgb;
			output_color.a = input_color.a + tex1_color.a;
			break;
		}
		case M2_FRAGMENT_ADD_ADD:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color = input_color + tex1_color + tex2_color;
			break;
		}
		case M2_FRAGMENT_ADD_MOD2X:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color = (input_color + tex1_color) * tex2_color * 2;
			break;
		}
		case M2_FRAGMENT_ADD_FADE:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			float3 tmp = input_color.rgb + tex1_color.rgb;
			output_color.rgb = lerp(tex2_color.rgb, tmp, input_color.a);
			output_color.a = input_color.a + tex1_color.a;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_OPAQUE_2X:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color.rgb = input_color.rgb * tex1_color.rgb * tex2_color.rgb * 2;
			output_color.a = input_color.a * tex1_color.a;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_DECAL_2X:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			float4 tmp = input_color * tex1_color * 2;
			output_color.rgb = lerp(tmp.rgb, tex2_color.rgb, tmp.a);
			output_color.a = tmp.a;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_ADD_2X:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color = input_color * tex1_color * 2 + tex2_color;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_4X:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			output_color = input_color * tex1_color * tex2_color * 4;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_FADE_2X:
		{
			float4 tex2_color = tex2.Sample(tex2_sampler, input.uv2);
			float3 tmp = input_color.rgb * tex1_color.rgb * 2;
			output_color.rgb = lerp(tex2_color.rgb, tmp, input_color.a);
			output_color.a = input_color.a * tex1_color.a;
			break;
		}
		default:
			output_color = float4(0, 1, 0, 1);
			break;
	}
	if (output_color.a < alpha_test)
		discard;
	if (settings.x == 0)
	{
		float fog_factor = clamp((length(input.position) - fog_range.x) / (fog_range.y - fog_range.x), 0, 1);
		output_color = float4(lerp(output_color.rgb, fog_color, fog_factor), output_color.a);
	}
	output.color = clamp(output_color, 0, 1);
	output.normal = float4(input.normal, 1);
	output.position = float4(input.position, 1);
	return output;
}
