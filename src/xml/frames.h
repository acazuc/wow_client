#ifndef XML_FRAMES_H
#define XML_FRAMES_H

#include "xml/element.h"

#include <jks/array.h>

struct xml_frames
{
	struct xml_element element;
	struct jks_array frames; /* struct xml_layout_frame* */
};

OPTIONAL_DEF(optional_xml_frames, struct xml_frames);

extern const struct xml_vtable xml_frames_vtable;

#endif
