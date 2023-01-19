#ifndef XML_SCRIPT_H
#define XML_SCRIPT_H

#include "xml/element.h"

struct xml_script
{
	struct xml_element element;
	char *script;
	char *file;
};

extern const struct xml_vtable xml_script_vtable;

#endif
