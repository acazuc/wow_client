#ifndef XML_VALUE_H
#define XML_VALUE_H

#include "xml/element.h"

#ifdef interface
# undef interface
#endif

struct xml_abs_value
{
	struct xml_element element;
	struct optional_int32 val;
};

struct xml_rel_value
{
	struct xml_element element;
	struct optional_float val;
};

struct xml_value
{
	struct xml_element element;
	struct xml_element *value;
	struct optional_int32 val;
};

OPTIONAL_DEF(optional_xml_value, struct xml_value);

extern const struct xml_vtable xml_abs_value_vtable;
extern const struct xml_vtable xml_rel_value_vtable;
extern const struct xml_vtable xml_value_vtable;

#endif
