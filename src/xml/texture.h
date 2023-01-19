#ifndef XML_TEXTURE_H
#define XML_TEXTURE_H

#include "xml/layout_frame.h"
#include "xml/tex_coords.h"
#include "xml/gradient.h"
#include "xml/color.h"

#ifdef interface
# undef interface
#endif

struct xml_texture
{
	struct xml_layout_frame layout_frame;
	struct optional_xml_tex_coords tex_coords;
	struct optional_xml_gradient gradient;
	struct optional_xml_color color;
	char *file;
	char *alpha_mode;
};

OPTIONAL_DEF(optional_xml_texture, struct xml_texture);

extern const struct xml_vtable xml_texture_vtable;

#endif
