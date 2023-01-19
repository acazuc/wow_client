#version 330

layout(location=0) in vec3 vs_position;
layout(location=1) in float vs_color0;
layout(location=2) in float vs_color1;
layout(location=3) in float vs_color2;
layout(location=4) in float vs_color3;
layout(location=5) in float vs_color4;

out fs_block
{
	vec4 fs_color;
};

layout (std140) uniform model_block
{
	mat4 mvp;
	vec4 colors[6];
};

void main()
{
	gl_Position = mvp * vec4(vs_position, 1);
	vec4 ret = colors[0];
	ret = mix(ret, colors[1], vs_color0);
	ret = mix(ret, colors[2], vs_color1);
	ret = mix(ret, colors[3], vs_color2);
	ret = mix(ret, colors[4], vs_color3);
	ret = mix(ret, colors[5], vs_color4);
	fs_color = ret;
}
