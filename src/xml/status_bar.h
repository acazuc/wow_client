#ifndef XML_STATUS_BAR_H
#define XML_STATUS_BAR_H

#include "xml/texture.h"
#include "xml/color.h"
#include "xml/frame.h"

#ifdef interface
# undef interface
#endif

struct xml_status_bar
{
	struct xml_frame frame;
	struct optional_xml_texture bar_texture;
	struct optional_xml_color bar_color;
	char *draw_layer;
	struct optional_float min_value;
	struct optional_float max_value;
	struct optional_float default_value;
};

extern const struct xml_vtable xml_status_bar_vtable;

#endif
