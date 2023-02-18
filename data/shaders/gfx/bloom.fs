in 0 vec2 uv

out 0 vec4 color

sampler 0 color_tex

constant 1 model_block
{
	mat4 mvp
	float threshold
}

fs_output gfx_main(fs_input gfx_input)
{
	fs_output gfx_output;
	vec3 texture_color = gfx_sampler(color_tex, gfx_input.uv).xyz;
	float brightness = dot(texture_color, vec3(0.2126, 0.7152, 0.0722));
	if (brightness >= threshold)
		fragcolor = vec4(texture_color, 1);
	else
		fragcolor = vec4(0);
	return gfx_output;
}
