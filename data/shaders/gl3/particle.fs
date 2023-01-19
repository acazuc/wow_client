#version 330

in fs_block
{
	vec4 fs_color;
	vec2 fs_uv;
};

layout(std140) uniform model_block
{
	float alpha_test;
};

layout(std140) uniform scene_block
{
	mat4 vp;
};

uniform sampler2D tex;

layout(location=0) out vec4 fragcolor;

void main()
{
#if 0
	fragcolor = vec4(0, 1, 0, .2);
	return;
#endif
	vec4 tex_color = texture(tex, fs_uv);
	vec4 col = fs_color.bgra * tex_color;
	if (col.a < alpha_test)
		discard;
	fragcolor = col;
}
