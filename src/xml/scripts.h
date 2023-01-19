#ifndef XML_SCRIPTS_H
#define XML_SCRIPTS_H

#include "xml/element.h"

#include <jks/hmap.h>

struct xml_scripts
{
	struct xml_element element;
	struct jks_hmap scripts; /* char*, char* */
};

OPTIONAL_DEF(optional_xml_scripts, struct xml_scripts);

extern const struct xml_vtable xml_scripts_vtable;

#endif
