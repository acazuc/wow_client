#version 330

layout(location=0) in vec3 vs_position;
layout(location=1) in vec4 vs_uv;

out fs_block
{
	vec3 fs_position;
	vec3 fs_light_dir;
	vec3 fs_diffuse;
	vec3 fs_normal;
	vec2 fs_uv;
};

layout(std140) uniform mesh_block
{
	float alpha;
	int type;
};

layout(std140) uniform model_block
{
	mat4 mvp;
	mat4 mv;
	mat4 v;
};

layout(std140) uniform scene_block
{
	vec4 light_direction;
	vec4 specular_color;
	vec4 diffuse_color;
	vec4 final_color;
	vec4 base_color;
	vec4 fog_color;
	vec2 fog_range;
};

void main()
{
	vec4 position_fixed = vec4(vs_position, 1);
	gl_Position = mvp * position_fixed;
	fs_position = (mv * position_fixed).xyz;
	fs_normal = normalize((mv * vec4(0, 1, 0, 0)).xyz);
	if (type <= 1 || type == 4 || type == 8)
	{
		fs_light_dir = normalize((v * -light_direction).xyz);
		//fs_diffuse = mix(base_color, final_color, min(1, vs_depth * 5));
		//fs_diffuse = base_color + final_color;
		fs_diffuse = vec3(.2, .2, .2);//vec3(clamp(dot(fs_normal, fs_light_dir), 0, 1));
		fs_uv = vs_uv.zw;
	}
	else
	{
		fs_uv = vs_uv.xy;
	}
}
