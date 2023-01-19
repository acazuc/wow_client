#ifndef XML_GRADIENT_H
#define XML_GRADIENT_H

#include "xml/element.h"
#include "xml/color.h"

struct xml_gradient
{
	struct xml_element element;
	struct optional_xml_color min_color;
	struct optional_xml_color max_color;
	char *orientation;
};

OPTIONAL_DEF(optional_xml_gradient, struct xml_gradient);

extern const struct xml_vtable xml_gradient_vtable;

#endif
