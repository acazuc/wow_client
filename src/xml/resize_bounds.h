#ifndef XML_RESIZE_BOUNDS_H
#define XML_RESIZE_BOUNDS_H

#include "xml/dimension.h"
#include "xml/element.h"

struct xml_resize_bounds
{
	struct xml_element element;
	struct optional_xml_dimension min_resize;
	struct optional_xml_dimension max_resize;
};

OPTIONAL_DEF(optional_xml_resize_bounds, struct xml_resize_bounds);

extern const struct xml_vtable xml_resize_bounds_vtable;

#endif
