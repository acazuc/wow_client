#ifndef XML_DIMENSION_H
#define XML_DIMENSION_H

#include "xml/element.h"

struct xml_abs_dimension
{
	struct xml_element element;
	struct optional_int32 x;
	struct optional_int32 y;
};

struct xml_rel_dimension
{
	struct xml_element element;
	struct optional_float x;
	struct optional_float y;
};

struct xml_dimension
{
	struct xml_element element;
	struct xml_element *dimension;
	struct optional_int32 x;
	struct optional_int32 y;
};

OPTIONAL_DEF(optional_xml_dimension, struct xml_dimension);

extern const struct xml_vtable xml_abs_dimension_vtable;
extern const struct xml_vtable xml_rel_dimension_vtable;
extern const struct xml_vtable xml_dimension_vtable;

#endif
