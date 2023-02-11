#version 330

in fs_block
{
	vec3 fs_position;
	vec3 fs_diffuse;
	vec3 fs_normal;
	vec2 fs_uv;
};

uniform sampler2D tex;

layout(std140) uniform scene_block
{
	vec4 diffuse_color;
	vec4 fog_color;
	vec2 fog_range;
};

layout(location=0) out vec4 fragcolor;
layout(location=1) out vec4 fragnormal;
layout(location=2) out vec4 fragposition;

void main()
{
	vec3 color = texture(tex, fs_uv).rgb;
	float fog_factor = clamp((length(fs_position) - fog_range.x) / (fog_range.y - fog_range.x), 0, 1);
	fragcolor = vec4(mix(color, fog_color.xyz, fog_factor), 1);
	fragnormal = vec4(fs_normal, 1);
	fragposition = vec4(fs_position, 1);
}
