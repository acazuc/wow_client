#ifndef XML_WORLD_FRAME_H
#define XML_WORLD_FRAME_H

#include "xml/frame.h"

#ifdef interface
# undef interface
#endif

struct xml_world_frame
{
	struct xml_frame frame;
};

extern const struct xml_vtable xml_world_frame_vtable;

#endif
