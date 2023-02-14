#version 450

layout(location = 0) in fs_block
{
	vec4 fs_position;
	vec2 fs_uv;
};

layout(set = 0, binding = 1, std140) uniform model_block
{
	mat4 mvp;
	vec4 color;
};

layout(set = 1, binding = 0) uniform sampler2D tex;

layout(location=0) out vec4 fragcolor;
layout(location=1) out vec4 fragnormal;
layout(location=2) out vec4 fragposition;

const float threshold = .5;

void main()
{
	float alpha = texture(tex, fs_uv).r;
	fragcolor = color * vec4(vec3(1), alpha);
	fragnormal = vec4(0, 0, 1, 1);
	fragposition = fs_position;
}
