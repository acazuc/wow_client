#include "./compile.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

struct shader_ctx
{
	struct gfx_shader_constant_def constants[16];
	struct gfx_shader_sampler_def samplers[16];
	struct gfx_shader_struct_def structs[16];
	struct gfx_shader_output_def outputs[16];
	struct gfx_shader_input_def inputs[16];
	uint8_t constants_nb;
	uint8_t samplers_nb;
	uint8_t structs_nb;
	uint8_t outputs_nb;
	uint8_t inputs_nb;
	uint8_t type;
	size_t code_size;
	char *code;
};

struct membuf
{
	char *data;
	size_t size;
};

static void membuf_printf(struct membuf *membuf, char *fmt, ...)
{
	va_list va_arg;
	va_list va_arg2;
	va_start(va_arg, fmt);
	va_copy(va_arg2, va_arg);
	size_t n = vsnprintf(NULL, 0, fmt, va_arg);
	char *data = realloc(membuf->data, membuf->size + n + 1);
	vsnprintf(data + membuf->size, n + 1, fmt, va_arg2);
	membuf->data = data;
	membuf->size += n;
	va_end(va_arg2);
	va_end(va_arg);
}

static bool membuf_read_file(struct membuf *membuf, int fd)
{
	while (1)
	{
		char *data = realloc(membuf->data, membuf->size + 4096);
		if (!data)
		{
			fprintf(stderr, "allocation failed\n");
			return false;
		}
		membuf->data = data;
		int ret = read(fd, &data[membuf->size], 4096);
		if (!ret)
			return true;
		if (ret < 0)
		{
			fprintf(stderr, "failed to read file\n");
			return false;
		}
		membuf->size += ret;
	}
}

static bool shader_type_from_string(const char *str, enum gfx_shader_type *type)
{
	static const char *strings[] =
	{
		[GFX_SHADER_VERTEX]   = "vs",
		[GFX_SHADER_FRAGMENT] = "fs",
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

static bool shader_backend_from_string(const char *str, enum gfx_device_backend *backend)
{
	static const char *strings[] =
	{
		[GFX_DEVICE_GL3] = "gl3",
		[GFX_DEVICE_GL4] = "gl4",
		[GFX_DEVICE_GLES3] = "gles3",
		[GFX_DEVICE_VK] = "vk",
		[GFX_DEVICE_D3D9] = "d3d9",
		[GFX_DEVICE_D3D11] = "d3d11",
	};
	for (size_t i = 0; i < sizeof(strings) / sizeof(*strings); ++i)
	{
		if (!strcmp(strings[i], str))
		{
			*backend = i;
			return true;
		}
	}
	return false;
}

static bool shader_backend_mask_from_string(const char *str, uint32_t *mask)
{
	char tmp[8];
	enum gfx_device_backend backend;
	char *coma;
	while ((coma = strchr(str, ',')))
	{
		if (snprintf(tmp, sizeof(tmp), "%.*s", coma - str, str) >= sizeof(tmp))
			return false;
		if (!shader_backend_from_string(tmp, &backend))
			return false;
		*mask |= (1 << backend);
		str = coma + 1;
	}
	if (!shader_backend_from_string(tmp, &backend))
		return false;
	*mask |= (1 << backend);
	return true;
}

static bool variable_type_from_string(const char *str, size_t len, enum gfx_variable_type *type)
{
#define TEST(name, val) \
	if (!strncmp(str, #name, len)) \
	{ \
		*type = GFX_VARIABLE_##val; \
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

static bool sampler_type_from_string(const char *str, size_t len, enum gfx_texture_type *type)
{
#define TEST(name, val) \
	if (!strncmp(str, #name, len)) \
	{ \
		*type = GFX_TEXTURE_##val; \
		return true; \
	}

	TEST(2d, 2D);
	TEST(2d_array, 2D_ARRAY);
	TEST(3d, 3D);
	return false;

#undef TEST
}

static const char *variable_type_to_string(enum gfx_variable_type type)
{
	static const char *strings[] =
	{
		[GFX_VARIABLE_FLOAT] = "float",
		[GFX_VARIABLE_VEC1] = "vec1",
		[GFX_VARIABLE_VEC2] = "vec2",
		[GFX_VARIABLE_VEC3] = "vec3",
		[GFX_VARIABLE_VEC4] = "vec4",
		[GFX_VARIABLE_MAT1] = "mat1",
		[GFX_VARIABLE_MAT2] = "mat2",
		[GFX_VARIABLE_MAT3] = "mat3",
		[GFX_VARIABLE_MAT4] = "mat4",
		[GFX_VARIABLE_INT] = "int",
		[GFX_VARIABLE_IVEC1] = "ivec1",
		[GFX_VARIABLE_IVEC2] = "ivec2",
		[GFX_VARIABLE_IVEC3] = "ivec3",
		[GFX_VARIABLE_IVEC4] = "ivec4",
	};
	return strings[type];
}

static const char *sampler_type_to_string(enum gfx_texture_type type)
{
	static const char *strings[] =
	{
		[GFX_TEXTURE_2D] = "sampler2D",
		[GFX_TEXTURE_2D_ARRAY] = "sampler2DArray",
		[GFX_TEXTURE_3D] = "sampler3D",
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

static bool parse_variable_type(const char **line, enum gfx_variable_type *type)
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
		if (**line == '[')
			break;
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

static bool parse_texture_type(const char **line, enum gfx_texture_type *type)
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

static bool parse_array_size(const char **line, uint16_t *array_sizep)
{
	if (**line != '[')
	{
		fprintf(stderr, "invalid array size: unknown char\n");
		return false;
	}
	(*line)++;
	char *endptr;
	errno = 0;
	long unsigned array_size = strtoul(*line, &endptr, 10);
	if (errno)
	{
		fprintf(stderr, "invalid array size: failed to parse number\n");
		return false;
	}
	if (endptr == *line)
	{
		fprintf(stderr, "invalid array size: no number found\n");
		return false;
	}
	if (*endptr && *endptr != ']')
	{
		fprintf(stderr, "invalid array size: not numeric\n");
		return false;
	}
	if (array_size > UINT16_MAX)
	{
		fprintf(stderr, "invalid array size: too big\n");
		return false;
	}
	*array_sizep = array_size;
	*line = endptr + 1;
	return true;
}

static bool parse_in(struct shader_ctx *shader, const char *line)
{
	if (shader->inputs_nb >= sizeof(shader->inputs) / sizeof(*shader->inputs))
	{
		fprintf(stderr, "too much input\n");
		return false;
	}
	struct gfx_shader_input_def *input = &shader->inputs[shader->inputs_nb];
	skip_spaces((const char**)&line);
	if (!parse_bind(&line, &input->bind))
		return false;
	skip_spaces(&line);
	enum gfx_variable_type type;
	if (!parse_variable_type(&line, &type))
	{
		fprintf(stderr, "invalid input type\n");
		return false;
	}
	input->type = type;
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

static bool parse_out(struct shader_ctx *shader, const char *line)
{
	if (shader->outputs_nb >= sizeof(shader->outputs) / sizeof(*shader->outputs))
	{
		fprintf(stderr, "too much output\n");
		return false;
	}
	struct gfx_shader_output_def *output = &shader->outputs[shader->outputs_nb];
	skip_spaces((const char**)&line);
	if (!parse_bind(&line, &output->bind))
		return false;
	skip_spaces(&line);
	enum gfx_variable_type type;
	if (!parse_variable_type(&line, &type))
	{
		fprintf(stderr, "invalid output type\n");
		return false;
	}
	output->type = type;
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

static bool parse_constant_member_type(struct shader_ctx *shader, const char **line, struct gfx_shader_constant_member_def *member)
{
	const char *ptr = *line;
	enum gfx_variable_type type;
	if (parse_variable_type(line, &type))
	{
		member->type = type;
		return true;
	}
	char name[64];
	if (*line - ptr >= sizeof(name))
	{
		fprintf(stderr, "invalid constant: member type too long\n");
		return false;
	}
	memcpy(name, ptr, *line - ptr);
	member->type = GFX_VARIABLE_STRUCT;
	name[*line - ptr] = '\0';
	for (size_t i = 0; i < shader->structs_nb; ++i)
	{
		struct gfx_shader_struct_def *st = &shader->structs[i];
		if (!strcmp(name, st->name))
		{
			member->struct_id = i;
			return true;
		}
	}
	return false;
}

static bool parse_constant_members(struct shader_ctx *shader, struct gfx_shader_constant_def *constant, FILE *fp)
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
		struct gfx_shader_constant_member_def *member = &constant->members[constant->members_nb];
		if (constant->members_nb >= sizeof(constant->members) / sizeof(*constant->members))
		{
			fprintf(stderr, "invalid constant: too much members\n");
			return false;
		}
		skip_spaces(&line);
		if (!parse_constant_member_type(shader, &line, member))
			return false;
		skip_spaces(&line);
		if (!parse_variable_name(&line, member->name, sizeof(member->name)))
			return false;
		if (*line == '[')
		{
			if (!parse_array_size(&line, &member->array_size))
				return false;
		}
		else
		{
			member->array_size = 0;
		}
		if (*line && *line != '\n')
		{
			fprintf(stderr, "invalid constant name: trailing char\n");
			return false;
		}
		constant->members_nb++;
	}
	return true;
}

static bool parse_constant(struct shader_ctx *shader, const char *line, FILE *fp)
{
	if (shader->constants_nb >= sizeof(shader->constants) / sizeof(*shader->constants))
	{
		fprintf(stderr, "too much constant\n");
		return false;
	}
	struct gfx_shader_constant_def *constant = &shader->constants[shader->constants_nb];
	skip_spaces(&line);
	if (!parse_bind(&line, &constant->bind))
		return false;
	skip_spaces(&line);
	if (!parse_variable_name(&line, constant->name, sizeof(constant->name)))
		return false;
	if (*line && *line != '\n')
	{
		fprintf(stderr, "invalid constant: trailling char\n");
		return false;
	}
	if (!parse_constant_members(shader, constant, fp))
		return false;
	shader->constants_nb++;
	return true;
}

static bool parse_struct_members(struct gfx_shader_struct_def *st, FILE *fp)
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
		struct gfx_shader_struct_member_def *member = &st->members[st->members_nb];
		if (st->members_nb >= sizeof(st->members) / sizeof(*st->members))
		{
			fprintf(stderr, "invalid struct: too much members\n");
			return false;
		}
		skip_spaces(&line);
		enum gfx_variable_type type;
		if (!parse_variable_type(&line, &type))
		{
			fprintf(stderr, "invalid struct: unknown member type\n");
			return false;
		}
		member->type = type;
		skip_spaces(&line);
		if (!parse_variable_name(&line, member->name, sizeof(member->name)))
			return false;
		if (*line == '[')
		{
			if (!parse_array_size(&line, &member->array_size))
				return false;
		}
		else
		{
			member->array_size = 0;
		}
		if (*line && *line != '\n')
		{
			fprintf(stderr, "invalid struct name: trailing char\n");
			return false;
		}
		st->members_nb++;
	}
	return true;
}

static bool parse_sampler(struct shader_ctx *shader, const char *line)
{
	if (shader->samplers_nb >= sizeof(shader->samplers) / sizeof(*shader->samplers))
	{
		fprintf(stderr, "too much sampler\n");
		return false;
	}
	struct gfx_shader_sampler_def *sampler = &shader->samplers[shader->samplers_nb];
	skip_spaces(&line);
	if (!parse_bind(&line, &sampler->bind))
		return false;
	skip_spaces(&line);
	enum gfx_texture_type type;
	if (!parse_texture_type(&line, &type))
		return false;
	sampler->type = type;
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

static bool parse_struct(struct shader_ctx *shader, const char *line, FILE *fp)
{
	if (shader->structs_nb >= sizeof(shader->structs) / sizeof(*shader->structs))
	{
		fprintf(stderr, "too much struct\n");
		return false;
	}
	struct gfx_shader_struct_def *st = &shader->structs[shader->structs_nb];
	skip_spaces(&line);
	if (!parse_variable_name(&line, st->name, sizeof(st->name)))
		return false;
	if (*line && *line != '\n')
	{
		fprintf(stderr, "invalid struct: trailing char\n");
		return false;
	}
	if (!parse_struct_members(st, fp))
		return false;
	shader->structs_nb++;
	return true;
}

static bool parse_code(struct shader_ctx *shader, char **line, size_t *len, FILE *fp)
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

static bool parse_shader(struct shader_ctx *shader, FILE *fp, enum gfx_shader_type type)
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

static void print_glsl_defines(struct membuf *membuf, enum gfx_device_backend backend)
{
	membuf_printf(membuf, "#define GFX_GLSL\n");
	membuf_printf(membuf, "#define mul(a, b) (b * a)\n");
	membuf_printf(membuf, "#define gfx_sample(t, u) texture(t, u)\n");
	membuf_printf(membuf, "#define gfx_sample_offset(t, u, o) textureOffset(t, u, o)\n");
	membuf_printf(membuf, "\n");
	if (backend == GFX_DEVICE_GLES3)
	{
		membuf_printf(membuf, "precision highp float;\n");
		membuf_printf(membuf, "precision highp int;\n");
		membuf_printf(membuf, "precision highp sampler2D;\n");
		membuf_printf(membuf, "precision highp sampler2DArray;\n");
		membuf_printf(membuf, "\n");
	}
}

static void print_glsl_constant(struct membuf *membuf, const struct shader_ctx *shader, const struct gfx_shader_constant_def *constant, enum gfx_device_backend backend)
{
	switch (backend)
	{
		case GFX_DEVICE_VK:
			membuf_printf(membuf, "layout(set = 0, binding = %" PRIu8 ", std140) uniform %s\n", constant->bind, constant->name);
			break;
		case GFX_DEVICE_GL4:
		case GFX_DEVICE_GLES3:
			membuf_printf(membuf, "layout(binding = %" PRIu8 ", std140) uniform %s\n", constant->bind, constant->name);
			break;
		case GFX_DEVICE_GL3:
			membuf_printf(membuf, "layout(std140) uniform %s\n", constant->name);
			break;
		default:
			assert(!"unknown backend\n");
	}
	membuf_printf(membuf, "{\n");
	for (size_t i = 0; i < constant->members_nb; ++i)
	{
		const struct gfx_shader_constant_member_def *member = &constant->members[i];
		const char *type_str;
		if (member->type == GFX_VARIABLE_STRUCT)
			type_str = shader->structs[member->struct_id].name;
		else
			type_str = variable_type_to_string(member->type);
		membuf_printf(membuf, "\t%s %s", type_str, member->name);
		if (member->array_size)
			membuf_printf(membuf, "[%" PRIu32 "]", member->array_size);
		membuf_printf(membuf, ";\n");
	}
	membuf_printf(membuf, "};\n\n");
}

static void print_glsl_struct(struct membuf *membuf, const struct gfx_shader_struct_def *st)
{
	membuf_printf(membuf, "struct %s\n", st->name);
	membuf_printf(membuf, "{\n");
	for (size_t i = 0; i < st->members_nb; ++i)
	{
		const struct gfx_shader_struct_member_def *member = &st->members[i];
		membuf_printf(membuf, "\t%s %s", variable_type_to_string(member->type), member->name);
		if (member->array_size)
			membuf_printf(membuf, "[%" PRIu32 "]", member->array_size);
		membuf_printf(membuf, ";\n");
	}
	membuf_printf(membuf, "};\n\n");
}

static void print_glsl_sampler(struct membuf *membuf, const struct gfx_shader_sampler_def *sampler, enum gfx_device_backend backend)
{
	switch (backend)
	{
		case GFX_DEVICE_VK:
			membuf_printf(membuf, "layout(set = 1, binding = %" PRIu8 ") uniform %s %s;\n", sampler->bind, sampler_type_to_string(sampler->type), sampler->name);
			break;
		case GFX_DEVICE_GL4:
		case GFX_DEVICE_GLES3:
			membuf_printf(membuf, "layout(binding = %" PRIu8 ") uniform %s %s;\n", sampler->bind, sampler_type_to_string(sampler->type), sampler->name);
			break;
		case GFX_DEVICE_GL3:
			membuf_printf(membuf, "uniform %s %s;\n", sampler_type_to_string(sampler->type), sampler->name);
			break;
		default:
			assert(!"unknown backend\n");
	}
}

static void print_glsl_version(struct membuf *membuf, enum gfx_device_backend backend)
{
	switch (backend)
	{
		case GFX_DEVICE_VK:
		case GFX_DEVICE_GL4:
			membuf_printf(membuf, "#version 450 core\n\n");
			break;
		case GFX_DEVICE_GL3:
			membuf_printf(membuf, "#version 330 core\n\n");
			break;
		case GFX_DEVICE_GLES3:
			membuf_printf(membuf, "#version 320 es\n\n");
			break;
		default:
			assert(!"unknown backend\n");
	}
}

static bool print_glsl_vs(struct membuf *membuf, const struct shader_ctx *shader, enum gfx_device_backend backend)
{
	print_glsl_version(membuf, backend);
	print_glsl_defines(membuf, backend);
	membuf_printf(membuf, "\n");
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct gfx_shader_input_def *input = &shader->inputs[i];
		membuf_printf(membuf, "layout(location = %" PRIu8 ") in %s gfx_in_vs_%s;\n", input->bind, variable_type_to_string(input->type), input->name);
	}
	membuf_printf(membuf, "\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct gfx_shader_output_def *output = &shader->outputs[i];
		switch (backend)
		{
			case GFX_DEVICE_GL3:
				membuf_printf(membuf, "%sout %s gfx_in_fs_%s;\n", output->type == GFX_VARIABLE_INT ? "flat " : "", variable_type_to_string(output->type), output->name);
				break;
			case GFX_DEVICE_GL4:
			case GFX_DEVICE_VK:
			case GFX_DEVICE_GLES3:
				membuf_printf(membuf, "layout(location = %" PRIu8 ") %sout %s gfx_in_fs_%s;\n", output->bind, output->type == GFX_VARIABLE_INT ? "flat " : "", variable_type_to_string(output->type), output->name);
				break;
			default:
				assert(!"unknown backend\n");
		}
	}
	membuf_printf(membuf, "\n");
	for (size_t i = 0; i < shader->structs_nb; ++i)
		print_glsl_struct(membuf, &shader->structs[i]);
	for (size_t i = 0; i < shader->constants_nb; ++i)
		print_glsl_constant(membuf, shader, &shader->constants[i], backend);
	membuf_printf(membuf, "struct vs_input\n");
	membuf_printf(membuf, "{\n");
	membuf_printf(membuf, "\tint vertex_id;\n");
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct gfx_shader_input_def *input = &shader->inputs[i];
		membuf_printf(membuf, "\t%s %s;\n", variable_type_to_string(input->type), input->name);
	}
	membuf_printf(membuf, "};\n");
	membuf_printf(membuf, "\n");
	membuf_printf(membuf, "struct vs_output\n");
	membuf_printf(membuf, "{\n");
	membuf_printf(membuf, "\tvec4 gfx_position;\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct gfx_shader_output_def *output = &shader->outputs[i];
		membuf_printf(membuf, "\t%s %s;\n", variable_type_to_string(output->type), output->name);
	}
	membuf_printf(membuf, "};\n");
	membuf_printf(membuf, "\n");
	membuf_printf(membuf, shader->code);
	membuf_printf(membuf, "\n");
	membuf_printf(membuf, "void main()\n");
	membuf_printf(membuf, "{\n");
	membuf_printf(membuf, "\tvs_input gfx_in;\n");
	switch (backend)
	{
		case GFX_DEVICE_GL3:
		case GFX_DEVICE_GL4:
		case GFX_DEVICE_GLES3:
			membuf_printf(membuf, "\tgfx_in.vertex_id = gl_VertexID;\n");
			break;
		case GFX_DEVICE_VK:
			membuf_printf(membuf, "\tgfx_in.vertex_id = gl_VertexIndex;\n");
			break;
		default:
			assert(!"unknown backend\n");
	}
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct gfx_shader_input_def *input = &shader->inputs[i];
		membuf_printf(membuf, "\tgfx_in.%s = gfx_in_vs_%s;\n", input->name, input->name);
	}
	membuf_printf(membuf, "\tvs_output gfx_out = gfx_main(gfx_in);\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct gfx_shader_output_def *output = &shader->outputs[i];
		membuf_printf(membuf, "\tgfx_in_fs_%s = gfx_out.%s;\n", output->name, output->name);
	}
	membuf_printf(membuf, "\tgl_Position = gfx_out.gfx_position;\n");
	membuf_printf(membuf, "}\n");
	return true;
}

static bool print_glsl_fs(struct membuf *membuf, const struct shader_ctx *shader, enum gfx_device_backend backend)
{
	print_glsl_version(membuf, backend);
	print_glsl_defines(membuf, backend);
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct gfx_shader_input_def *input = &shader->inputs[i];
		if (backend == GFX_DEVICE_GL3)
			membuf_printf(membuf, "%sin %s gfx_in_fs_%s;\n", input->type == GFX_VARIABLE_INT ? "flat " : "", variable_type_to_string(input->type), input->name);
		else
			membuf_printf(membuf, "layout(location = %" PRIu8 ") in %s%s gfx_in_fs_%s;\n", input->bind, input->type == GFX_VARIABLE_INT ? "flat " : "", variable_type_to_string(input->type), input->name);
	}
	if (shader->inputs_nb)
		membuf_printf(membuf, "\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct gfx_shader_output_def *output = &shader->outputs[i];
		membuf_printf(membuf, "layout(location = %" PRIu8 ") out %s gfx_out_fs_%s;\n", output->bind, variable_type_to_string(output->type), output->name);
	}
	membuf_printf(membuf, "\n");
	for (size_t i = 0; i < shader->structs_nb; ++i)
		print_glsl_struct(membuf, &shader->structs[i]);
	for (size_t i = 0; i < shader->constants_nb; ++i)
		print_glsl_constant(membuf, shader, &shader->constants[i], backend);
	for (size_t i = 0; i < shader->samplers_nb; ++i)
		print_glsl_sampler(membuf, &shader->samplers[i], backend);
	if (shader->samplers_nb)
		membuf_printf(membuf, "\n");
	membuf_printf(membuf, "struct fs_input\n");
	membuf_printf(membuf, "{\n");
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct gfx_shader_input_def *input = &shader->inputs[i];
		membuf_printf(membuf, "\t%s %s;\n", variable_type_to_string(input->type), input->name);
	}
	membuf_printf(membuf, "\tint dummy; /* don't make empty fs_input */\n");
	membuf_printf(membuf, "};\n");
	membuf_printf(membuf, "\n");
	membuf_printf(membuf, "struct fs_output\n");
	membuf_printf(membuf, "{\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct gfx_shader_output_def *output = &shader->outputs[i];
		membuf_printf(membuf, "\t%s %s;\n", variable_type_to_string(output->type), output->name);
	}
	membuf_printf(membuf, "};\n");
	membuf_printf(membuf, "\n");
	membuf_printf(membuf, shader->code);
	membuf_printf(membuf, "\n");
	membuf_printf(membuf, "void main()\n");
	membuf_printf(membuf, "{\n");
	membuf_printf(membuf, "\tfs_input gfx_in;\n");
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct gfx_shader_input_def *input = &shader->inputs[i];
		membuf_printf(membuf, "\tgfx_in.%s = gfx_in_fs_%s;\n", input->name, input->name);
	}
	membuf_printf(membuf, "\tfs_output gfx_out = gfx_main(gfx_in);\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct gfx_shader_output_def *output = &shader->outputs[i];
		membuf_printf(membuf, "\tgfx_out_fs_%s = gfx_out.%s;\n", output->name, output->name);
	}
	membuf_printf(membuf, "}\n");
	return true;
}

static void print_hlsl_defines(struct membuf *membuf)
{
	membuf_printf(membuf, "#define GFX_HLSL\n");
	membuf_printf(membuf, "#define vec1 float1\n");
	membuf_printf(membuf, "#define vec2 float2\n");
	membuf_printf(membuf, "#define vec3 float3\n");
	membuf_printf(membuf, "#define vec4 float4\n");
	membuf_printf(membuf, "#define ivec1 int1\n");
	membuf_printf(membuf, "#define ivec2 int2\n");
	membuf_printf(membuf, "#define ivec3 int3\n");
	membuf_printf(membuf, "#define ivec4 int4\n");
	membuf_printf(membuf, "#define mat1 float1x1\n");
	membuf_printf(membuf, "#define mat2 float2x2\n");
	membuf_printf(membuf, "#define mat3 float3x3\n");
	membuf_printf(membuf, "#define mat4 float4x4\n");
	membuf_printf(membuf, "\n");
	membuf_printf(membuf, "#define sampler2D Texture2D\n");
	membuf_printf(membuf, "#define sampler2DArray Texture2DArray\n");
	membuf_printf(membuf, "#define sampler3D Texture3D\n");
	membuf_printf(membuf, "\n");
	membuf_printf(membuf, "#define gfx_sample(name, uv) name.Sample(name##_sampler, uv)\n");
	membuf_printf(membuf, "#define gfx_sample_offset(name, uv, offset) name.Sample(name##_sampler, uv, offset)\n");
	membuf_printf(membuf, "\n");
	membuf_printf(membuf, "#define mix(a, b, v) lerp(a, b, v)\n");
	membuf_printf(membuf, "#define mod(a, b) fmod(a, b)\n");
}

static void print_d3d11_constant(struct membuf *membuf, const struct shader_ctx *shader, const struct gfx_shader_constant_def *constant)
{
	membuf_printf(membuf, "cbuffer %s : register(b%" PRIu8 ")\n", constant->name, constant->bind);
	membuf_printf(membuf, "{\n");
	for (size_t i = 0; i < constant->members_nb; ++i)
	{
		const struct gfx_shader_constant_member_def *member = &constant->members[i];
		const char *type_str;
		if (member->type == GFX_VARIABLE_STRUCT)
			type_str = shader->structs[member->struct_id].name;
		else
			type_str = variable_type_to_string(member->type);
		membuf_printf(membuf, "\t%s %s", type_str, member->name);
		if (member->array_size)
			membuf_printf(membuf, "[%" PRIu32 "]", member->array_size);
		membuf_printf(membuf, ";\n");
	}
	membuf_printf(membuf, "};\n\n");
}

static void print_d3d11_sampler(struct membuf *membuf, const struct gfx_shader_sampler_def *sampler)
{
	membuf_printf(membuf, "%s %s : register(t%" PRIu8 ");\n", sampler_type_to_string(sampler->type), sampler->name, sampler->bind);
	membuf_printf(membuf, "SamplerState %s_sampler : register(s%" PRIu8 ");\n", sampler->name, sampler->bind);
}

static void print_d3d11_struct(struct membuf *membuf, const struct gfx_shader_struct_def *st)
{
	membuf_printf(membuf, "struct %s\n", st->name);
	membuf_printf(membuf, "{\n");
	for (size_t i = 0; i < st->members_nb; ++i)
	{
		const struct gfx_shader_struct_member_def *member = &st->members[i];
		membuf_printf(membuf, "\t%s %s", variable_type_to_string(member->type), member->name);
		if (member->array_size)
			membuf_printf(membuf, "[%" PRIu32 "]", member->array_size);
		membuf_printf(membuf, ";\n");
	}
	membuf_printf(membuf, "};\n\n");
}

static bool print_d3d11_vs(struct membuf *membuf, const struct shader_ctx *shader)
{
	print_hlsl_defines(membuf);
	membuf_printf(membuf, "\n");
	membuf_printf(membuf, "struct vs_input\n");
	membuf_printf(membuf, "{\n");
	membuf_printf(membuf, "\tuint vertex_id : SV_VertexID;\n");
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct gfx_shader_input_def *input = &shader->inputs[i];
		membuf_printf(membuf, "\t%s %s : VS_INPUT%" PRIu8 ";\n", variable_type_to_string(input->type), input->name, input->bind);
	}
	membuf_printf(membuf, "};\n");
	membuf_printf(membuf, "\n");
	membuf_printf(membuf, "struct vs_output\n");
	membuf_printf(membuf, "{\n");
	membuf_printf(membuf, "\tvec4 gfx_position : SV_POSITION;\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct gfx_shader_output_def *output = &shader->outputs[i];
		membuf_printf(membuf, "\t%s%s %s : FS_INPUT%" PRIu8 ";\n", output->type == GFX_VARIABLE_INT ? "nointerpolation " : "", variable_type_to_string(output->type), output->name, output->bind);
	}
	membuf_printf(membuf, "};\n");
	membuf_printf(membuf, "\n");
	for (size_t i = 0; i < shader->structs_nb; ++i)
		print_d3d11_struct(membuf, &shader->structs[i]);
	for (size_t i = 0; i < shader->constants_nb; ++i)
		print_d3d11_constant(membuf, shader, &shader->constants[i]);
	membuf_printf(membuf, shader->code);
	membuf_printf(membuf, "\n");
	membuf_printf(membuf, "vs_output main(vs_input input)\n");
	membuf_printf(membuf, "{\n");
	membuf_printf(membuf, "\treturn gfx_main(input);\n");
	membuf_printf(membuf, "}\n");
	return true;
}

static bool print_d3d11_fs(struct membuf *membuf, const struct shader_ctx *shader)
{
	print_hlsl_defines(membuf);
	membuf_printf(membuf, "\n");
	membuf_printf(membuf, "struct fs_input\n");
	membuf_printf(membuf, "{\n");
	membuf_printf(membuf, "\tvec4 gfx_position : SV_POSITION;\n");
	for (size_t i = 0; i < shader->inputs_nb; ++i)
	{
		const struct gfx_shader_input_def *input = &shader->inputs[i];
		membuf_printf(membuf, "\t%s%s %s : FS_INPUT%" PRIu8 ";\n", input->type == GFX_VARIABLE_INT ? "nointerpolation " : "", variable_type_to_string(input->type), input->name, input->bind);
	}
	membuf_printf(membuf, "};\n");
	membuf_printf(membuf, "\n");
	membuf_printf(membuf, "struct fs_output\n");
	membuf_printf(membuf, "{\n");
	for (size_t i = 0; i < shader->outputs_nb; ++i)
	{
		const struct gfx_shader_output_def *output = &shader->outputs[i];
		membuf_printf(membuf, "\t%s %s : SV_TARGET%" PRIu8 ";\n", variable_type_to_string(output->type), output->name, output->bind);
	}
	membuf_printf(membuf, "};\n");
	membuf_printf(membuf, "\n");
	for (size_t i = 0; i < shader->structs_nb; ++i)
		print_d3d11_struct(membuf, &shader->structs[i]);
	for (size_t i = 0; i < shader->constants_nb; ++i)
		print_d3d11_constant(membuf, shader, &shader->constants[i]);
	for (size_t i = 0; i < shader->samplers_nb; ++i)
		print_d3d11_sampler(membuf, &shader->samplers[i]);
	if (shader->samplers_nb)
		membuf_printf(membuf, "\n");
	membuf_printf(membuf, shader->code);
	membuf_printf(membuf, "\n");
	membuf_printf(membuf, "fs_output main(fs_input input)\n");
	membuf_printf(membuf, "{\n");
	membuf_printf(membuf, "\treturn gfx_main(input);\n");
	membuf_printf(membuf, "}\n");
	return true;
}

static bool print_shader(struct membuf *membuf, struct shader_ctx *shader, enum gfx_device_backend backend)
{
	switch (backend)
	{
		case GFX_DEVICE_GL3:
		case GFX_DEVICE_GL4:
		case GFX_DEVICE_GLES3:
		case GFX_DEVICE_VK:
			switch (shader->type)
			{
				case GFX_SHADER_VERTEX:
					return print_glsl_vs(membuf, shader, backend);
				case GFX_SHADER_FRAGMENT:
					return print_glsl_fs(membuf, shader, backend);
				default:
					fprintf(stderr, "unknown shader type\n");
					return false;
			}
			break;
		case GFX_DEVICE_D3D11:
			switch (shader->type)
			{
				case GFX_SHADER_VERTEX:
					return print_d3d11_vs(membuf, shader);
				case GFX_SHADER_FRAGMENT:
					return print_d3d11_fs(membuf, shader);
				default:
					fprintf(stderr, "unknown shader type\n");
					return false;
			}
			break;
		default:
			fprintf(stderr, "unknown backend\n");
			return false;
	}
	return true;
}

static bool convert_vk(struct membuf *membuf, struct shader_ctx *shader)
{
	int in_fd = -1;
	int out_fd = -1;
	char in[256] = "/tmp/gfx_input_XXXXXX";
	char out[256] = "/tmp/gfx_output_XXXXXX";
	char command[1024];
	const char *prog = getenv("GLSLANG_VALIDATOR");
	if (!prog)
		prog = "glslangValidator";
	bool ret = false;
	const char *type;
	switch (shader->type)
	{
		case GFX_SHADER_VERTEX:
			type = "vert";
			break;
		case GFX_SHADER_FRAGMENT:
			type = "frag";
			break;
		default:
			fprintf(stderr, "unknown shader type\n");
			return false;
	}
	in_fd = mkstemp(in);
	if (in_fd == -1)
	{
		fprintf(stderr, "failed to create temp in file: %s\n", strerror(errno));
		goto err;
	}
	out_fd = mkstemp(out);
	if (out_fd == -1)
	{
		fprintf(stderr, "failed to create temp out file: %s\n", strerror(errno));
		goto err;
	}
	if (write(in_fd, membuf->data, membuf->size) != membuf->size)
	{
		fprintf(stderr, "failed to write glslangValidator input file\n");
		goto err;
	}
	snprintf(command, sizeof(command), "%s -V -S %s -o \"%s\" \"%s\"", prog, type, out, in);
	if (system(command))
	{
		fprintf(stderr, "glslangValidator call failed\n");
		goto err;
	}
	free(membuf->data);
	membuf->data = NULL;
	membuf->size = 0;
	if (!membuf_read_file(membuf, out_fd))
		goto err;
	ret = true;

err:
	if (in_fd != -1)
	{
		close(in_fd);
		unlink(in);
	}
	if (out_fd != -1)
	{
		close(out_fd);
		unlink(out);
	}
	return ret;
}

static bool convert_shader(struct membuf *membuf, struct shader_ctx *shader, enum gfx_device_backend backend)
{
	switch (backend)
	{
		case GFX_DEVICE_VK:
			return convert_vk(membuf, shader);
		default:
			break;
	}
	return true;
}

static bool print_file(const char *path, struct shader_ctx *ctx, struct membuf *membufs)
{
	FILE *fp = fopen(path, "w");
	if (!fp)
	{
		fprintf(stderr, "failed to open output file\n");
		return false;
	}
	struct gfx_shader_def shader_def;
	if (ctx->type == GFX_SHADER_VERTEX)
		shader_def.magic = GFX_MAGIC_VERTEX;
	else
		shader_def.magic = GFX_MAGIC_FRAGMENT;
	shader_def.outputs_count = ctx->outputs_nb;
	shader_def.inputs_count = ctx->inputs_nb;
	shader_def.structs_count = ctx->structs_nb;
	shader_def.samplers_count = ctx->samplers_nb;
	shader_def.constants_count = ctx->constants_nb;
	size_t tmp = sizeof(shader_def);
	tmp+= sizeof(*ctx->inputs) * ctx->inputs_nb;
	tmp+= sizeof(*ctx->outputs) * ctx->outputs_nb;
	tmp+= sizeof(*ctx->structs) * ctx->structs_nb;
	tmp+= sizeof(*ctx->samplers) * ctx->samplers_nb;
	tmp+= sizeof(*ctx->constants) * ctx->constants_nb;
	for (size_t i = 0; i < 8; ++i)
	{
		if (!membufs[i].size)
		{
			shader_def.codes_lengths[i] = 0;
			shader_def.codes_offsets[i] = 0;
			continue;
		}
		shader_def.codes_lengths[i] = membufs[i].size;
		shader_def.codes_offsets[i] = tmp;
		tmp += membufs[i].size;
	}
	fwrite(&shader_def, 1, sizeof(shader_def), fp);
	fwrite(ctx->inputs, sizeof(*ctx->inputs), ctx->inputs_nb, fp);
	fwrite(ctx->outputs, sizeof(*ctx->outputs), ctx->outputs_nb, fp);
	fwrite(ctx->structs, sizeof(*ctx->structs), ctx->structs_nb, fp);
	fwrite(ctx->samplers, sizeof(*ctx->samplers), ctx->samplers_nb, fp);
	fwrite(ctx->constants, sizeof(*ctx->constants), ctx->constants_nb, fp);
	for (size_t i = 0; i < 8; ++i)
	{
		if (!membufs[i].size)
			continue;
		fwrite(membufs[i].data, 1, membufs[i].size, fp);
	}
	fclose(fp);
	return true;
}

static void usage()
{
	printf("compile -t <type> -x <backend> -i <input> -o <output>\n");
	printf("-t: shader type (vs, fs)\n");
	printf("-x: backend list, coma-separated (gl3, gl4, gles3, vk, d3d9, d3d11)\n");
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
	const char *backend = NULL;
	enum gfx_shader_type shader_type;
	uint32_t backend_mask = 0;
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
				backend = optarg;
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
	if (!backend)
	{
		fprintf(stderr, "no backend given\n");
		usage();
		return EXIT_FAILURE;
	}
	if (!shader_type_from_string(type, &shader_type))
	{
		fprintf(stderr, "unknown shader type\n");
		usage();
		return EXIT_FAILURE;
	}
	if (!shader_backend_mask_from_string(backend, &backend_mask))
	{
		fprintf(stderr, "unknown shader backend\n");
		usage();
		return EXIT_FAILURE;
	}
	FILE *fp = fopen(in, "r");
	if (!fp)
	{
		fprintf(stderr, "failed to open %s\n", in);
		return EXIT_FAILURE;
	}
	struct shader_ctx shader;
	memset(&shader, 0, sizeof(shader));
	if (!parse_shader(&shader, fp, shader_type))
	{
		fprintf(stderr, "failed to parse shader\n");
		fclose(fp);
		return EXIT_FAILURE;
	}
	fclose(fp);

	struct membuf membufs[8];
	memset(membufs, 0, sizeof(membufs));
	for (size_t i = 0; i < 8; ++i)
	{
		if (!(backend_mask & (1 << i)))
			continue;
		print_shader(&membufs[i], &shader, i);
	}
	for (size_t i = 0; i < 8; ++i)
	{
		if (!(backend_mask & (1 << i)))
			continue;
		convert_shader(&membufs[i], &shader, i);
	}
	if (!print_file(out, &shader, membufs))
		return EXIT_FAILURE;
	for (size_t i = 0; i < 8; ++i)
		free(membufs[i].data);
	return EXIT_SUCCESS;
}
