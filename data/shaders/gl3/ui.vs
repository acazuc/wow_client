#version 330

layout(location=0) in vec2 vs_position;
layout(location=1) in vec4 vs_color;
layout(location=2) in vec2 vs_uv;

out fs_block
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

void main()
{
	gl_Position = mvp * vec4(vs_position, 0, 1);
	fs_col = vs_color * color;
	fs_uv = vec4(vs_uv.x * uv_transform.x + uv_transform.y,
	             vs_uv.y * uv_transform.z + uv_transform.w,
	             vs_uv.x,
	             vs_uv.y);
}
