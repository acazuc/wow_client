#ifndef XML_GAME_TOOLTIP_H
#define XML_GAME_TOOLTIP_H

#include "xml/frame.h"

#ifdef interface
# undef interface
#endif

struct xml_game_tooltip
{
	struct xml_frame frame;
};

extern const struct xml_vtable xml_game_tooltip_vtable;

#endif
