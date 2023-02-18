#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
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
	API_GL3,
	API_GL4,
	API_GLES3,
	API_VK,
	API_D3D9,
	API_D3D11,
};

enum variable_type
{
	VARIABLE_FLOAT,
	VARIABLE_VEC1,
	VARIABLE_VEC2,
	VARIABLE_VEC3,
	VARIABLE_VEC4,
	VARIABLE_MAT1,
	VARIABLE_MAT2,
	VARIABLE_MAT3,
	VARIABLE_MAT4,
	VARIABLE_INT,
	VARIABLE_IVEC1,
	VARIABLE_IVEC2,
	VARIABLE_IVEC3,
	VARIABLE_IVEC4,
	VARIABLE_STRUCT,
};

enum sampler_type
{
	SAMPLER_2D,
	SAMPLER_2D_ARRAY,
	SAMPLER_3D,
};

struct shader_constant_member
{
	enum variable_type type;
	char struct_name[64];
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
	enum sampler_type type;
};

struct shader_output
{
	uint8_t bind;
	char name[64];
	enum variable_type type;
};

struct shader_input
{
	uint8_t bind;
	char name[64];
	enum variable_type type;
};

struct shader_struct_member
{
	enum variable_type type;
	char name[64];
};

struct shader_struct
{
	char name[64];
	struct shader_struct_member members[64];
	uint32_t members_nb;
};

struct shader
{
	struct shader_constant constants[16];
	struct shader_sampler samplers[16];
	struct shader_struct structs[16];
	struct shader_output outputs[16];
	struct shader_input inputs[16];
	uint8_t constants_nb;
	uint8_t samplers_nb;
	uint8_t structs_nb;
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
		[API_GL3] = "gl3",
		[API_GL4] = "gl4",
		[API_GLES3] = "gles3",
		[API_VK] = "vk",
		[API_D3D9] = "d3d9",
		[API_D3D11] = "d3d11",
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

static bool variable_type_from_string(const char *str, size_t len, enum variable_type *type)
{
#define TEST(name, val) \
	if (!strncmp(str, #name, len)) \
	{ \
		*type = VARIABLE_##val; \
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

static bool sampler_type_from_string(const char *str, size_t len, enum sampler_type *type)
{
#define TEST(name, val) \
	if (!strncmp(str, #name, len)) \
	{ \
		*type = SAMPLER_##val; \
		return true; \
	}

	TEST(2d, 2D);
	TEST(2d_array, 2D_ARRAY);
	TEST(3d, 3D);
	return false;

#undef TEST
}

static const char *variable_type_to_string(enum variable_type type)
{
	static const char *strings[] =
	{
		[VARIABLE_FLOAT] = "float",
		[VARIABLE_VEC1] = "vec1",
		[VARIABLE_VEC2] = "vec2",
		[VARIABLE_VEC3] = "vec3",
		[VARIABLE_VEC4] = "vec4",
		[VARIABLE_MAT1] = "mat1",
		[VARIABLE_MAT2] = "mat2",
		[VARIABLE_MAT3] = "mat3",
		[VARIABLE_MAT4] = "mat4",
		[VARIABLE_INT] = "int",
		[VARIABLE_IVEC1] = "ivec1",
		[VARIABLE_IVEC2] = "ivec2",
		[VARIABLE_IVEC3] = "ivec3",
		[VARIABLE_IVEC4] = "ivec4",
	};
	return strings[type];
}

static const char *sampler_type_to_string(enum sampler_type type)
{
	static const char *strings[] =
	{
		[SAMPLER_2D] = "sampler2D",
		[SAMPLER_2D_ARRAY] = "sampler2DArray",
		[SAMPLER_3D] = "sampler3D",
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

static bool parse_variable_type(const char **line, enum variable_type *type)
{
	const char *str = *line;
	if (!**line)
	{
		fprintf(stderr, "invalid variable type: empty\n");
		return false;
	}
	while (**line && !isspace(**line))
	{
		if (!isalnum(**line) && **line != '_')
		{
			fprintf(stderr, "invalid variable type: isn't [a-zA-Z0-9_]\n");
			return false;
		}
		(*line)++;
	}
	return variable_type_from_string(str, *line - str, type);
}

static bool parse_variable_name(const char **line, char *name, size_t size)
{
	const char *str = *line;
	if (!**line)
	{
		fprintf(stderr, "invalid variable name: empty\n");
		return false;
	}
	while (**line && !isspace(**line))
	{
		if (!isalnum(**line) && **line != '_' && **line != '[' && **line != ']')
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

static bool parse_sampler_type(const char **line, enum sampler_type *type)
{
	const char *str = *line;
	if (!**line)
	{
		fprintf(stderr, "invalid sampler type: empty\n");
		return false;
	}
	while (**line && !isspace(**line))
	{
		if (!isalnum(**line) && **line != '_')
		{
			fprintf(stderr, "invalid sampler type: isn't [a-zA-Z0-9_]\n");
			return false;
		}
		(*line)++;
	}
	return sampler_type_from_string(str, *line - str, type);
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
	{
		fprintf(stderr, "invalid input type\n");
		return false;
	}
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
	{
		fprintf(stderr, "invalid output type\n");
		return false;
	}
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

static bool parse_constant_member_type(const char **line, struct shader_constant_member *member)
{
	const char *ptr = *line;
	if (parse_variable_type(line, &member->type))
		return true;
	if (*line - ptr >= sizeof(member->struct_name))
	{
		fprintf(stderr, "invalid constant: member type too long\n");
		return false;
	}
	memcpy(member->struct_name, ptr, *line - ptr);
	member->type = VARIABLE_STRUCT;
	member->struct_name[*line - ptr] = '\0';
	return true;
}

static bool parse_constant_members(struct shader_constant *constant, FILE *fp)
{
	char *lineptr = NULL;
	size_t len = 0;
	bool ret = false;
	if (getline(&lineptr, &len, fp) == -1)
	{
		fprintf(stderr, "invalid constant: failed to find {\n");
		return false;
	}
	if (strcmp(lineptr, "{\n"))
	{
		fprintf(stderr, "invalid constant: '{' expected\n");
		return false;
	}
	while (getline(&lineptr, &len, fp) != -1)
	{
		const char *line = lineptr;
		if (!strcmp(line, "}\n"))
			break;
		struct shader_constant_member *member = &constant->members[constant->members_nb];
		if (constant->members_nb >= sizeof(constant->members) / sizeof(*constant->members))
		{
			fprintf(stderr, "invalid constant: too much members\n");
			return false;
		}
		skip_spaces(&line);
		if (!parse_constant_member_type(&line, member))
			return false;
		skip_spaces(&line);
		if (!parse_variable_name(&line, member->name, sizeof(member->name)))
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
	skip_spaces(&line);
	if (!parse_bind(&line, &constant->bind))
		return false;
	skip_spaces(&line);
	if (!parse_variable_name(&line, constant->name, sizeof(constant->name)))
		return false;
	if (!parse_constant_members(constant, fp))
		return false;
	shader->constants_nb++;
	return true;
}

static bool parse_struct_members(struct shader_struct *st, FILE *fp)
{
	char *lineptr = NULL;
	size_t len = 0;
	bool ret = false;
	if (getline(&lineptr, &len, fp) == -1)
	{
		fprintf(stderr, "invalid struct: failed to find {\n");
		return false;
	}
	if (strcmp(lineptr, "{\n"))
	{
		fprintf(stderr, "invalid struct: '{' expected\n");
		return false;
	}
	while (getline(&lineptr, &len, fp) != -1)
	{
		const char *line = lineptr;
		if (!strcmp(line, "}\n"))
			break;
		struct shader_struct_member *member = &st->members[st->members_nb];
		if (st->members_nb >= sizeof(st->members) / sizeof(*st->members))
		{
			fprintf(stderr, "invalid struct: too much members\n");
			return false;
		}
		skip_spaces(&line);
		if (!parse_variable_type(&line, &member->type))
		{
			fprintf(stderr, "invalid struct: unknown member type\n");
			return false;
		}
		skip_spaces(&line);
		if (!parse_variable_name(&line, member->name, sizeof(member->name)))
			return false;
		st->members_nb++;
	}
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
	skip_spaces(&line);
	if (!parse_bind(&line, &sampler->bind))
		return false;
	skip_spaces(&line);
	if (!parse_sampler_type(&line, &sampler->type))
		return false;
	skip_spaces(&line);
	if (!parse_variable_name(&line, sampler->name, sizeof(sampler->name)))
		return false;
	if (*line && *line != '\n')
	{
		fprintf(stderr, "invalid sampler: trailing char\n");
		return false;
	}
	shader->samplers_nb++;
	return true;
}

static bool parse_struct(struct shader *shader, const char *line, FILE *fp)
{
	if (shader->structs_nb >= sizeof(shader->structs) / sizeof(*shader->structs))
	{
		fprintf(stderr, "too much struct\n");
		return false;
	}
	struct shader_struct *st = &shader->structs[shader->structs_nb];
	skip_spaces(&line);
	if (!parse_variable_name(&line, st->name, sizeof(st->name)))
		return false;
	if (!parse_struct_members(st, fp))
		return false;
	shader->structs_nb++;
	return true;
}

static bool parse_code(struct shader *shader, char **line, size_t *len, FILE *fp)
{
	do
	{
		size_t line_len = strlen(*line);
		char *code = realloc(shader->code, shader->code_size + line_len + 1);
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
		if (!line[0] || line[0] == '\n')
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
		if (!strncmp(line, "struct ", 7))
		{
			if (!parse_struct(shader, line + 7, fp))
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

static void print_glsl_defines(FILE *fp, enum shader_api api)
{
	fprintf(fp, "#define GFX_GLSL\n");
	fprintf(fp, "#define mul(a, b) (b * a)\n");
	fprintf(fp, "#define gfx_sample(t, u) texture(t, u)\n");
	fprintf(fp, "#define gfx_sample_offset(t, u, o) textureOffset(t, u, o)\n");
	fprintf(fp, "\n");
	if (api == API_GLES3)
	{
		fprintf(fp, "precision highp float;\n");
		fprintf(fp, "precision highp int;\n");
		fprintf(fp, "precision highp sampler2D;\n");
		fprintf(fp, "precision highp sampler2DArray;\n");
		fprintf(fp, "\n");
	}
}

static void print_glsl_constant(const struct shader_constant *constant, FILE *fp, enum shader_api api)
{
	switch (api)
	{
		case API_VK:
			fprintf(fp, "layout(set = 0, binding = %u, std140) uniform %s\n", constant->bind, constant->name);
			break;
		case API_GL4:
		case API_GLES3:
			fprintf(fp, "layout(binding = %u, std140) uniform %s\n", constant->bind, constant->name);
			break;
		case API_GL3:
			fprintf(fp, "layout(std140) uniform %s\n", constant->name);
			break;
		default:
			assert(!"unknown api\n");
	}
	fprintf(fp, "{\n");
	for (size_t j = 0; j < constant->members_nb; ++j)
	{
		fprintf(fp, "\t%s %s;\n", constant->members[j].type == VARIABLE_STRUCT ? constant->members[j].struct_name : variable_type_to_string(constant->members[j].type), constant->members[j].name);
	}
	fprintf(fp, "};\n\n");
}

static void print_glsl_struct(const struct shader_struct *st, FILE *fp)
{
	fprintf(fp, "struct %s\n", st->name);
	fprintf(fp, "{\n");
	for (size_t j = 0; j < st->members_nb; ++j)
	{
		fprintf(fp, "\t%s %s;\n", variable_type_to_string(st->members[j].type), st->members[j].name);
	}
	fprintf(fp, "};\n\n");
}

static void print_glsl_sampler(const struct shader_sampler *sampler, FILE *fp, enum shader_api api)
{
	switch (api)
	{
		case API_VK:
			fprintf(fp, "layout(set = 1, binding = %u) uniform %s %s;\n", sampler->bind, sampler_type_to_string(sampler->type), sampler->name);
			break;
		case API_GL4:
		case API_GLES3:
			fprintf(fp, "layout(binding = %u) uniform %s %s;\n", sampler->bind, sampler_type_to_string(sampler->type), sampler->name);
			break;
		case API_GL3:
			fprintf(fp, "uniform %s %s;\n", sampler_type_to_string(sampler->type), sampler->name);
			break;
		default:
			assert(!"unknown api\n");
	}
}

static void print_glsl_version(FILE *fp, enum shader_api api)
{
	switch (api)
	{
		case API_VK:
		case API_GL4:
			fprintf(fp, "#version 450 core\n\n");
			break;
		case API_GL3:
			fprintf(fp, "#version 330 core\n\n");
			break;
		case API_GLES3:
			fprintf(fp, "#version 320 es\n\n");
			break;
		default:
			assert(!"unknown api\n");
	}
}

static bool print_glsl_vs(const struct shader *shader, FILE *fp, enum shader_api api)
{
	print_glsl_version(fp, api);
	print_glsl_defines(fp, api);
	fprintf(fp, "\n");
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct shader_input *input = &shader->inputs[i];
		fprintf(fp, "layout(location = %u) in %s gfx_in_vs_%s;\n", input->bind, variable_type_to_string(input->type), input->name);
	}
	fprintf(fp, "\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct shader_output *output = &shader->outputs[i];
		switch (api)
		{
			case API_GL3:
				fprintf(fp, "%sout %s gfx_in_fs_%s;\n", output->type == VARIABLE_INT ? "flat " : "", variable_type_to_string(output->type), output->name);
				break;
			case API_GL4:
			case API_VK:
			case API_GLES3:
				fprintf(fp, "layout(location = %u) out %s%s gfx_in_fs_%s;\n", output->bind, output->type == VARIABLE_INT ? "flat " : "", variable_type_to_string(output->type), output->name);
				break;
			default:
				assert(!"unknown api\n");
		}
	}
	fprintf(fp, "\n");
	for (size_t i = 0; i < shader->structs_nb; ++i)
		print_glsl_struct(&shader->structs[i], fp);
	for (size_t i = 0; i < shader->constants_nb; ++i)
		print_glsl_constant(&shader->constants[i], fp, api);
	fprintf(fp, "struct vs_input\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tint vertex_id;\n");
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
	switch (api)
	{
		case API_GL3:
		case API_GL4:
		case API_GLES3:
			fprintf(fp, "\tgfx_in.vertex_id = gl_VertexID;\n");
			break;
		case API_VK:
			fprintf(fp, "\tgfx_in.vertex_id = gl_VertexIndex;\n");
			break;
		default:
			assert(!"unknown api\n");
	}
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct shader_input *input = &shader->inputs[i];
		fprintf(fp, "\tgfx_in.%s = gfx_in_vs_%s;\n", input->name, input->name);
	}
	fprintf(fp, "\tvs_output gfx_out = gfx_main(gfx_in);\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct shader_output *output = &shader->outputs[i];
		fprintf(fp, "\tgfx_in_fs_%s = gfx_out.%s;\n", output->name, output->name);
	}
	fprintf(fp, "\tgl_Position = gfx_out.gfx_position;\n");
	fprintf(fp, "}\n");
	return true;
}

static bool print_glsl_fs(const struct shader *shader, FILE *fp, enum shader_api api)
{
	print_glsl_version(fp, api);
	print_glsl_defines(fp, api);
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct shader_input *input = &shader->inputs[i];
		if (api == API_GL3)
			fprintf(fp, "%sin %s gfx_in_fs_%s;\n", input->type == VARIABLE_INT ? "flat " : "", variable_type_to_string(input->type), input->name);
		else
			fprintf(fp, "layout(location = %u) in %s%s gfx_in_fs_%s;\n", input->bind, input->type == VARIABLE_INT ? "flat " : "", variable_type_to_string(input->type), input->name);
	}
	if (shader->inputs_nb)
		fprintf(fp, "\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct shader_output *output = &shader->outputs[i];
		fprintf(fp, "layout(location = %u) out %s gfx_out_fs_%s;\n", output->bind, variable_type_to_string(output->type), output->name);
	}
	fprintf(fp, "\n");
	for (size_t i = 0; i < shader->structs_nb; ++i)
		print_glsl_struct(&shader->structs[i], fp);
	for (size_t i = 0; i < shader->constants_nb; ++i)
		print_glsl_constant(&shader->constants[i], fp, api);
	for (size_t i = 0; i < shader->samplers_nb; ++i)
		print_glsl_sampler(&shader->samplers[i], fp, api);
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
		fprintf(fp, "\tgfx_in.%s = gfx_in_fs_%s;\n", input->name, input->name);
	}
	fprintf(fp, "\tfs_output gfx_out = gfx_main(gfx_in);\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct shader_output *output = &shader->outputs[i];
		fprintf(fp, "\tgfx_out_fs_%s = gfx_out.%s;\n", output->name, output->name);
	}
	fprintf(fp, "}\n");
	return true;
}

static void print_hlsl_defines(FILE *fp)
{
	fprintf(fp, "#define GFX_HLSL\n");
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
	fprintf(fp, "#define sampler2D Texture2D\n");
	fprintf(fp, "#define sampler2DArray Texture2DArray\n");
	fprintf(fp, "\n");
	fprintf(fp, "#define gfx_sample(name, uv) name.Sample(name##_sampler, uv)\n");
	fprintf(fp, "#define gfx_sample_offset(name, uv, offset) name.Sample(name##_sampler, uv, offset)\n");
	fprintf(fp, "\n");
	fprintf(fp, "#define mix(a, b, v) lerp(a, b, v)\n");
	fprintf(fp, "#define mod(a, b) fmod(a, b)\n");
}

static void print_d3d11_constant(const struct shader_constant *constant, FILE *fp)
{
	fprintf(fp, "cbuffer %s : register(b%u)\n", constant->name, constant->bind);
	fprintf(fp, "{\n");
	for (size_t j = 0; j < constant->members_nb; ++j)
	{
		fprintf(fp, "\t%s %s;\n", constant->members[j].type == VARIABLE_STRUCT ? constant->members[j].struct_name : variable_type_to_string(constant->members[j].type), constant->members[j].name);
	}
	fprintf(fp, "};\n\n");
}

static void print_d3d11_sampler(const struct shader_sampler *sampler, FILE *fp)
{
	fprintf(fp, "%s %s : register(t%u);\n", sampler_type_to_string(sampler->type), sampler->name, sampler->bind);
	fprintf(fp, "SamplerState %s_sampler : register(s%u);\n", sampler->name, sampler->bind);
}

static void print_d3d11_struct(const struct shader_struct *st, FILE *fp)
{
	fprintf(fp, "struct %s\n", st->name);
	fprintf(fp, "{\n");
	for (size_t j = 0; j < st->members_nb; ++j)
	{
		fprintf(fp, "\t%s %s;\n", variable_type_to_string(st->members[j].type), st->members[j].name);
	}
	fprintf(fp, "};\n\n");
}

static bool print_d3d11_vs(const struct shader *shader, FILE *fp)
{
	print_hlsl_defines(fp);
	fprintf(fp, "\n");
	fprintf(fp, "struct vs_input\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tuint vertex_id : SV_VertexID;\n");
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
		fprintf(fp, "\t%s%s %s : FS_INPUT%u;\n", output->type == VARIABLE_INT ? "nointerpolation " : "", variable_type_to_string(output->type), output->name, output->bind);
	}
	fprintf(fp, "};\n");
	fprintf(fp, "\n");
	for (size_t i = 0; i < shader->structs_nb; ++i)
		print_d3d11_struct(&shader->structs[i], fp);
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
		fprintf(fp, "\t%s%s %s : FS_INPUT%u;\n", input->type == VARIABLE_INT ? "nointerpolation " : "", variable_type_to_string(input->type), input->name, input->bind);
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
	for (size_t i = 0; i < shader->structs_nb; ++i)
		print_d3d11_struct(&shader->structs[i], fp);
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
		case API_GL3:
		case API_GL4:
		case API_GLES3:
		case API_VK:
			switch (shader->type)
			{
				case SHADER_VERTEX:
					return print_glsl_vs(shader, fp, api);
				case SHADER_FRAGMENT:
					return print_glsl_fs(shader, fp, api);
				default:
					fprintf(stderr, "unknown shader type");
					return false;
			}
			break;
		case API_D3D11:
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
