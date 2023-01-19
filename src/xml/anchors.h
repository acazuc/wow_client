#ifndef XML_ANCHORS_H
#define XML_ANCHORS_H

#include "xml/dimension.h"
#include "xml/element.h"

#include <jks/array.h>

#ifdef interface
# undef interface
#endif

struct xml_anchor
{
	struct xml_element element;
	struct optional_xml_dimension offset;
	char *point;
	char *relative_point;
	char *relative_to;
};

struct xml_anchors
{
	struct xml_element element;
	struct jks_array anchors; /* struct xml_anchor* */
};

OPTIONAL_DEF(optional_xml_anchors, struct xml_anchors);

extern const struct xml_vtable xml_anchor_vtable;
extern const struct xml_vtable xml_anchors_vtable;

#endif
