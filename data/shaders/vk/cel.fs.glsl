#version 450

layout(location = 0) in fs_block
{
	vec2 fs_uv;
};

layout(location=0) out vec4 fragcolor;
layout(location=1) out vec4 fragnormal;
layout(location=2) out vec4 fragposition;

layout(set = 1, binding = 0) uniform sampler2D color_tex;
layout(set = 1, binding = 1) uniform sampler2D normal_tex;
layout(set = 1, binding = 2) uniform sampler2D position_tex;

layout(set = 0, binding = 1, std140) uniform model_block
{
	mat4 mvp;
	float cel;
};

void main()
{
	vec4 texture_color = texture(color_tex, fs_uv);
	float pwr = length(texture_color.xyz);
	float fac;
	if (pwr > 0)
		fac = (pwr + cel * .5 - mod(pwr, cel)) / pwr;
	else
		fac = 1;
	fragcolor = vec4((texture_color.xyz * fac) + cel * .5, 1);
	fragnormal = texture(normal_tex, fs_uv);
	fragposition = texture(position_tex, fs_uv);
}
