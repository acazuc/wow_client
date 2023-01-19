#ifndef UI_COLOR_H
#define UI_COLOR_H

#include <jks/optional.h>

#ifdef interface
# undef interface
#endif

struct xml_color;

struct ui_color
{
	union { float x, r; };
	union { float y, g; };
	union { float z, b; };
	union { float w, a; };
};

OPTIONAL_DEF(optional_ui_color, struct ui_color);

void ui_color_init_xml(struct ui_color *color, const struct xml_color *xml);
void ui_color_init(struct ui_color *color, float r, float g, float b, float a);
bool ui_color_eq(const struct ui_color *c1, const struct ui_color *c2);

#endif
