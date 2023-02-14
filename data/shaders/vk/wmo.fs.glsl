#version 450

layout(location = 0) in fs_block
{
	vec3 fs_position;
	vec3 fs_light_dir;
	vec3 fs_normal;
	vec4 fs_color;
	vec3 fs_light;
	vec2 fs_uv1;
	vec2 fs_uv2;
};

layout(set = 1, binding = 0) uniform sampler2D tex1;

layout(set = 0, binding = 0, std140) uniform mesh_block
{
	vec4 emissive_color;
	ivec4 combiners;
	ivec4 settings;
	float alpha_test;
};

layout(set = 0, binding = 1, std140) uniform model_block
{
	mat4 v;
	mat4 mv;
	mat4 mvp;
};

layout(set = 0, binding = 2, std140) uniform scene_block
{
	vec4 light_direction;
	vec4 specular_color;
	vec4 diffuse_color;
	vec4 ambient_color;
	vec4 fog_color;
	vec2 fog_range;
};

layout(location=0) out vec4 fragcolor;
layout(location=1) out vec4 fragnormal;
layout(location=2) out vec4 fragposition;

void main()
{
	vec4 input_color = vec4(mix(fs_color.xyz, fs_light, fs_color.a), 1);
	vec4 tex1_color = texture(tex1, fs_uv1);
	vec4 output_color = input_color;
	switch (combiners.y)
	{
		case 0: //Diffuse
		{
			output_color = tex1_color * input_color;
			//output_color = vec4(.25, .25, .25, 1) * vec4(vec3(length(input_color.rgb)), 1);
			break;
		}
		case 1: //Specular
		{
			float specular_factor = pow(clamp(dot(normalize(-fs_position), normalize(reflect(-fs_light_dir, fs_normal))), 0, 1), 10);
			output_color = tex1_color * input_color;
			output_color.rgb += vec3(specular_factor * specular_color);
			//output_color = vec4(1, 1, 1, 1) * vec4(vec3(length(input_color.rgb)), 1);
			break;
		}
		case 2: //Metal
		{
			float specular_factor = pow(clamp(dot(normalize(-fs_position), normalize(reflect(-fs_light_dir, fs_normal))), 0, 1), 10);
			output_color = tex1_color * input_color;
			output_color.rgb += vec3(specular_factor * specular_color * tex1_color.a * 4);
			//output_color = vec4(0, 1, 1, 1) * vec4(vec3(length(input_color.rgb)), 1);
			break;
		}
		case 3: //Env
		{
			//2 textures
			output_color = tex1_color * input_color;
			//output_color = vec4(0, 0, 1, 1) * vec4(vec3(length(input_color.rgb)), 1);
			break;
		}
		case 4: //Opaque
		{
			output_color.rgb = input_color.rgb * tex1_color.rgb;
			//output_color = vec4(1, 0, 1, 1) * vec4(vec3(length(input_color.rgb)), 1);
			break;
		}
		case 5: //EnvMetal
		{
			//2 textures
			float specular_factor = pow(clamp(dot(normalize(-fs_position), normalize(reflect(-fs_light_dir, fs_normal))), 0, 1), 10);
			output_color = tex1_color * input_color + vec4(vec3(specular_factor * specular_color.xyz * tex1_color.rgb), 1);
			//output_color = vec4(1, 1, 0, 1) * vec4(vec3(length(input_color.rgb)), 1);
			break;
		}
		case 6: //MapObjTwoLayerDiffuse
		{
			output_color = vec4(1, 0, 0, 1) * vec4(vec3(length(input_color.rgb)), 1);
			break;
		}
		default:
		{
			output_color = vec4(0, 1, 0, 1) * vec4(vec3(length(input_color.rgb)), 1);
			break;
		}
	}
	if (output_color.a < alpha_test)
		discard;
	if (settings.z == 0)
	{
		float fog_factor = clamp((length(fs_position) - fog_range.x) / (fog_range.y - fog_range.x), 0, 1);
		output_color = vec4(mix(output_color.rgb, fog_color.xyz, fog_factor), output_color.a);
	}
	fragcolor = output_color;
	fragnormal = vec4(fs_normal, 1);
	fragposition = vec4(fs_position, 1);
}
