#version 330

in fs_block
{
	vec4 fs_col;
	vec4 fs_uv;
};

layout(std140) uniform model_block
{
	mat4 mvp;
	vec4 color;
	vec4 uv_transform;
	float alpha_test;
	int use_mask;
};

uniform sampler2D tex;
uniform sampler2D mask;

layout(location=0) out vec4 fragcolor;

void main()
{
	vec4 c = fs_col * texture(tex, fs_uv.xy);
	if (use_mask != 0)
		c *= texture(mask, fs_uv.zw);
	if (c.a < alpha_test)
		discard;
	fragcolor = c;
}
