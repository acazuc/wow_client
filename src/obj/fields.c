#include "fields.h"

#include "memory.h"
#include "log.h"

#include <inttypes.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>

MEMORY_DECL(OBJ);

void object_fields_init(struct object_fields *fields)
{
	jks_array_init(&fields->fields, sizeof(uint32_t), NULL, &jks_array_memory_fn_OBJ);
	jks_array_init(&fields->bit_mask, sizeof(uint32_t), NULL, &jks_array_memory_fn_OBJ);
}

void object_fields_destroy(struct object_fields *fields)
{
	jks_array_destroy(&fields->fields);
	jks_array_destroy(&fields->bit_mask);
}

bool object_fields_resize(struct object_fields *fields, uint32_t size)
{
	size_t old_size = fields->fields.size;
	if (!jks_array_resize(&fields->fields, size))
	{
		LOG_ERROR("failed to resize fields to %" PRIu32, size);
		return false;
	}
	memset(JKS_ARRAY_GET(&fields->fields, old_size, uint32_t), 0, (fields->fields.size - old_size) * sizeof(uint32_t));
	old_size = fields->bit_mask.size;
	if (!jks_array_resize(&fields->bit_mask, (size + 7) / 8))
	{
		LOG_ERROR("failed to resize fields bitmask to %" PRIu32, size);
		return false;
	}
	memset(JKS_ARRAY_GET(&fields->bit_mask, old_size, uint32_t), 0, (fields->bit_mask.size - old_size) * sizeof(uint32_t));
	return true;
}

bool object_fields_get_bit(const struct object_fields *fields, uint32_t field)
{
	assert(field < fields->fields.size);
	return *JKS_ARRAY_GET(&fields->bit_mask, field / 8, uint32_t) & (1 << (field & 8));
}

void object_fields_enable_bit(struct object_fields *fields, uint32_t field)
{
	assert(field < fields->fields.size);
	*JKS_ARRAY_GET(&fields->bit_mask, field / 8, uint32_t) |= (1 << (field & 8));
}

void object_fields_disable_bit(struct object_fields *fields, uint32_t field)
{
	assert(field < fields->fields.size);
	*JKS_ARRAY_GET(&fields->bit_mask, field / 8, uint32_t) &= ~(1 << (field & 8));
}

int32_t object_fields_get_i32(const struct object_fields *fields, uint32_t field)
{
	assert(field < fields->fields.size);
	return *JKS_ARRAY_GET(&fields->fields, field, int32_t);
}

void object_fields_set_i32(struct object_fields *fields, uint32_t field, int32_t value)
{
	assert(field < fields->fields.size);
	*JKS_ARRAY_GET(&fields->fields, field, int32_t) = value;
	object_fields_enable_bit(fields, field);
}

uint32_t object_fields_get_u32(const struct object_fields *fields, uint32_t field)
{
	assert(field < fields->fields.size);
	return *JKS_ARRAY_GET(&fields->fields, field, uint32_t);
}

void object_fields_set_u32(struct object_fields *fields, uint32_t field, uint32_t value)
{
	assert(field < fields->fields.size);
	*JKS_ARRAY_GET(&fields->fields, field, uint32_t) = value;
	object_fields_enable_bit(fields, field);
}

int64_t object_fields_get_i64(const struct object_fields *fields, uint32_t field)
{
	assert(field < fields->fields.size - 1);
	return *JKS_ARRAY_GET(&fields->fields, field, int32_t) | ((int64_t)*JKS_ARRAY_GET(&fields->fields, field + 1, int32_t) << 32);
}

void object_fields_set_i64(struct object_fields *fields, uint32_t field, int64_t value)
{
	assert(field < fields->fields.size - 1);
	*JKS_ARRAY_GET(&fields->fields, field, int32_t) = value;
	*JKS_ARRAY_GET(&fields->fields, field + 1, int32_t) = value >> 32;
	object_fields_enable_bit(fields, field);
	object_fields_enable_bit(fields, field + 1);
}

uint64_t object_fields_get_u64(const struct object_fields *fields, uint32_t field)
{
	assert(field < fields->fields.size - 1);
	return *JKS_ARRAY_GET(&fields->fields, field, uint32_t) | ((uint64_t)*JKS_ARRAY_GET(&fields->fields, field + 1, uint32_t) << 32);
}

void object_fields_set_u64(struct object_fields *fields, uint32_t field, uint64_t value)
{
	assert(field < fields->fields.size - 1);
	*JKS_ARRAY_GET(&fields->fields, field, uint32_t) = value;
	*JKS_ARRAY_GET(&fields->fields, field + 1, uint32_t) = value >> 32;
	object_fields_enable_bit(fields, field);
	object_fields_enable_bit(fields, field + 1);
}

float object_fields_get_flt(const struct object_fields *fields, uint32_t field)
{
	assert(field < fields->fields.size);
	union
	{
		float f;
		uint32_t u;
	} val;
	val.u = *JKS_ARRAY_GET(&fields->fields, field, uint32_t);
	return val.f;
}

void object_fields_set_flt(struct object_fields *fields, uint32_t field, float value)
{
	assert(field < fields->fields.size);
	union
	{
		float f;
		uint32_t u;
	} val;
	val.f = value;
	*JKS_ARRAY_GET(&fields->fields, field, uint32_t) = val.u;
	object_fields_enable_bit(fields, field);
}
