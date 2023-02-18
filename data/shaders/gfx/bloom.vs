in 0 vec2 position
in 1 vec2 uv

out 0 vec2 uv

constant 1 model_block
{
	mat4 mvp
	float threshold
}

vs_output gfx_main(vs_input gfx_input)
{
	vs_output gfx_output;
	gfx_output.gfx_position = mul(vec4(gfx_input.position, 0, 1), mvp);
	gfx_output.uv = gfx_input.uv;
	return gfx_output;
}
