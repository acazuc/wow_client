#ifndef UI_DIMENSION_H
#define UI_DIMENSION_H

#include "itf/enum.h"

#include <jks/optional.h>

#include <stdint.h>

#ifdef interface
# undef interface
#endif

struct xml_dimension;

struct ui_dimension
{
	enum dimension_type type;
	union
	{
		struct
		{
			int32_t x;
			int32_t y;
		} abs;
		struct
		{
			float x;
			float y;
		} rel;
	};
};

OPTIONAL_DEF(optional_ui_dimension, struct ui_dimension);

void ui_dimension_init_xml(struct ui_dimension *dimension, const struct xml_dimension *xml);
void ui_dimension_init(struct ui_dimension *dimension, int32_t x, int32_t y);
bool ui_dimension_eq(const struct ui_dimension *d1, const struct ui_dimension *d2);

#endif
