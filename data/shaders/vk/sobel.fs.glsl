#version 450

layout(location = 0) in fs_block
{
	vec2 fs_uv;
};

layout(location=0) out vec4 fragcolor;
layout(location=1) out vec4 fragnormal;
layout(location=2) out vec4 fragposition;

layout(set = 1, binding = 0) uniform sampler2D color_tex;
layout(set = 1, binding = 1) uniform sampler2D normal_tex;
layout(set = 1, binding = 2) uniform sampler2D position_tex;

mat3 sx = mat3
(
	 1.0,  2.0,  1.0,
	 0.0,  0.0,  0.0,
	-1.0, -2.0, -1.0
);
mat3 sy = mat3
(
	1.0, 0.0, -1.0,
	2.0, 0.0, -2.0,
	1.0, 0.0, -1.0
);

float sobelPosition(vec2 uv)
{
	vec3 samples[3];
	samples[0] = vec3(
			textureOffset(position_tex, uv, ivec2(-1, -1)).z,
			textureOffset(position_tex, uv, ivec2(-1,  0)).z,
			textureOffset(position_tex, uv, ivec2(-1,  1)).z);
	samples[1] = vec3(
			textureOffset(position_tex, uv, ivec2( 0, -1)).z,
			0,
			textureOffset(position_tex, uv, ivec2( 0,  1)).z);
	samples[2] = vec3(
			textureOffset(position_tex, uv, ivec2( 1, -1)).z,
			textureOffset(position_tex, uv, ivec2( 1,  0)).z,
			textureOffset(position_tex, uv, ivec2( 1,  1)).z);
	/*float divisor = 100;
	samples[0] /= divisor;
	samples[1] /= divisor;
	samples[2] /= divisor;*/
	float gx = dot(sx[0], samples[0]) + dot(sx[1], samples[1]) + dot(sx[2], samples[2]);
	float gy = dot(sy[0], samples[0]) + dot(sy[1], samples[1]) + dot(sy[2], samples[2]);
	return clamp(sqrt(gx * gx + gy * gy), 0, 1);
}

float sobelNormal(vec2 uv)
{
	vec4 normal = texture(normal_tex, uv);
	vec3 samples[3];
	samples[0] = vec3(
			dot(normal.xyz, textureOffset(normal_tex, uv, ivec2(-1, -1)).xyz),
			dot(normal.xyz, textureOffset(normal_tex, uv, ivec2(-1,  0)).xyz),
			dot(normal.xyz, textureOffset(normal_tex, uv, ivec2(-1,  1)).xyz));
	samples[1] = vec3(
			dot(normal.xyz, textureOffset(normal_tex, uv, ivec2( 0, -1)).xyz),
			0,
			dot(normal.xyz, textureOffset(normal_tex, uv, ivec2( 0,  1)).xyz));
	samples[2] = vec3(
			dot(normal.xyz, textureOffset(normal_tex, uv, ivec2( 1, -1)).xyz),
			dot(normal.xyz, textureOffset(normal_tex, uv, ivec2( 1,  0)).xyz),
			dot(normal.xyz, textureOffset(normal_tex, uv, ivec2( 1,  1)).xyz));
	float gx = dot(sx[0], samples[0]) + dot(sx[1], samples[1]) + dot(sx[2], samples[2]);
	float gy = dot(sy[0], samples[0]) + dot(sy[1], samples[1]) + dot(sy[2], samples[2]);
	return clamp(sqrt(gx * gx + gy * gy), 0, 1);
}

void main()
{
	float sobel = sobelNormal(fs_uv);
	//float sobel = sobelPosition(fs_uv);
	fragcolor = vec4(vec3(texture(color_tex, fs_uv) * (1 - sobel)), 1);
	fragnormal = texture(normal_tex, fs_uv);
	fragposition = texture(position_tex, fs_uv);
}
