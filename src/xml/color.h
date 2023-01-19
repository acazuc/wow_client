#ifndef XML_COLOR_H
#define XML_COLOR_H

#include "xml/element.h"

struct xml_color
{
	struct xml_element element;
	struct optional_float r;
	struct optional_float g;
	struct optional_float b;
	struct optional_float a;
};

OPTIONAL_DEF(optional_xml_color, struct xml_color);

extern const struct xml_vtable xml_color_vtable;

#endif
