#ifndef UI_SHADOW_H
#define UI_SHADOW_H

#include "ui/dimension.h"
#include "ui/color.h"

#include <jks/optional.h>

#ifdef interface
# undef interface
#endif

struct xml_shadow;

struct ui_shadow
{
	struct optional_ui_color color;
	struct optional_ui_dimension offset;
};

OPTIONAL_DEF(optional_ui_shadow, struct ui_shadow);

void ui_shadow_init_xml(struct ui_shadow *shadow, const struct xml_shadow *xml);
void ui_shadow_init(struct ui_shadow *shadow, const struct ui_color *color, const struct ui_dimension *offset);
bool ui_shadow_eq(const struct ui_shadow *s1, const struct ui_shadow *s2);

#endif
