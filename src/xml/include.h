#ifndef XML_INCLUDE_H
#define XML_INCLUDE_H

#include "xml/element.h"

struct xml_include
{
	struct xml_element element;
	char *file;
};

extern const struct xml_vtable xml_include_vtable;

#endif
