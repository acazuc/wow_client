#ifndef DBC_H
#define DBC_H

#include <libwow/dbc.h>

#include <jks/hmap.h>

#include <stdint.h>

struct dbc
{
	struct jks_hmap index;
	struct wow_dbc_file *file;
	size_t refcount;
};

struct dbc *dbc_new(struct wow_dbc_file *file);
void dbc_delete(struct dbc *dbc);
struct wow_dbc_row dbc_get_row(const struct dbc *dbc, uint32_t row);
bool dbc_get_row_indexed_str(struct dbc *dbc, struct wow_dbc_row *row, const char *index);
bool dbc_get_row_indexed(struct dbc *dbc, struct wow_dbc_row *row, uint32_t index);
bool dbc_set_index(struct dbc *dbc, uint32_t column_offset, bool string_index);
void dbc_dump(const struct dbc *dbc);
void dbc_dump_def(const struct dbc *dbc, const struct wow_dbc_def *def);

#endif
