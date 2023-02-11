#version 330

in fs_block
{
	vec2 fs_uv;
};

layout(location=0) out vec4 fragcolor;
layout(location=1) out vec4 fragnormal;
layout(location=2) out vec4 fragposition;

uniform sampler2D color_tex;
uniform sampler2D normal_tex;
uniform sampler2D position_tex;

layout(std140) uniform model_block
{
	mat4 mvp;
};

void main()
{
	fragcolor = vec4(texture(color_tex, fs_uv).rgb, 1);
	fragnormal = texture(normal_tex, fs_uv);
	fragposition = texture(position_tex, fs_uv);
}
