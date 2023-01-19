#ifndef UI_GRADIENT_H
#define UI_GRADIENT_H

#include "itf/enum.h"

#include "ui/color.h"

#include <jks/optional.h>

#ifdef interface
# undef interface
#endif

struct xml_gradient;

struct ui_gradient
{
	struct ui_color min_color;
	struct ui_color max_color;
	enum orientation orientation;
};

OPTIONAL_DEF(optional_ui_gradient, struct ui_gradient);

void ui_gradient_init_xml(struct ui_gradient *gradient, const struct xml_gradient *xml);
void ui_gradient_init(struct ui_gradient *gradient);

#endif
