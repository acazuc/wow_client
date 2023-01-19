#include "scroll_frame.h"
#include "internal.h"

#include "ui/scroll_frame.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_frame_vtable.ctr(element);
	struct xml_scroll_frame *scroll_frame = (struct xml_scroll_frame*)element;
	((struct xml_layout_frame*)scroll_frame)->mask |= XML_LAYOUT_FRAME_scroll_frame;
	OPTIONAL_UNSET(scroll_frame->scroll_child);
}

static void dtr(struct xml_element *element)
{
	struct xml_scroll_frame *scroll_frame = (struct xml_scroll_frame*)element;
	XML_ELEMENT_DTR(scroll_frame->scroll_child);
	xml_frame_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	return xml_frame_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_scroll_frame *scroll_frame = (struct xml_scroll_frame*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "ScrollChild", scroll_frame->scroll_child, &xml_scroll_child_vtable);
			break;
		default:
			break;
	}
	return xml_frame_vtable.parse_child(element, child);
}

static struct ui_region *load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return (struct ui_region*)ui_scroll_frame_new(interface, layout_frame->name, parent);
}

const struct xml_vtable xml_scroll_frame_vtable =
{
	.name = "ScrollFrame",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
	.load_interface = load_interface,
};
