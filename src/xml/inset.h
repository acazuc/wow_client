#ifndef XML_INSET_H
#define XML_INSET_H

#include "xml/element.h"

struct xml_abs_inset
{
	struct xml_element element;
	struct optional_int32 left;
	struct optional_int32 right;
	struct optional_int32 top;
	struct optional_int32 bottom;
};

struct xml_rel_inset
{
	struct xml_element element;
	struct optional_float left;
	struct optional_float right;
	struct optional_float top;
	struct optional_float bottom;
};

struct xml_inset
{
	struct xml_element element;
	struct xml_element *inset;
	struct optional_int32 left;
	struct optional_int32 right;
	struct optional_int32 top;
	struct optional_int32 bottom;
};

OPTIONAL_DEF(optional_xml_inset, struct xml_inset);

extern const struct xml_vtable xml_abs_inset_vtable;
extern const struct xml_vtable xml_rel_inset_vtable;
extern const struct xml_vtable xml_inset_vtable;

#endif
