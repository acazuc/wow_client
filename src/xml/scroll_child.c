#include "scroll_child.h"
#include "internal.h"

#include "xml/layout_frame.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_scroll_child *scroll_child = (struct xml_scroll_child*)element;
	scroll_child->frame = NULL;
}

static void dtr(struct xml_element *element)
{
	struct xml_scroll_child *scroll_child = (struct xml_scroll_child*)element;
	xml_element_delete((struct xml_element*)scroll_child->frame);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_scroll_child *scroll_child = (struct xml_scroll_child*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
		{
			if (scroll_child->frame)
				return XML_PARSE_CHILD_ALREADY;
			scroll_child->frame = xml_create_layout_frame((const char*)child->name);
			if (!scroll_child->frame)
				break;
			xml_element_parse((struct xml_element*)scroll_child->frame, child);
			return XML_PARSE_CHILD_OK;
		}
		default:
			break;
	}
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_scroll_child_vtable =
{
	.name = "ScrollChild",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
