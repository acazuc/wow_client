#ifndef XML_TEX_COORDS_H
#define XML_TEX_COORDS_H

#include "xml/element.h"

#ifdef interface
# undef interface
#endif

struct xml_tex_coords
{
	struct xml_element element;
	struct optional_float left;
	struct optional_float right;
	struct optional_float top;
	struct optional_float bottom;
};

OPTIONAL_DEF(optional_xml_tex_coords, struct xml_tex_coords);

extern const struct xml_vtable xml_tex_coords_vtable;

#endif
