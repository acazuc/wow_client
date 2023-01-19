#version 330

in fs_block
{
	vec4 fs_col;
	vec2 fs_uv;
};

layout(std140) uniform model_block
{
	mat4 mvp;
	vec4 color;
	float alpha_test;
};

uniform sampler2D tex;

layout(location=0) out vec4 fragcolor;

void main()
{
	vec4 c = fs_col * texture(tex, fs_uv);
	if (c.a < alpha_test)
		discard;
	fragcolor = c;
}
