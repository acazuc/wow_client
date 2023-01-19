#ifndef XML_MINIMAP_H
#define XML_MINIMAP_H

#include "xml/frame.h"

#ifdef interface
# undef interface
#endif

struct xml_minimap
{
	struct xml_frame frame;
	char *arrow_model;
	char *player_model;
};

extern const struct xml_vtable xml_minimap_vtable;

#endif
