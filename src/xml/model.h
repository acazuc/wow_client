#ifndef XML_MODEL_H
#define XML_MODEL_H

#include "xml/frame.h"
#include "xml/color.h"

#ifdef interface
# undef interface
#endif

struct xml_model
{
	struct xml_frame frame;
	struct optional_xml_color fog_color;
	char *file;
	struct optional_float scale;
	struct optional_float fog_near;
	struct optional_float fog_far;
};

extern const struct xml_vtable xml_model_vtable;

#endif
