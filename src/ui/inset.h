#ifndef UI_INSET_H
#define UI_INSET_H

#include "itf/enum.h"

#include <jks/optional.h>

#include <stdint.h>

#ifdef interface
# undef interface
#endif

struct xml_inset;

struct ui_inset
{
	enum inset_type type;
	union
	{
		struct
		{
			int32_t left;
			int32_t right;
			int32_t top;
			int32_t bottom;
		} abs;
		struct
		{
			float left;
			float right;
			float top;
			float bottom;
		} rel;
	};
};

OPTIONAL_DEF(optional_ui_inset, struct ui_inset);

void ui_inset_init_xml(struct ui_inset *inset, const struct xml_inset *xml);
void ui_inset_init(struct ui_inset *inset, int32_t left, int32_t right, int32_t top, int32_t bottom);

#endif
