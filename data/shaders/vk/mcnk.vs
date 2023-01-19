#version 330

layout(location=0) in vec3 vs_normal;
layout(location=1) in vec2 vs_xz;
layout(location=2) in vec2 vs_uv;
layout(location=3) in float vs_y;

out fs_block
{
	vec3 fs_position;
	vec3 fs_light_dir;
	vec3 fs_normal;
	vec2 fs_uv0;
	vec2 fs_uv1;
	vec2 fs_uv2;
	vec2 fs_uv3;
	vec2 fs_uv4;
	flat int fs_id;
};

struct mesh_block_st
{
	vec2 uv_offset0;
	vec2 uv_offset1;
	vec2 uv_offset2;
	vec2 uv_offset3;
};

layout(std140) uniform mesh_block
{
	mesh_block_st mesh_blocks[256];
};

layout(std140) uniform model_block
{
	mat4 v;
	mat4 mv;
	mat4 mvp;
	float offset_time;
};

layout(std140) uniform scene_block
{
	vec4 light_direction;
	vec4 ambient_color;
	vec4 diffuse_color;
	vec4 specular_color;
	vec4 fog_color;
	vec2 fog_range;
};

void main()
{
	vec4 position_fixed = vec4(vs_xz.x, vs_y, vs_xz.y, 1);
	vec4 normal_fixed = vec4(vs_normal, 0);
	gl_Position =  mvp * position_fixed;
	fs_position = (mv * position_fixed).xyz;
	fs_normal = normalize((mv * normal_fixed).xyz);
	fs_light_dir = normalize((v * -light_direction).xyz);
	fs_uv0 = vs_uv;
	int offset = gl_VertexID / 145;
	fs_uv1 = -(vs_uv + mesh_blocks[offset].uv_offset0 * offset_time).yx * 8;
	fs_uv2 = -(vs_uv + mesh_blocks[offset].uv_offset1 * offset_time).yx * 8;
	fs_uv3 = -(vs_uv + mesh_blocks[offset].uv_offset2 * offset_time).yx * 8;
	fs_uv4 = -(vs_uv + mesh_blocks[offset].uv_offset3 * offset_time).yx * 8;
	fs_id = gl_VertexID;
}
