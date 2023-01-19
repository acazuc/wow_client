#version 330

layout(location=0) in vec4 vs_position;
layout(location=1) in vec4 vs_color;
layout(location=2) in vec4 vs_uv;
layout(location=3) in vec4 vs_matrix;
layout(location=4) in float vs_scale;

out gs_block
{
	vec4 color;
	vec4 uv;
	mat2 matrix;
	float scale;
} gs_input;

layout(std140) uniform model_block
{
	float alpha_test;
};

layout(std140) uniform scene_block
{
	mat4 vp;
};

void main()
{
	gl_Position = vp * vs_position;
	gs_input.color = vs_color;
	gs_input.uv = vs_uv;
	gs_input.scale = vs_scale;
	gs_input.matrix[0][0] = vs_matrix.x;
	gs_input.matrix[0][1] = vs_matrix.y;
	gs_input.matrix[1][0] = vs_matrix.z;
	gs_input.matrix[1][1] = vs_matrix.w;
}
