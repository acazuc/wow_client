#ifndef XML_SCROLL_FRAME_H
#define XML_SCROLL_FRAME_H

#include "xml/scroll_child.h"
#include "xml/frame.h"

#ifdef interface
# undef interface
#endif

struct xml_scroll_frame
{
	struct xml_frame frame;
	struct optional_xml_scroll_child scroll_child;
};

extern const struct xml_vtable xml_scroll_frame_vtable;

#endif
