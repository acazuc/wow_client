#version 450

layout(location = 0) in fs_block
{
	vec3 fs_position;
	vec4 fs_color;
	vec2 fs_uv;
};

layout(set = 0, binding = 1, std140) uniform model_block
{
	mat4 mvp;
	mat4 mv;
	vec4 fog_color;
	float alpha_test;
};

layout(set = 0, binding = 2, std140) uniform scene_block
{
	vec2 fog_range;
};

layout(set = 1, binding = 0) uniform sampler2D tex;

layout(location=0) out vec4 fragcolor;

void main()
{
#if 0
	fragcolor = vec4(fs_uv, 1, .2);
	return;
#endif
	vec4 tex_color = texture(tex, fs_uv);
	vec4 color = fs_color.bgra * tex_color;
	if (color.a < alpha_test)
		discard;
	float fog_factor = clamp((length(fs_position) - fog_range.x) / (fog_range.y - fog_range.x), 0, 1);
	color.rgb = mix(color.rgb, fog_color.rgb, fog_factor);
	fragcolor = color;
}
