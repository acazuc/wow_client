#ifndef XML_SHADOW_H
#define XML_SHADOW_H

#include "xml/dimension.h"
#include "xml/element.h"
#include "xml/color.h"

#ifdef interface
# undef interface
#endif

struct xml_shadow
{
	struct xml_element element;
	struct optional_xml_dimension offset;
	struct optional_xml_color color;
};

OPTIONAL_DEF(optional_xml_shadow, struct xml_shadow);

extern const struct xml_vtable xml_shadow_vtable;

#endif
