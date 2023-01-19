#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in gs_block
{
	vec4 color;
	vec4 uv;
	mat2 matrix;
	float scale;
} gs_input[1];

out fs_block
{
	vec4 fs_color;
	vec2 fs_uv;
};

const float size = 0.5;

void main()
{
	gl_Position = gl_in[0].gl_Position + vec4(gs_input[0].matrix * vec2(gs_input[0].scale, gs_input[0].scale), 0, 0);
	fs_color = gs_input[0].color;
	fs_uv = vec2(gs_input[0].uv.z, gs_input[0].uv.w);
	EmitVertex();
	gl_Position = gl_in[0].gl_Position + vec4(gs_input[0].matrix * vec2(gs_input[0].scale, -gs_input[0].scale), 0, 0);
	fs_color = gs_input[0].color;
	fs_uv = vec2(gs_input[0].uv.z, gs_input[0].uv.y);
	EmitVertex();
	gl_Position = gl_in[0].gl_Position + vec4(gs_input[0].matrix * vec2(-gs_input[0].scale, gs_input[0].scale), 0, 0);
	fs_color = gs_input[0].color;
	fs_uv = vec2(gs_input[0].uv.x, gs_input[0].uv.w);
	EmitVertex();
	gl_Position = gl_in[0].gl_Position + vec4(gs_input[0].matrix * vec2(-gs_input[0].scale, -gs_input[0].scale), 0, 0);
	fs_color = gs_input[0].color;
	fs_uv = vec2(gs_input[0].uv.x, gs_input[0].uv.y);
	EmitVertex();
	EndPrimitive();
}
