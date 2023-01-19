#ifndef XML_SLIDER_H
#define XML_SLIDER_H

#include "xml/texture.h"
#include "xml/frame.h"

#ifdef interface
# undef interface
#endif

struct xml_slider
{
	struct xml_frame frame;
	struct optional_xml_texture thumb_texture;
	char *draw_layer;
	struct optional_float min_value;
	struct optional_float max_value;
	struct optional_float value_step;
	struct optional_float default_value;
	char *orientation;
};

extern const struct xml_vtable xml_slider_vtable;

#endif
