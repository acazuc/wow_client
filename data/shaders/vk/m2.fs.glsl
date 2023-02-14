#version 450

layout(location = 0) in fs_block
{
	vec4 fs_position_fixed;
	vec4 fs_normal_fixed;
	vec3 fs_position;
	vec3 fs_diffuse;
	vec3 fs_normal;
	vec2 fs_uv1;
	vec2 fs_uv2;
};

layout(set = 1, binding = 0) uniform sampler2D tex1;
layout(set = 1, binding = 1) uniform sampler2D tex2;

struct Light
{
	vec4 ambient;
	vec4 diffuse;
	vec4 position;
	vec2 attenuation;
	vec2 data;
};

layout(set = 0, binding = 0, std140) uniform mesh_block
{
	mat4 tex1_matrix;
	mat4 tex2_matrix;
	ivec4 combiners;
	ivec4 settings;
	vec4 color;
	vec3 fog_color;
	float alpha_test;
};

layout(set = 0, binding = 1, std140) uniform model_block
{
	mat4 v;
	mat4 mv;
	mat4 mvp;
	ivec4 lights_count;
	Light lights[4];
	mat4 bone_mats[256];
};

layout(set = 0, binding = 2, std140) uniform scene_block
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
	vec4 color_input = vec4(fs_diffuse, 1);
	color_input *= color;
	vec4 tex1_color = texture(tex1, fs_uv1);
	vec4 color_output;
	switch (combiners.y)
	{
		case M2_FRAGMENT_OPAQUE:
			color_output.rgb = color_input.rgb * tex1_color.rgb;
			color_output.a = color_input.a;
			break;
		case M2_FRAGMENT_DIFFUSE:
			color_output = color_input * tex1_color;
			break;
		case M2_FRAGMENT_DECAL:
			color_output.rgb = mix(color_input.rgb, tex1_color.rgb, color_input.a);
			color_output.a = color_input.a;
			break;
		case M2_FRAGMENT_ADD:
			color_output = color_input + tex1_color;
			break;
		case M2_FRAGMENT_DIFFUSE2X:
			color_output = color_input * tex1_color * 2;
			break;
		case M2_FRAGMENT_FADE:
			color_output.rgb = mix(tex1_color.rgb, color_input.rgb, color_input.a);
			color_output.a = color_input.a;
			break;
		case M2_FRAGMENT_OPAQUE_OPAQUE:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output.rgb = color_input.rgb * tex1_color.rgb * tex2_color.rgb;
			color_output.a = color_input.a;
			break;
		}
		case M2_FRAGMENT_OPAQUE_MOD:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output.rgb = color_input.rgb * tex1_color.rgb * tex2_color.rgb;
			color_output.a = color_input.a * tex2_color.a;
			break;
		}
		case M2_FRAGMENT_OPAQUE_DECAL:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output.rgb = (color_input.rgb * tex1_color.rgb - tex2_color.rgb) * tex1_color.a + tex2_color.rgb;
			color_output.a = color_input.a;
			break;
		}
		case M2_FRAGMENT_OPAQUE_ADD:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output.rgb = color_input.rgb * tex1_color.rgb + tex2_color.rgb;
			color_output.a = color_input.a * tex2_color.a;
			break;
		}
		case M2_FRAGMENT_OPAQUE_MOD2X:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output.rgb = color_input.rgb * tex1_color.rgb * tex2_color.rgb * 2;
			color_output.a = color_input.a * tex2_color.a * 2;
			break;
		}
		case M2_FRAGMENT_OPAQUE_FADE:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output.rgb = (tex2_color.rgb - tex1_color.rgb * color_input.rgb) * color_input.a + tex1_color.rgb;
			color_output.a = color_input.a;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_OPAQUE:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output.rgb = color_input.rgb * tex1_color.rgb * tex2_color.rgb;
			color_output.a = color_input.a * tex1_color.a;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output = color_input * tex1_color * tex2_color;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_DECAL:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			vec4 tmp = color_input * tex1_color;
			tmp.rgb -= tex2_color.rgb;
			color_output.rgb = tmp.rgb * tmp.a + tex2_color.rgb;
			color_output.a = tmp.a;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_ADD:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output = color_input * tex1_color + tex2_color;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_2X:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output = color_input * tex1_color * tex2_color * 2;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_FADE:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			vec4 tmp = tex1_color * color_input;
			color_output.rgb = mix(tex2_color.rgb, tmp.rgb, color_input.a);
			color_output.a = tmp.a;
			break;
		}
		case M2_FRAGMENT_DECAL_OPAQUE:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output.rgb = mix(color_input.rgb, tex1_color.rgb, color_input.a) * tex2_color.rgb;
			color_output.a = color_input.a;
			break;
		}
		case M2_FRAGMENT_DECAL_MOD:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output.rgb = ((color_input.rgb - tex1_color.rgb) * color_input.a + tex1_color.rgb) * tex2_color.rgb;
			color_output.a = color_input.a * tex2_color.a;
			break;
		}
		case M2_FRAGMENT_DECAL_DECAL:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output.rgb = tex1_color.a * (mix(color_input.rgb, tex1_color.rgb, color_input.a) - tex2_color.rgb) * tex2_color.rgb;
			color_output.a = color_input.a;
			break;
		}
		case M2_FRAGMENT_DECAL_ADD:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output.rgb = tex2_color.rgb + mix(color_input.rgb, tex1_color.rgb, color_input.a);
			color_output.a = color_input.a + tex2_color.a;
			break;
		}
		case M2_FRAGMENT_DECAL_MOD2X:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output.rgb = tex2_color.rgb * mix(color_input.rgb, tex1_color.rgb, color_input.a);
			color_output.a = color_input.a * tex2_color.a;
			color_output = color_output * 2;
			break;
		}
		case M2_FRAGMENT_DECAL_FADE:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			vec3 tmp = (color_input.rgb - tex1_color.rgb) * color_input.a + tex1_color.rgb;
			color_output.rgb = mix(tex2_color.rgb, tmp, color_input.a);
			color_output.a = color_input.a;
			break;
		}
		case M2_FRAGMENT_ADD_OPAQUE:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output.rgb = (color_input.rgb + tex1_color.rgb) * tex2_color.rgb;
			color_output.a = color_input.a + tex1_color.a;
			break;
		}
		case M2_FRAGMENT_ADD_MOD:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output = (color_input + tex1_color) * tex2_color;
			break;
		}
		case M2_FRAGMENT_ADD_DECAL:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output.rgb = (color_input.rgb + tex1_color.rgb - tex2_color.rgb) * tex1_color.a + tex2_color.rgb;
			color_output.a = color_input.a + tex1_color.a;
			break;
		}
		case M2_FRAGMENT_ADD_ADD:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output = color_input + tex1_color + tex2_color;
			break;
		}
		case M2_FRAGMENT_ADD_MOD2X:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output = (color_input + tex1_color) * tex2_color * 2;
			break;
		}
		case M2_FRAGMENT_ADD_FADE:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			vec3 tmp = color_input.rgb + tex1_color.rgb;
			color_output.rgb = mix(tex2_color.rgb, tmp, color_input.a);
			color_output.a = color_input.a + tex1_color.a;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_OPAQUE_2X:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output.rgb = color_input.rgb * tex1_color.rgb * tex2_color.rgb * 2;
			color_output.a = color_input.a * tex1_color.a;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_DECAL_2X:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			vec4 tmp = color_input * tex1_color * 2;
			color_output.rgb = mix(tmp.rgb, tex2_color.rgb, tmp.a);
			color_output.a = tmp.a;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_ADD_2X:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output = color_input * tex1_color * 2 + tex2_color;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_4X:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			color_output = color_input * tex1_color * tex2_color * 4;
			break;
		}
		case M2_FRAGMENT_DIFFUSE_2TEX_FADE_2X:
		{
			vec4 tex2_color = texture(tex2, fs_uv2);
			vec3 tmp = color_input.rgb * tex1_color.rgb * 2;
			color_output.rgb = mix(tex2_color.rgb, tmp, color_input.a);
			color_output.a = color_input.a * tex1_color.a;
			break;
		}
		default:
			color_output = vec4(0, 1, 0, 1);
			break;
	}
	if (color_output.a < alpha_test)
		discard;
	if (settings.x == 0)
	{
		float fog_factor = clamp((length(fs_position) - fog_range.x) / (fog_range.y - fog_range.x), 0, 1);
		color_output.rgb = mix(color_output.rgb, fog_color, fog_factor);
	}
	color_output = clamp(color_output, 0, 1);
	fragcolor = color_output;
	fragnormal = vec4(fs_normal, 1);
	fragposition = vec4(fs_position, 1);
}
