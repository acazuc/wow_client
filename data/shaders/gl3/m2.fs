#version 330

in fs_block
{
	vec4 fs_position_fixed;
	vec4 fs_normal_fixed;
	vec3 fs_position;
	vec3 fs_diffuse;
	vec3 fs_normal;
	vec2 fs_uv1;
	vec2 fs_uv2;
};

uniform sampler2D tex1;
uniform sampler2D tex2;

struct Light
{
	vec4 ambient;
	vec4 diffuse;
	vec4 position;
	vec2 attenuation;
	vec2 data;
};

layout(std140) uniform mesh_block
{
	mat4 tex1_matrix;
	mat4 tex2_matrix;
	ivec4 combiners;
	ivec4 settings;
	vec4 color;
	vec3 fog_color;
	float alpha_test;
};

layout(std140) uniform model_block
{
	mat4 v;
	mat4 mv;
	mat4 mvp;
	ivec4 lights_count;
	Light lights[4];
	mat4 bone_mats[256];
};

layout(std140) uniform scene_block
{
	vec4 light_direction;
	vec4 specular_color;
	vec4 diffuse_color;
	vec4 ambient_color;
	vec2 fog_range;
};

layout(location=0) out vec4 fragcolor;
layout(location=1) out vec4 fragnormal;
layout(location=2) out vec4 fragposition;

#define M2_FRAGMENT_OPAQUE                  0x1
#define M2_FRAGMENT_DIFFUSE                 0x2
#define M2_FRAGMENT_DECAL                   0x3
#define M2_FRAGMENT_ADD                     0x4
#define M2_FRAGMENT_DIFFUSE2X               0x5
#define M2_FRAGMENT_FADE                    0x6
#define M2_FRAGMENT_OPAQUE_OPAQUE           0x7
#define M2_FRAGMENT_OPAQUE_MOD              0x8
#define M2_FRAGMENT_OPAQUE_DECAL            0x9
#define M2_FRAGMENT_OPAQUE_ADD              0xa
#define M2_FRAGMENT_OPAQUE_MOD2X            0xb
#define M2_FRAGMENT_OPAQUE_FADE             0xc
#define M2_FRAGMENT_DIFFUSE_2TEX_OPAQUE     0xd
#define M2_FRAGMENT_DIFFUSE_2TEX            0xe
#define M2_FRAGMENT_DIFFUSE_2TEX_DECAL      0xf
#define M2_FRAGMENT_DIFFUSE_2TEX_ADD        0x10
#define M2_FRAGMENT_DIFFUSE_2TEX_2X         0x11
#define M2_FRAGMENT_DIFFUSE_2TEX_FADE       0x12
#define M2_FRAGMENT_DECAL_OPAQUE            0x13
#define M2_FRAGMENT_DECAL_MOD               0x14
#define M2_FRAGMENT_DECAL_DECAL             0x15
#define M2_FRAGMENT_DECAL_ADD               0x16
#define M2_FRAGMENT_DECAL_MOD2X             0x17
#define M2_FRAGMENT_DECAL_FADE              0x18
#define M2_FRAGMENT_ADD_OPAQUE              0x19
#define M2_FRAGMENT_ADD_MOD                 0x1a
#define M2_FRAGMENT_ADD_DECAL               0x1b
#define M2_FRAGMENT_ADD_ADD                 0x1c
#define M2_FRAGMENT_ADD_MOD2X               0x1d
#define M2_FRAGMENT_ADD_FADE                0x1e
#define M2_FRAGMENT_DIFFUSE_2TEX_OPAQUE_2X  0x1f
//#define M2_FRAGMENT_DIFFUSE_2TEX_2X         0x20
#define M2_FRAGMENT_DIFFUSE_2TEX_DECAL_2X   0x21
#define M2_FRAGMENT_DIFFUSE_2TEX_ADD_2X     0x22
#define M2_FRAGMENT_DIFFUSE_2TEX_4X         0x23
#define M2_FRAGMENT_DIFFUSE_2TEX_FADE_2X    0x24

void main()
{
	vec4 input;
	if (settings.y == 0)
	{
		input = vec4(fs_diffuse + ambient_color.xyz, 1);
		for (int i = 0; i < lights_count.x; ++i)
		{
			float diffuse_factor;
			vec3 attenuated;
			if (lights[i].data.y == 0)
			{
				vec3 light_dir = -lights[i].position.xyz;
				diffuse_factor = clamp(dot(fs_normal_fixed.xyz, normalize(light_dir)), 0, 1);
				attenuated = lights[i].diffuse.rgb;
			}
			else
			{
				vec3 light_dir = lights[i].position.xyz - fs_position_fixed.xyz;
				diffuse_factor = clamp(dot(fs_normal_fixed.xyz, normalize(light_dir)), 0, 1);
				float len = length(light_dir);
				float attenuation;
				//attenuation = 0;
				//attenuation = 1 / (((lights[i].attenuation.x + lights[i].attenuation.y * len) * len));
				//attenuation = 1 - ((lights[i].attenuation.x + lights[i].attenuation.y * len) * len);
				attenuation = 1 - clamp((len - lights[i].attenuation.x) / (lights[i].attenuation.y - lights[i].attenuation.x), 0, 1);
				attenuated = lights[i].diffuse.rgb * attenuation;
				attenuated = attenuated * attenuated;
			}
			input.rgb += lights[i].ambient.rgb * lights[i].ambient.a + /*lights[i].diffuse.a * */ diffuse_factor * attenuated/* * lights[i].data.x*/;
		}
	}
	else
	{
		input = vec4(1);
	}
	input *= color;
	vec4 tex1_color = texture(tex1, fs_uv1);
	vec4 output;
	switch (combiners.y)
	{
		case M2_FRAGMENT_OPAQUE:
			output.rgb = input.rgb * tex1_color.rgb;
			output.a = input.a;
			break;
		case M2_FRAGMENT_DIFFUSE:
			output = input * tex1_color;
			break;
		case M2_FRAGMENT_DECAL:
			output.rgb = mix(input.rgb, tex1_color.rgb, input.a);
			output.a = input.a;
			break;
		case M2_FRAGMENT_ADD:
			output = input + tex1_color;
			break;
		case M2_FRAGMENT_DIFFUSE2X:
			output = input * tex1_color * 2;
			break;
		case M2_FRAGMENT_FADE:
			output.rgb = mix(tex1_color.rgb, input.rgb, input.a);
			output.a = input.a;
			break;
		case M2_FRAGMENT_OPAQUE_OPAQUE:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output.rgb = input.rgb * tex1_color.rgb * tex2_color.rgb;
			output.a = input.a;
			break;
		}
		case M2_FRAGMENT_OPAQUE_MOD:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output.rgb = input.rgb * tex1_color.rgb * tex2_color.rgb;
			output.a = input.a * tex2_color.a;
			break;
		}
		case M2_FRAGMENT_OPAQUE_DECAL:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output.rgb = (input.rgb * tex1_color.rgb - tex2_color.rgb) * tex1_color.a + tex2_color.rgb;
			output.a = input.a;
			break;
		}
		case M2_FRAGMENT_OPAQUE_ADD:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output.rgb = input.rgb * tex1_color.rgb + tex2_color.rgb;
			output.a = input.a * tex2_color.a;
			break;
		}
		case M2_FRAGMENT_OPAQUE_MOD2X:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output.rgb = input.rgb * tex1_color.rgb * tex2_color.rgb * 2;
			output.a = input.a * tex2_color.a * 2;
			break;
		}
		case M2_FRAGMENT_OPAQUE_FADE:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output.rgb = (tex2_color.rgb - tex1_color.rgb * input.rgb) * input.a + tex1_color.rgb;
			output.a = input.a;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_OPAQUE:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output.rgb = input.rgb * tex1_color.rgb * tex2_color.rgb;
			output.a = input.a * tex1_color.a;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output = input * tex1_color * tex2_color;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_DECAL:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			vec4 tmp = input * tex1_color;
			tmp.rgb -= tex2_color.rgb;
			output.rgb = tmp.rgb * tmp.a + tex2_color.rgb;
			output.a = tmp.a;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_ADD:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output = input * tex1_color + tex2_color;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_2X:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output = input * tex1_color * tex2_color * 2;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_FADE:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			vec4 tmp = tex1_color * input;
			output.rgb = mix(tex2_color.rgb, tmp.rgb, input.a);
			output.a = tmp.a;
			break;
		}
		case M2_FRAGMENT_DECAL_OPAQUE:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output.rgb = mix(input.rgb, tex1_color.rgb, input.a) * tex2_color.rgb;
			output.a = input.a;
			break;
		}
		case M2_FRAGMENT_DECAL_MOD:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output.rgb = ((input.rgb - tex1_color.rgb) * input.a + tex1_color.rgb) * tex2_color.rgb;
			output.a = input.a * tex2_color.a;
			break;
		}
		case M2_FRAGMENT_DECAL_DECAL:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output.rgb = tex1_color.a * (mix(input.rgb, tex1_color.rgb, input.a) - tex2_color.rgb) * tex2_color.rgb;
			output.a = input.a;
			break;
		}
		case M2_FRAGMENT_DECAL_ADD:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output.rgb = tex2_color.rgb + mix(input.rgb, tex1_color.rgb, input.a);
			output.a = input.a + tex2_color.a;
			break;
		}
		case M2_FRAGMENT_DECAL_MOD2X:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output.rgb = tex2_color.rgb * mix(input.rgb, tex1_color.rgb, input.a);
			output.a = input.a * tex2_color.a;
			output = output * 2;
			break;
		}
		case M2_FRAGMENT_DECAL_FADE:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			vec3 tmp = (input.rgb - tex1_color.rgb) * input.a + tex1_color.rgb;
			output.rgb = mix(tex2_color.rgb, tmp, input.a);
			output.a = input.a;
			break;
		}
		case M2_FRAGMENT_ADD_OPAQUE:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output.rgb = (input.rgb + tex1_color.rgb) * tex2_color.rgb;
			output.a = input.a + tex1_color.a;
			break;
		}
		case M2_FRAGMENT_ADD_MOD:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output = (input + tex1_color) * tex2_color;
			break;
		}
		case M2_FRAGMENT_ADD_DECAL:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output.rgb = (input.rgb + tex1_color.rgb - tex2_color.rgb) * tex1_color.a + tex2_color.rgb;
			output.a = input.a + tex1_color.a;
			break;
		}
		case M2_FRAGMENT_ADD_ADD:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output = input + tex1_color + tex2_color;
			break;
		}
		case M2_FRAGMENT_ADD_MOD2X:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output = (input + tex1_color) * tex2_color * 2;
			break;
		}
		case M2_FRAGMENT_ADD_FADE:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			vec3 tmp = input.rgb + tex1_color.rgb;
			output.rgb = mix(tex2_color.rgb, tmp, input.a);
			output.a = input.a + tex1_color.a;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_OPAQUE_2X:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output.rgb = input.rgb * tex1_color.rgb * tex2_color.rgb * 2;
			output.a = input.a * tex1_color.a;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_DECAL_2X:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			vec4 tmp = input * tex1_color * 2;
			output.rgb = mix(tmp.rgb, tex2_color.rgb, tmp.a);
			output.a = tmp.a;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_ADD_2X:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output = input * tex1_color * 2 + tex2_color;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_4X:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			output = input * tex1_color * tex2_color * 4;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_FADE_2X:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			vec3 tmp = input.rgb * tex1_color.rgb * 2;
			output.rgb = mix(tex2_color.rgb, tmp, input.a);
			output.a = input.a * tex1_color.a;
			break;
		}
		default:
			output = vec4(0, 1, 0, 1);
			break;
	}
	if (output.a < alpha_test)
		discard;
	if (settings.x == 0)
	{
		float fog_factor = clamp((length(fs_position) - fog_range.x) / (fog_range.y - fog_range.x), 0, 1);
		output = vec4(mix(output.rgb, fog_color, fog_factor), output.a);
	}
	output = clamp(output, 0, 1);
	fragcolor = output;
	fragnormal = vec4(fs_normal, 1);
	fragposition = vec4(fs_position, 1);
}
