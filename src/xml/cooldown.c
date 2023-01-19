#include "cooldown.h"
#include "internal.h"

#include "ui/cooldown.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_frame_vtable.ctr(element);
	struct xml_cooldown *cooldown = (struct xml_cooldown*)element;
	((struct xml_layout_frame*)cooldown)->mask |= XML_LAYOUT_FRAME_cooldown;
	OPTIONAL_UNSET(cooldown->reverse);
	OPTIONAL_UNSET(cooldown->draw_edge);
}

static void dtr(struct xml_element *element)
{
	xml_frame_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_cooldown *cooldown = (struct xml_cooldown*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "reverse", &cooldown->reverse);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "drawEdge", &cooldown->draw_edge);
	return xml_frame_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	return xml_frame_vtable.parse_child(element, child);
}

static struct ui_region *load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return (struct ui_region*)ui_cooldown_new(interface, layout_frame->name, parent);
}

const struct xml_vtable xml_cooldown_vtable =
{
	.name = "Cooldown",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
	.load_interface = load_interface,
};
