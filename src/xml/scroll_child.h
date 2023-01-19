#ifndef XML_SCROLL_CHILD_H
#define XML_SCROLL_CHILD_H

#include "xml/element.h"
#include "xml/frame.h"

struct xml_scroll_child
{
	struct xml_element element;
	struct xml_layout_frame *frame;
};

OPTIONAL_DEF(optional_xml_scroll_child, struct xml_scroll_child);

extern const struct xml_vtable xml_scroll_child_vtable;

#endif
