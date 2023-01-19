#ifndef XML_COLOR_SELECT_H
#define XML_COLOR_SELECT_H

#include "xml/texture.h"
#include "xml/frame.h"

#ifdef interface
# undef interface
#endif

struct xml_color_select
{
	struct xml_frame frame;
	struct optional_xml_texture color_wheel_texture;
	struct optional_xml_texture color_wheel_thumb_texture;
	struct optional_xml_texture color_value_texture;
	struct optional_xml_texture color_value_thumb_texture;
};

extern const struct xml_vtable xml_color_select_vtable;

#endif
