#ifndef GFX_COMPILE_H
#define GFX_COMPILE_H

#include <gfx/objects.h>
#include <gfx/device.h>
#include <gfx/window.h>

#define GFX_MAGIC(a, b, c, d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))
#define GFX_MAGIC_VERTEX   GFX_MAGIC('G', 'F', 'X', 'V')
#define GFX_MAGIC_FRAGMENT GFX_MAGIC('G', 'F', 'X', 'F')

enum gfx_variable_type
{
	GFX_VARIABLE_FLOAT,
	GFX_VARIABLE_VEC1,
	GFX_VARIABLE_VEC2,
	GFX_VARIABLE_VEC3,
	GFX_VARIABLE_VEC4,
	GFX_VARIABLE_MAT1,
	GFX_VARIABLE_MAT2,
	GFX_VARIABLE_MAT3,
	GFX_VARIABLE_MAT4,
	GFX_VARIABLE_INT,
	GFX_VARIABLE_IVEC1,
	GFX_VARIABLE_IVEC2,
	GFX_VARIABLE_IVEC3,
	GFX_VARIABLE_IVEC4,
	GFX_VARIABLE_STRUCT,
};

struct gfx_shader_constant_member_def
{
	char name[64];
	uint16_t array_size;
	uint8_t struct_id;
	uint8_t type;
};

struct gfx_shader_constant_def
{
	char name[64];
	struct gfx_shader_constant_member_def members[64];
	uint32_t members_nb;
	uint8_t bind;
};

struct gfx_shader_sampler_def
{
	char name[64];
	uint8_t type;
	uint8_t bind;
};

struct gfx_shader_output_def
{
	char name[64];
	uint8_t type;
	uint8_t bind;
};

struct gfx_shader_input_def
{
	char name[64];
	uint8_t type;
	uint8_t bind;
};

struct gfx_shader_struct_member_def
{
	char name[64];
	uint16_t array_size;
	uint8_t type;
};

struct gfx_shader_struct_def
{
	char name[64];
	struct gfx_shader_struct_member_def members[64];
	uint32_t members_nb;
};

struct gfx_shader_def
{
	uint32_t magic;
	uint8_t inputs_count;
	uint8_t outputs_count;
	uint8_t structs_count;
	uint8_t constants_count;
	uint8_t samplers_count;
	uint32_t codes_lengths[8];
	uint32_t codes_offsets[8];
};

#endif
