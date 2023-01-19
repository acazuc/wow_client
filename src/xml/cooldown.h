#ifndef XML_COOLDOWN_H
#define XML_COOLDOWN_H

#include "xml/frame.h"

#ifdef interface
# undef interface
#endif

struct xml_cooldown
{
	struct xml_frame frame;
	struct optional_bool reverse;
	struct optional_bool draw_edge;
};

extern const struct xml_vtable xml_cooldown_vtable;

#endif
