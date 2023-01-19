#ifndef XML_ATTRIBUTES_H
#define XML_ATTRIBUTES_H

#include "xml/element.h"

#include <jks/array.h>

struct xml_attribute
{
	struct xml_element element;
	char *name;
	char *type;
	char *value;
};

struct xml_attributes
{
	struct xml_element element;
	struct jks_array attributes; /* struct xml_attribute* */
};

OPTIONAL_DEF(optional_xml_attributes, struct xml_attributes);

extern const struct xml_vtable xml_attribute_vtable;
extern const struct xml_vtable xml_attributes_vtable;

#endif
