#version 450

layout(location = 0) in fs_block
{
	vec2 fs_uv;
};

layout(location=0) out vec4 fragcolor;

layout(set = 1, binding = 0) uniform sampler2D color_tex;

layout(set = 0, binding = 1, std140) uniform model_block
{
	mat4 mvp;
	int horizontal;
};

#define WEIGHTS_NB 21

const float weights[WEIGHTS_NB] = float[] (0.079, 0.079, 0.078, 0.076, 0.073, 0.07, 0.066, 0.062, 0.058, 0.053, 0.048, 0.043, 0.038, 0.034, 0.029, 0.025, 0.022, 0.018, 0.015, 0.013, 0.01); 

void main()
{
	vec2 tex_offset = 1. / textureSize(color_tex, 0);
	vec3 result = texture(color_tex, fs_uv).xyz * weights[0];
	vec2 offset;
	if (horizontal != 0)
	{
		offset = vec2(tex_offset.x, 0);
	}
	else
	{
		offset = vec2(0, tex_offset.y);
	}
	for (int i = 1; i < WEIGHTS_NB; ++i)
	{
		result += texture(color_tex, fs_uv - offset * i).xyz * weights[i];
		result += texture(color_tex, fs_uv + offset * i).xyz * weights[i];
	}
	fragcolor = vec4(result, 1);
}
