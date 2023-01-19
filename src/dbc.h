#ifndef DBC_H
#define DBC_H

#include <libwow/dbc.h>

#include <jks/hmap.h>

#include <stdint.h>

struct dbc
{
	struct jks_hmap index;
	wow_dbc_file_t *file;
	size_t refcount;
};

struct dbc *dbc_new(wow_dbc_file_t *file);
void dbc_delete(struct dbc *dbc);
wow_dbc_row_t dbc_get_row(const struct dbc *dbc, uint32_t row);
bool dbc_get_row_indexed_str(struct dbc *dbc, wow_dbc_row_t *row, const char *index);
bool dbc_get_row_indexed(struct dbc *dbc, wow_dbc_row_t *row, uint32_t index);
bool dbc_set_index(struct dbc *dbc, uint32_t column_offset, bool string_index);
void dbc_dump(const struct dbc *dbc);
void dbc_dump_def(const struct dbc *dbc, const wow_dbc_def_t *def);

#endif
