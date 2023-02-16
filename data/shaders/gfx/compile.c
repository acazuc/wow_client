#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

enum shader_type
{
	SHADER_VERTEX,
	SHADER_FRAGMENT,
};

enum shader_api
{
	SHADER_GL3,
	SHADER_GL4,
	SHADER_GLES3,
	SHADER_VK,
	SHADER_D3D9,
	SHADER_D3D11,
};

enum shader_variable_type
{
	SHADER_FLOAT,
	SHADER_VEC1,
	SHADER_VEC2,
	SHADER_VEC3,
	SHADER_VEC4,
	SHADER_MAT1,
	SHADER_MAT2,
	SHADER_MAT3,
	SHADER_MAT4,
	SHADER_INT,
	SHADER_IVEC1,
	SHADER_IVEC2,
	SHADER_IVEC3,
	SHADER_IVEC4,
};

struct shader_constant_member
{
	enum shader_variable_type type;
	char name[64];
};

struct shader_constant
{
	uint8_t bind;
	char name[64];
	struct shader_constant_member members[64];
	uint32_t members_nb;
};

struct shader_sampler
{
	uint8_t bind;
	char name[64];
};

struct shader_output
{
	uint8_t bind;
	char name[64];
	enum shader_variable_type type;
};

struct shader_input
{
	uint8_t bind;
	char name[64];
	enum shader_variable_type type;
};

struct shader
{
	struct shader_constant constants[4];
	struct shader_sampler samplers[4];
	struct shader_output outputs[16];
	struct shader_input inputs[16];
	uint8_t constants_nb;
	uint8_t samplers_nb;
	uint8_t outputs_nb;
	uint8_t inputs_nb;
	char *code;
	size_t code_size;
	enum shader_type type;
};

static bool shader_type_from_string(const char *str, enum shader_type *type)
{
	static const char *strings[] =
	{
		[SHADER_VERTEX]   = "vs",
		[SHADER_FRAGMENT] = "fs",
	};
	for (size_t i = 0; i < sizeof(strings) / sizeof(*strings); ++i)
	{
		if (!strcmp(strings[i], str))
		{
			*type = i;
			return true;
		}
	}
	return false;
}

static bool shader_api_from_string(const char *str, enum shader_api *api)
{
	static const char *strings[] =
	{
		[SHADER_GL3] = "gl3",
		[SHADER_GL4] = "gl4",
		[SHADER_GLES3] = "gles3",
		[SHADER_VK] = "vk",
		[SHADER_D3D9] = "d3d9",
		[SHADER_D3D11] = "d3d11",
	};
	for (size_t i = 0; i < sizeof(strings) / sizeof(*strings); ++i)
	{
		if (!strcmp(strings[i], str))
		{
			*api = i;
			return true;
		}
	}
	return false;
}

static bool variable_type_from_string(const char *str, size_t len, enum shader_variable_type *type)
{
#define TEST(name, val) \
	if (!strncmp(str, #name, len)) \
	{ \
		*type = SHADER_##val; \
		return true; \
	}

	TEST(float, FLOAT);
	TEST(vec1, VEC1);
	TEST(vec2, VEC2);
	TEST(vec3, VEC3);
	TEST(vec4, VEC4);
	TEST(mat1, MAT1);
	TEST(mat2, MAT2);
	TEST(mat3, MAT3);
	TEST(mat4, MAT4);
	TEST(int, INT);
	TEST(ivec1, IVEC1);
	TEST(ivec2, IVEC2);
	TEST(ivec3, IVEC3);
	TEST(ivec4, IVEC4);
	return false;

#undef TEST
}

static const char *variable_type_to_string(enum shader_variable_type type)
{
	static const char *strings[] =
	{
		[SHADER_FLOAT] = "float",
		[SHADER_VEC1] = "vec1",
		[SHADER_VEC2] = "vec2",
		[SHADER_VEC3] = "vec3",
		[SHADER_VEC4] = "vec4",
		[SHADER_MAT1] = "mat1",
		[SHADER_MAT2] = "mat2",
		[SHADER_MAT3] = "mat3",
		[SHADER_MAT4] = "mat4",
		[SHADER_INT] = "int",
		[SHADER_IVEC1] = "ivec1",
		[SHADER_IVEC2] = "ivec2",
		[SHADER_IVEC3] = "ivec3",
		[SHADER_IVEC4] = "ivec4",
	};
	return strings[type];
}

static void skip_spaces(const char **line)
{
	if (**line && isspace(**line))
		(*line)++;
}

static bool parse_bind(const char **line, uint8_t *bindp)
{
	char *endptr;
	errno = 0;
	long unsigned bind = strtoul(*line, &endptr, 10);
	if (errno)
	{
		fprintf(stderr, "invalid bind: failed to parse number\n");
		return false;
	}
	if (endptr == *line)
	{
		fprintf(stderr, "invalid bind: no number found\n");
		return false;
	}
	if (*endptr && !isspace(*endptr))
	{
		fprintf(stderr, "invalid bind: not number\n");
		return false;
	}
	if (bind > 16)
	{
		fprintf(stderr, "invalid bind: too big\n");
		return false;
	}
	*bindp = bind;
	*line = endptr + 1;
	return true;
}

static bool parse_variable_type(const char **line, enum shader_variable_type *type)
{
	const char *str = *line;
	if (!**line)
	{
		fprintf(stderr, "invalid type: empty\n");
		return false;
	}
	while (**line && !isspace(**line))
	{
		if (!isalnum(**line))
		{
			fprintf(stderr, "invalid type: isn't [a-zA-Z0-9]\n");
			return false;
		}
		(*line)++;
	}
	if (!variable_type_from_string(str, *line - str, type))
	{
		fprintf(stderr, "invalid type: unknown type: %.*s\n", (int)(*line - str), str);
		return false;
	}
	return true;
}

static bool parse_variable_name(const char **line, char *name, size_t size)
{
	const char *str = *line;
	if (!**line)
	{
		fprintf(stderr, "invalid name: empty\n");
		return false;
	}
	while (**line && !isspace(**line))
	{
		if (!isalnum(**line) && **line != '_')
		{
			fprintf(stderr, "invalid variable name: isn't [a-zA-Z0-9_]\n");
			return false;
		}
		(*line)++;
	}
	if (*line - str >= size)
	{
		fprintf(stderr, "invalid variable name: too long\n");
		return false;
	}
	memcpy(name, str, *line - str);
	name[*line - str] = '\0';
	return true;
}

static bool parse_in(struct shader *shader, const char *line)
{
	if (shader->inputs_nb >= sizeof(shader->inputs) / sizeof(*shader->inputs))
	{
		fprintf(stderr, "too much input\n");
		return false;
	}
	struct shader_input *input = &shader->inputs[shader->inputs_nb];
	skip_spaces((const char**)&line);
	if (!parse_bind(&line, &input->bind))
		return false;
	skip_spaces(&line);
	if (!parse_variable_type(&line, &input->type))
		return false;
	skip_spaces(&line);
	if (!parse_variable_name(&line, input->name, sizeof(input->name)))
		return false;
	if (*line && *line != '\n')
	{
		fprintf(stderr, "invalid input bind: trailing char\n");
		return false;
	}
	shader->inputs_nb++;
	return true;
}

static bool parse_out(struct shader *shader, const char *line)
{
	if (shader->outputs_nb >= sizeof(shader->outputs) / sizeof(*shader->outputs))
	{
		fprintf(stderr, "too much output\n");
		return false;
	}
	struct shader_output *output = &shader->outputs[shader->outputs_nb];
	skip_spaces((const char**)&line);
	if (!parse_bind(&line, &output->bind))
		return false;
	skip_spaces(&line);
	if (!parse_variable_type(&line, &output->type))
		return false;
	skip_spaces(&line);
	if (!parse_variable_name(&line, output->name, sizeof(output->name)))
		return false;
	if (*line && *line != '\n')
	{
		fprintf(stderr, "invalid output bind: trailing char\n");
		return false;
	}
	shader->outputs_nb++;
	return true;
}

static bool parse_constant_members(struct shader_constant *constant, FILE *fp)
{
	char *line = NULL;
	size_t len = 0;
	bool ret = false;
	if (getline(&line, &len, fp) == -1)
	{
		fprintf(stderr, "invalid constant: failed to find {\n");
		return false;
	}
	if (strcmp(line, "{\n"))
	{
		fprintf(stderr, "invalid constant: '{' expected\n");
		return false;
	}
	while (getline(&line, &len, fp) != -1)
	{
		if (!strcmp(line, "}\n"))
			break;
		struct shader_constant_member *member = &constant->members[constant->members_nb];
		if (constant->members_nb >= sizeof(constant->members) / sizeof(*constant->members))
		{
			fprintf(stderr, "invalid constant: too much members\n");
			return false;
		}
		skip_spaces((const char**)&line);
		if (!parse_variable_type((const char**)&line, &member->type))
			return false;
		skip_spaces((const char**)&line);
		if (!parse_variable_name((const char**)&line, member->name, sizeof(member->name)))
			return false;
		constant->members_nb++;
	}
	return true;
}

static bool parse_constant(struct shader *shader, const char *line, FILE *fp)
{
	if (shader->constants_nb >= sizeof(shader->constants) / sizeof(*shader->constants))
	{
		fprintf(stderr, "too much constant\n");
		return false;
	}
	struct shader_constant *constant = &shader->constants[shader->constants_nb];
	skip_spaces((const char**)&line);
	if (!parse_bind(&line, &constant->bind))
		return false;
	skip_spaces((const char**)&line);
	if (!parse_variable_name((const char**)&line, constant->name, sizeof(constant->name)))
		return false;
	if (!parse_constant_members(constant, fp))
		return false;
	shader->constants_nb++;
	return true;
}

static bool parse_sampler(struct shader *shader, const char *line)
{
	if (shader->samplers_nb >= sizeof(shader->samplers) / sizeof(*shader->samplers))
	{
		fprintf(stderr, "too much sampler\n");
		return false;
	}
	struct shader_sampler *sampler = &shader->samplers[shader->samplers_nb];
	skip_spaces((const char**)&line);
	if (!parse_bind(&line, &sampler->bind))
		return false;
	skip_spaces((const char**)&line);
	if (!parse_variable_name((const char**)&line, sampler->name, sizeof(sampler->name)))
		return false;
	shader->samplers_nb++;
	return true;
}

static bool parse_code(struct shader *shader, char **line, size_t *len, FILE *fp)
{
	do
	{
		size_t line_len = strlen(*line);
		char *code = realloc(shader->code, shader->code_size + line_len);
		if (!code)
		{
			fprintf(stderr, "realloc failed\n");
			return false;
		}
		memcpy(code + shader->code_size, *line, line_len + 1);
		shader->code = code;
		shader->code_size += line_len;
	} while (getline(line, len, fp) != -1);
	return true;
}

static bool parse_shader(struct shader *shader, FILE *fp, enum shader_type type)
{
	memset(shader, 0, sizeof(*shader));
	shader->type = type;
	char *line = NULL;
	size_t len = 0;
	bool ret = false;
	while (getline(&line, &len, fp) != -1)
	{
		if (!line[0] || line[0] == '#' || line[0] == '\n')
			continue;
		if (!strncmp(line, "in ", 3))
		{
			if (!parse_in(shader, line + 3))
				goto end;
			continue;
		}
		if (!strncmp(line, "out ", 4))
		{
			if (!parse_out(shader, line + 4))
				goto end;
			continue;
		}
		if (!strncmp(line, "constant ", 9))
		{
			if (!parse_constant(shader, line + 9, fp))
				goto end;
			continue;
		}
		if (!strncmp(line, "sampler ", 8))
		{
			if (!parse_sampler(shader, line + 8))
				goto end;
			continue;
		}
		if (!parse_code(shader, &line, &len, fp))
			goto end;
		break;
	}
	ret = true;
end:
	return ret;
}

static void print_glsl_defines(FILE *fp)
{
	fprintf(fp, "vec4 mul(vec4 v, mat4 m)\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\treturn m * v;");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	fprintf(fp, "vec4 gfx_sample(sampler2D tex, vec2 uv)\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\treturn texture(tex, uv);\n");
	fprintf(fp, "}\n");
}

static void print_gl4_constant(const struct shader_constant *constant, FILE *fp)
{
	fprintf(fp, "layout(std140) uniform %s\n", constant->name);
	fprintf(fp, "{\n");
	for (size_t j = 0; j < constant->members_nb; ++j)
	{
		fprintf(fp, "\t%s %s;\n", variable_type_to_string(constant->members[j].type), constant->members[j].name);
	}
	fprintf(fp, "};\n\n");
}

static bool print_gl4_vs(const struct shader *shader, FILE *fp)
{
	fprintf(fp, "#version 450\n\n");
	print_glsl_defines(fp);
	fprintf(fp, "\n");
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct shader_input *input = &shader->inputs[i];
		fprintf(fp, "layout(location = %u) in %s vs_%s;\n", input->bind, variable_type_to_string(input->type), input->name);
	}
	fprintf(fp, "\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct shader_output *output = &shader->outputs[i];
		fprintf(fp, "layout(location = %u) out %s fs_%s;\n", output->bind, variable_type_to_string(output->type), output->name);
	}
	fprintf(fp, "\n");
	for (size_t i = 0; i < shader->constants_nb; ++i)
		print_gl4_constant(&shader->constants[i], fp);
	fprintf(fp, "struct vs_input\n");
	fprintf(fp, "{\n");
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct shader_input *input = &shader->inputs[i];
		fprintf(fp, "\t%s %s;\n", variable_type_to_string(input->type), input->name);
	}
	fprintf(fp, "};\n");
	fprintf(fp, "\n");
	fprintf(fp, "struct vs_output\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tvec4 gfx_position;\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct shader_output *output = &shader->outputs[i];
		fprintf(fp, "\t%s %s;\n", variable_type_to_string(output->type), output->name);
	}
	fprintf(fp, "};\n");
	fprintf(fp, "\n");
	fprintf(fp, shader->code);
	fprintf(fp, "\n");
	fprintf(fp, "void main()\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tvs_input gfx_in;\n");
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct shader_input *input = &shader->inputs[i];
		fprintf(fp, "\tgfx_in.%s = vs_%s;\n", input->name, input->name);
	}
	fprintf(fp, "\tvs_output gfx_out = gfx_main(gfx_in);\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct shader_output *output = &shader->outputs[i];
		fprintf(fp, "\tfs_%s = gfx_out.%s;\n", output->name, output->name);
	}
	fprintf(fp, "\tgl_Position = gfx_out.gfx_position;\n");
	fprintf(fp, "}\n");
	return true;
}

static bool print_gl4_fs(const struct shader *shader, FILE *fp)
{
	fprintf(fp, "#version 450\n\n");
	print_glsl_defines(fp);
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct shader_input *input = &shader->inputs[i];
		fprintf(fp, "layout(location = %u) in %s vs_%s;\n", input->bind, variable_type_to_string(input->type), input->name);
	}
	if (shader->inputs_nb)
		fprintf(fp, "\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct shader_output *output = &shader->outputs[i];
		fprintf(fp, "layout(location = %u) out %s fs_%s;\n", output->bind, variable_type_to_string(output->type), output->name);
	}
	fprintf(fp, "\n");
	for (size_t i = 0; i < shader->constants_nb; ++i)
		print_gl4_constant(&shader->constants[i], fp);
	for (size_t i = 0; i < shader->samplers_nb; ++i)
	{
		const struct shader_sampler *sampler = &shader->samplers[i];
		fprintf(fp, "layout(location = %u) uniform sampler2D %s;\n", sampler->bind, sampler->name);
	}
	if (shader->samplers_nb)
		fprintf(fp, "\n");
	fprintf(fp, "struct fs_input\n");
	fprintf(fp, "{\n");
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct shader_input *input = &shader->inputs[i];
		fprintf(fp, "\t%s %s;\n", variable_type_to_string(input->type), input->name);
	}
	fprintf(fp, "\tint dummy; /* don't make empty fs_input */\n");
	fprintf(fp, "};\n");
	fprintf(fp, "\n");
	fprintf(fp, "struct fs_output\n");
	fprintf(fp, "{\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct shader_output *output = &shader->outputs[i];
		fprintf(fp, "\t%s %s;\n", variable_type_to_string(output->type), output->name);
	}
	fprintf(fp, "};\n");
	fprintf(fp, "\n");
	fprintf(fp, shader->code);
	fprintf(fp, "\n");
	fprintf(fp, "void main()\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tfs_input gfx_in;\n");
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct shader_input *input = &shader->inputs[i];
		fprintf(fp, "\tgfx_in.%s = vs_%s;\n", input->name, input->name);
	}
	fprintf(fp, "\tfs_output gfx_out = gfx_main(gfx_in);\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct shader_output *output = &shader->outputs[i];
		fprintf(fp, "\tfs_%s = gfx_out.%s;\n", output->name, output->name);
	}
	fprintf(fp, "}\n");
	return true;
}

static void print_vk_constant(const struct shader_constant *constant, FILE *fp)
{
	fprintf(fp, "layout(set = 0, bindind = %u, std140) uniform %s\n", constant->bind, constant->name);
	fprintf(fp, "{\n");
	for (size_t j = 0; j < constant->members_nb; ++j)
	{
		fprintf(fp, "\t%s %s;\n", variable_type_to_string(constant->members[j].type), constant->members[j].name);
	}
	fprintf(fp, "};\n\n");
}

static bool print_vk_vs(const struct shader *shader, FILE *fp)
{
	fprintf(fp, "#version 450\n\n");
	print_glsl_defines(fp);
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct shader_input *input = &shader->inputs[i];
		fprintf(fp, "layout(location = %u) in %s vs_%s;\n", input->bind, variable_type_to_string(input->type), input->name);
	}
	fprintf(fp, "\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct shader_output *output = &shader->outputs[i];
		fprintf(fp, "layout(location = %u) out %s fs_%s;\n", output->bind, variable_type_to_string(output->type), output->name);
	}
	fprintf(fp, "\n");
	for (size_t i = 0; i < shader->constants_nb; ++i)
		print_vk_constant(&shader->constants[i], fp);
	fprintf(fp, shader->code);
	fprintf(fp, "\n");
	fprintf(fp, "void main()\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tvs_input gfx_in;\n");
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct shader_input *input = &shader->inputs[i];
		fprintf(fp, "\tgfx_in.%s = vs_%s;\n", input->name, input->name);
	}
	fprintf(fp, "\tvs_output gfx_out = gfx_main(gfx_in);\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct shader_output *output = &shader->outputs[i];
		fprintf(fp, "\tfs_%s = gfx_out.%s;\n", output->name, output->name);
	}
	fprintf(fp, "\tgl_Position = gfx_out.gfx_position;\n");
	fprintf(fp, "}\n");
	return true;
}

static bool print_vk_fs(const struct shader *shader, FILE *fp)
{
	fprintf(fp, "#version 450\n\n");
	print_glsl_defines(fp);
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct shader_input *input = &shader->inputs[i];
		fprintf(fp, "layout(location = %u) in %s vs_%s;\n", input->bind, variable_type_to_string(input->type), input->name);
	}
	fprintf(fp, "\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct shader_output *output = &shader->outputs[i];
		fprintf(fp, "layout(location = %u) out %s fs_%s;\n", output->bind, variable_type_to_string(output->type), output->name);
	}
	fprintf(fp, "\n");
	for (size_t i = 0; i < shader->constants_nb; ++i)
		print_vk_constant(&shader->constants[i], fp);
	fprintf(fp, shader->code);
	fprintf(fp, "\n");
	fprintf(fp, "void main()\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tvs_input gfx_in;\n");
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct shader_input *input = &shader->inputs[i];
		fprintf(fp, "\tgfx_in.%s = vs_%s;\n", input->name, input->name);
	}
	fprintf(fp, "\tfs_input gfx_out = gfx_main(gfx_in);\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct shader_output *output = &shader->outputs[i];
		fprintf(fp, "\tfs_%s = gfx_out.%s;\n", output->name, output->name);
	}
	fprintf(fp, "}\n");
	return true;
}

static void print_hlsl_defines(FILE *fp)
{
	fprintf(fp, "#define vec1 float1\n");
	fprintf(fp, "#define vec2 float2\n");
	fprintf(fp, "#define vec3 float3\n");
	fprintf(fp, "#define vec4 float4\n");
	fprintf(fp, "#define ivec1 int1\n");
	fprintf(fp, "#define ivec2 int2\n");
	fprintf(fp, "#define ivec3 int3\n");
	fprintf(fp, "#define ivec4 int4\n");
	fprintf(fp, "#define mat1 float1x1\n");
	fprintf(fp, "#define mat2 float2x2\n");
	fprintf(fp, "#define mat3 float3x3\n");
	fprintf(fp, "#define mat4 float4x4\n");
	fprintf(fp, "\n");
	fprintf(fp, "#define gfx_sample(name, uv) name.Sample(name##_sampler, uv)\n");
	fprintf(fp, "\n");
	fprintf(fp, "#define mix(a, b, v) lerp(a, b, v)\n");
}

static void print_d3d11_constant(const struct shader_constant *constant, FILE *fp)
{
	fprintf(fp, "cbuffer %s : register(b%u)\n", constant->name, constant->bind);
	fprintf(fp, "{\n");
	for (size_t j = 0; j < constant->members_nb; ++j)
	{
		fprintf(fp, "\t%s %s;\n", variable_type_to_string(constant->members[j].type), constant->members[j].name);
	}
	fprintf(fp, "};\n\n");
}

static void print_d3d11_sampler(const struct shader_sampler *sampler, FILE *fp)
{
	fprintf(fp, "Texture2D %s : register(t%u);\n", sampler->name, sampler->bind);
	fprintf(fp, "SamplerState %s_sampler : register(s%u);\n", sampler->name, sampler->bind);
}

static bool print_d3d11_vs(const struct shader *shader, FILE *fp)
{
	print_hlsl_defines(fp);
	fprintf(fp, "\n");
	fprintf(fp, "struct vs_input\n");
	fprintf(fp, "{\n");
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct shader_input *input = &shader->inputs[i];
		fprintf(fp, "\t%s %s : VS_INPUT%u;\n", variable_type_to_string(input->type), input->name, input->bind);
	}
	fprintf(fp, "};\n");
	fprintf(fp, "\n");
	fprintf(fp, "struct vs_output\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tvec4 gfx_position : SV_POSITION;\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct shader_output *output = &shader->outputs[i];
		fprintf(fp, "\t%s %s : FS_INPUT%u;\n", variable_type_to_string(output->type), output->name, output->bind);
	}
	fprintf(fp, "};\n");
	fprintf(fp, "\n");
	for (size_t i = 0; i < shader->constants_nb; ++i)
		print_d3d11_constant(&shader->constants[i], fp);
	fprintf(fp, shader->code);
	fprintf(fp, "\n");
	fprintf(fp, "vs_output main(vs_input input)\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\treturn gfx_main(input);\n");
	fprintf(fp, "}\n");
	return true;
}

static bool print_d3d11_fs(const struct shader *shader, FILE *fp)
{
	print_hlsl_defines(fp);
	fprintf(fp, "\n");
	fprintf(fp, "struct fs_input\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tvec4 gfx_position : SV_POSITION;\n");
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct shader_input *input = &shader->inputs[i];
		fprintf(fp, "\t%s %s : FS_INPUT%u;\n", variable_type_to_string(input->type), input->name, input->bind);
	}
	fprintf(fp, "};\n");
	fprintf(fp, "\n");
	fprintf(fp, "struct fs_output\n");
	fprintf(fp, "{\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct shader_output *output = &shader->outputs[i];
		fprintf(fp, "\t%s %s : SV_TARGET%u;\n", variable_type_to_string(output->type), output->name, output->bind);
	}
	fprintf(fp, "};\n");
	fprintf(fp, "\n");
	for (size_t i = 0; i < shader->constants_nb; ++i)
		print_d3d11_constant(&shader->constants[i], fp);
	for (size_t i = 0; i < shader->samplers_nb; ++i)
		print_d3d11_sampler(&shader->samplers[i], fp);
	if (shader->samplers_nb)
		fprintf(fp, "\n");
	fprintf(fp, shader->code);
	fprintf(fp, "\n");
	fprintf(fp, "fs_output main(fs_input input)\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\treturn gfx_main(input);\n");
	fprintf(fp, "}\n");
	return true;
}

static bool print_shader(struct shader *shader, FILE *fp, enum shader_api api)
{
	switch (api)
	{
		case SHADER_GL4:
			switch (shader->type)
			{
				case SHADER_VERTEX:
					return print_gl4_vs(shader, fp);
				case SHADER_FRAGMENT:
					return print_gl4_fs(shader, fp);
				default:
					fprintf(stderr, "unknown shader type");
					return false;
			}
			break;
		case SHADER_VK:
			switch (shader->type)
			{
				case SHADER_VERTEX:
					return print_vk_vs(shader, fp);
				case SHADER_FRAGMENT:
					return print_vk_fs(shader, fp);
				default:
					fprintf(stderr, "unknown shader type");
					return false;
			}
			break;
		case SHADER_D3D11:
			switch (shader->type)
			{
				case SHADER_VERTEX:
					return print_d3d11_vs(shader, fp);
				case SHADER_FRAGMENT:
					return print_d3d11_fs(shader, fp);
				default:
					fprintf(stderr, "unknown shader type");
					return false;
			}
			break;
		default:
			fprintf(stderr, "unknown api\n");
			return false;
	}
	return true;
}

static void usage()
{
	printf("compile -t <type> -x <api> -i <input> -o <output>\n");
	printf("-t: shader type (vs, fs)\n");
	printf("-x: api (gl3, gl4, gles3, vk, d3d9, d3d11)\n");
	printf("-i: input file\n");
	printf("-o: output file\n");
}

int main(int argc, char **argv)
{
	int ret = EXIT_FAILURE;
	int c;
	const char *in = NULL;
	const char *out = NULL;
	const char *type = NULL;
	const char *api = NULL;
	enum shader_type shader_type;
	enum shader_api shader_api;
	opterr = 1;
	while ((c = getopt(argc, argv, "t:x:i:o:")) != -1)
	{
		switch (c)
		{
			case 'i':
				in = optarg;
				break;
			case 'o':
				out = optarg;
				break;
			case 't':
				type = optarg;
				break;
			case 'x':
				api = optarg;
				break;
			default:
				usage();
				return EXIT_FAILURE;
		}
	}
	if (!in)
	{
		fprintf(stderr, "no input file given\n");
		usage();
		return EXIT_FAILURE;
	}
	if (!out)
	{
		fprintf(stderr, "no output file given\n");
		usage();
		return EXIT_FAILURE;
	}
	if (!type)
	{
		fprintf(stderr, "no type given\n");
		usage();
		return EXIT_FAILURE;
	}
	if (!api)
	{
		fprintf(stderr, "no api given\n");
		usage();
		return EXIT_FAILURE;
	}
	if (!shader_type_from_string(type, &shader_type))
	{
		fprintf(stderr, "unknown shader type\n");
		usage();
		return EXIT_FAILURE;
	}
	if (!shader_api_from_string(api, &shader_api))
	{
		fprintf(stderr, "unknown shader api\n");
		usage();
		return EXIT_FAILURE;
	}
	FILE *fp = fopen(in, "r");
	if (!fp)
	{
		fprintf(stderr, "failed to open %s\n", in);
		return EXIT_FAILURE;
	}
	struct shader shader;
	memset(&shader, 0, sizeof(shader));
	if (!parse_shader(&shader, fp, shader_type))
	{
		fprintf(stderr, "failed to parse shader\n");
		fclose(fp);
		return EXIT_FAILURE;
	}
	fclose(fp);
	fp = fopen(out, "w");
	if (!fp)
	{
		fprintf(stderr, "failed to open %s\n", out);
		fclose(fp);
		return EXIT_FAILURE;
	}
	print_shader(&shader, fp, shader_api);
	fclose(fp);
	return EXIT_SUCCESS;
}
