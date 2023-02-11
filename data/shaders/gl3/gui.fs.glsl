#version 330

in fs_block
{
	vec4 fs_color;
	vec2 fs_uv;
};

uniform sampler2D tex;

layout(location=0) out vec4 fragcolor;

void main()
{
	vec4 tex_col = texture(tex, fs_uv);
	vec4 col = fs_color * tex_col;
	if (col.a == 0)
		discard;
	fragcolor = col;
	//fragcolor = mix(vec4(1, 0, 0, 1), col, .5);
}
