#ifndef UI_VALUE_H
#define UI_VALUE_H

#include "itf/enum.h"

#include <jks/optional.h>

#include <stdint.h>

#ifdef interface
# undef interface
#endif

struct xml_value;

struct ui_value
{
	enum value_type type;
	union
	{
		int32_t abs;
		float rel;
	};
};

OPTIONAL_DEF(optional_ui_value, struct ui_value);

void ui_value_init_xml(struct ui_value *value, const struct xml_value *xml);
void ui_value_init(struct ui_value *value, int32_t val);

#endif
