#include "dbc.h"

#include "memory.h"
#include "log.h"

#include <inttypes.h>
#include <stdio.h>

MEMORY_DECL(DBC);

struct dbc *dbc_new(struct wow_dbc_file *file)
{
	struct dbc *dbc = mem_malloc(MEM_DBC, sizeof(*dbc));
	if (!dbc)
		return NULL;
	dbc->file = file;
	dbc->refcount = 1;
	jks_hmap_init(&dbc->index, sizeof(uint32_t), NULL, jks_hmap_hash_u32, jks_hmap_cmp_u32, &jks_hmap_memory_fn_DBC);
	return dbc;
}

void dbc_delete(struct dbc *dbc)
{
	jks_hmap_destroy(&dbc->index);
	wow_dbc_file_delete(dbc->file);
	mem_free(MEM_DBC, dbc);
}

struct wow_dbc_row dbc_get_row(const struct dbc *dbc, uint32_t row)
{
	return wow_dbc_get_row(dbc->file, row);
}

bool dbc_get_row_indexed_str(struct dbc *dbc, struct wow_dbc_row *row, const char *index)
{
	uint32_t *data = jks_hmap_get(&dbc->index, JKS_HMAP_KEY_STR((char*)index));
	if (!data)
		return false;
	*row = dbc_get_row(dbc, *data);
	return true;
}

bool dbc_get_row_indexed(struct dbc *dbc, struct wow_dbc_row *row, uint32_t index)
{
	uint32_t *data = jks_hmap_get(&dbc->index, JKS_HMAP_KEY_U32(index));
	if (!data)
		return false;
	*row = dbc_get_row(dbc, *data);
	return true;
}

bool dbc_set_index(struct dbc *dbc, uint32_t column_offset, bool string_index)
{
	jks_hmap_destroy(&dbc->index);
	if (string_index)
		jks_hmap_init(&dbc->index, sizeof(uint32_t), NULL, jks_hmap_hash_str, jks_hmap_cmp_str, &jks_hmap_memory_fn_DBC);
	else
		jks_hmap_init(&dbc->index, sizeof(uint32_t), NULL, jks_hmap_hash_u32, jks_hmap_cmp_u32, &jks_hmap_memory_fn_DBC);
	if (!jks_hmap_reserve(&dbc->index, dbc->file->header.record_count * 4))
		return false;
	if (string_index)
	{
		for (uint32_t i = 0; i < dbc->file->header.record_count; ++i)
		{
			struct wow_dbc_row row = wow_dbc_get_row(dbc->file, i);
			if (!jks_hmap_set(&dbc->index, JKS_HMAP_KEY_STR((char*)wow_dbc_get_str(&row, column_offset)), &i))
				return false;
		}
	}
	else
	{
		for (uint32_t i = 0; i < dbc->file->header.record_count; ++i)
		{
			if (!jks_hmap_set(&dbc->index, JKS_HMAP_KEY_U32(*(uint32_t*)(dbc->file->data + dbc->file->header.record_size * i + column_offset)), &i))
				return false;
		}
	}
	return true;
}

void dbc_dump(const struct dbc *dbc)
{
	printf("strings offset: %" PRId32 "\n", (uint32_t)(sizeof(dbc->file->header) + dbc->file->header.field_count * dbc->file->header.record_count * 4));
	for (uint32_t i = 0; i < dbc->file->header.field_count; ++i)
		printf("%12" PRId32 " | ", i);
	printf("\n");
	for (uint32_t i = 0; i < dbc->file->header.field_count; ++i)
		printf("------------ | ");
	printf("\n");
	for (uint32_t i = 0; i < dbc->file->header.record_count; ++i)
	{
		struct wow_dbc_row row = dbc_get_row(dbc, i);
		for (uint32_t j = 0; j < dbc->file->header.field_count; ++j)
			printf("%12" PRId32 " | ", wow_dbc_get_u32(&row, j * 4));
		printf("\n");
	}
}

void dbc_dump_def(const struct dbc *dbc, const struct wow_dbc_def *def)
{
	for (size_t i = 0; def[i].type != WOW_DBC_TYPE_END; ++i)
	{
		if (i)
			printf(" | ");
		printf("%12.12s", def[i].name);
	}
	printf("\n");
	for (size_t i = 0; def[i].type != WOW_DBC_TYPE_END; ++i)
	{
		if (i)
			printf(" | ");
		printf("------------");
	}
	printf("\n");
	for (size_t row_id = 0; row_id < dbc->file->header.record_count; ++row_id)
	{
		struct wow_dbc_row row = dbc_get_row(dbc, row_id);
		size_t offset = 0;
		for (size_t i = 0; def[i].type != WOW_DBC_TYPE_END; ++i)
		{
			if (i)
				printf(" | ");
			switch (def[i].type)
			{
				case WOW_DBC_TYPE_I8:
					printf("%12" PRId8, wow_dbc_get_i8(&row, offset));
					offset++;
					break;
				case WOW_DBC_TYPE_U8:
					printf("%12" PRIu8, wow_dbc_get_u8(&row, offset));
					offset++;
					break;
				case WOW_DBC_TYPE_I16:
					printf("%12" PRId16, wow_dbc_get_i16(&row, offset));
					offset += 2;
					break;
				case WOW_DBC_TYPE_U16:
					printf("%12" PRIu16, wow_dbc_get_u16(&row, offset));
					offset += 2;
					break;
				case WOW_DBC_TYPE_I32:
					printf("%12" PRId32, wow_dbc_get_i32(&row, offset));
					offset += 4;
					break;
				case WOW_DBC_TYPE_U32:
					printf("%12" PRIu32, wow_dbc_get_u32(&row, offset));
					offset += 4;
					break;
				case WOW_DBC_TYPE_I64:
					printf("%12" PRId64, wow_dbc_get_i64(&row, offset));
					offset += 8;
					break;
				case WOW_DBC_TYPE_U64:
					printf("%12" PRIu64, wow_dbc_get_u64(&row, offset));
					offset += 8;
					break;
				case WOW_DBC_TYPE_STR:
					printf("%12.12s", wow_dbc_get_str(&row, offset));
					offset += 4;
					break;
				case WOW_DBC_TYPE_LSTR:
					printf("%12.12s", wow_dbc_get_str(&row, offset + 8));
					offset += 4 * 17;
					break;
				case WOW_DBC_TYPE_FLT:
					printf("%12f", wow_dbc_get_flt(&row, offset));
					offset += 4;
					break;
				case WOW_DBC_TYPE_END:
					break;
			}
		}
		printf("\n");
	}
}
