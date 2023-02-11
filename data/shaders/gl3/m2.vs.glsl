#version 330

layout(location=0) in vec4 vs_bone_weights;
layout(location=1) in vec3 vs_position;
layout(location=2) in vec3 vs_normal;
layout(location=3) in ivec4 vs_bones;
layout(location=4) in vec2 vs_uv1;
layout(location=5) in vec2 vs_uv2;

out fs_block
{
	vec4 fs_position_fixed;
	vec4 fs_normal_fixed;
	vec3 fs_position;
	vec3 fs_diffuse;
	vec3 fs_normal;
	vec2 fs_uv1;
	vec2 fs_uv2;
};

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

#define M2_VERTEX_DIFFUSE                   0x1
#define M2_VERTEX_DIFFUSE_ENV_MAP           0x2
#define M2_VERTEX_DIFFUSE_2TEX              0x3
#define M2_VERTEX_DIFFUSE_ENV_MAP0_2TEX     0x4
#define M2_VERTEX_DIFFUSE_ENV_MAP_2TEX      0x5
#define M2_VERTEX_DIFFUSE_DUAL_ENV_MAP_2TEX 0x6

vec2 diffuse_coord(vec2 uv, mat4 matrix)
{
	return (matrix * vec4(uv - .5, 0, 1)).xy;
}

vec2 env_coord(vec3 position, vec3 normal)
{
	return .5 + .5 * reflect(normalize(position), normal).xy;
}

void main()
{
	fs_position_fixed = vec4(vs_position.xyz, 1);
	fs_normal_fixed = vec4(vs_normal.xyz, 0);
	if (settings.z != 0)
	{
		mat4 bone_mat = vs_bone_weights.x * bone_mats[vs_bones.x] + vs_bone_weights.y * bone_mats[vs_bones.y] + vs_bone_weights.z * bone_mats[vs_bones.z] + vs_bone_weights.w * bone_mats[vs_bones.w];
		fs_position_fixed = bone_mat * fs_position_fixed;
		fs_normal_fixed = bone_mat * fs_normal_fixed;
	}
	fs_normal_fixed = normalize(fs_normal_fixed);
	gl_Position = mvp * fs_position_fixed;
	fs_position = (mv * fs_position_fixed).xyz;
	fs_normal = normalize((mv * fs_normal_fixed).xyz);
	switch (combiners.x)
	{
		case M2_VERTEX_DIFFUSE:
			fs_uv1 = diffuse_coord(vs_uv1, tex1_matrix);
			break;
		case M2_VERTEX_DIFFUSE_ENV_MAP:
			fs_uv1 = env_coord(fs_position, fs_normal);
			break;
		case M2_VERTEX_DIFFUSE_2TEX:
			fs_uv1 = diffuse_coord(vs_uv1, tex1_matrix);
			fs_uv2 = diffuse_coord(vs_uv2, tex2_matrix);
			break;
		case M2_VERTEX_DIFFUSE_ENV_MAP0_2TEX:
			fs_uv1 = env_coord(fs_position, fs_normal);
			fs_uv2 = diffuse_coord(vs_uv2, tex2_matrix);
			break;
		case M2_VERTEX_DIFFUSE_ENV_MAP_2TEX:
			fs_uv1 = diffuse_coord(vs_uv1, tex1_matrix);
			fs_uv2 = env_coord(fs_position, fs_normal);
			break;
		case M2_VERTEX_DIFFUSE_DUAL_ENV_MAP_2TEX:
			fs_uv1 = env_coord(fs_position, fs_normal);
			fs_uv2 = fs_uv1;
			break;
		default:
			fs_uv1 = diffuse_coord(vs_uv1, tex1_matrix);
			fs_uv2 = diffuse_coord(vs_uv2, tex2_matrix);
			break;
	}
	if (settings.y == 0)
	{
		vec3 light_dir = normalize((v * -light_direction).xyz);
		vec3 input = diffuse_color.xyz * clamp(dot(fs_normal, light_dir), 0, 1) + ambient_color.xyz;
		for (int i = 0; i < lights_count.x; ++i)
		{
			float diffuse_factor;
			if (lights[i].data.y == 0)
			{
				vec3 light_dir = lights[i].position.xyz;
				diffuse_factor = clamp(dot(fs_normal_fixed.xyz, normalize(light_dir)), 0, 1);
			}
			else
			{
				vec3 light_dir = lights[i].position.xyz - fs_position_fixed.xyz;
				diffuse_factor = clamp(dot(fs_normal_fixed.xyz, normalize(light_dir)), 0, 1);
				float len = length(light_dir);
				len *= 0.817102;
				diffuse_factor /= len * (0.7 + len * 0.03);
			}
			input += lights[i].ambient.rgb * lights[i].ambient.a + lights[i].diffuse.rgb * lights[i].diffuse.a * diffuse_factor * lights[i].data.x;
		}
		fs_diffuse = clamp(input, 0, 1);
	}
	else
	{
		fs_diffuse = vec3(1);
	}
}
