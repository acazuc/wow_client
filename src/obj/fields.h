#ifndef OBJECT_FIELDS_H
#define OBJECT_FIELDS_H

#include <jks/array.h>

#include <stdint.h>

struct object_fields
{
	struct jks_array fields; /* uint32_t */
	struct jks_array bit_mask; /* uint32_t */
};

void object_fields_init(struct object_fields *fields);
void object_fields_destroy(struct object_fields *fields);
bool object_fields_resize(struct object_fields *fields, uint32_t size);
bool object_fields_get_bit(const struct object_fields *fields, uint32_t field);
void object_fields_enable_bit(struct object_fields *fields, uint32_t field);
void object_fields_disable_bit(struct object_fields *fields, uint32_t field);
int32_t  object_fields_get_i32(const struct object_fields *fields, uint32_t field);
uint32_t object_fields_get_u32(const struct object_fields *fields, uint32_t field);
int64_t  object_fields_get_i64(const struct object_fields *fields, uint32_t field);
uint64_t object_fields_get_u64(const struct object_fields *fields, uint32_t field);
float    object_fields_get_flt(const struct object_fields *fields, uint32_t field);
void object_fields_set_i32(struct object_fields *fields, uint32_t field, int32_t value);
void object_fields_set_u32(struct object_fields *fields, uint32_t field, uint32_t value);
void object_fields_set_i64(struct object_fields *fields, uint32_t field, int64_t value);
void object_fields_set_u64(struct object_fields *fields, uint32_t field, uint64_t value);
void object_fields_set_flt(struct object_fields *fields, uint32_t field, float value);

#endif
