#include "game_tooltip.h"
#include "internal.h"

#include "ui/game_tooltip.h"

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_frame_vtable.ctr(element);
	struct xml_game_tooltip *game_tooltip = (struct xml_game_tooltip*)element;
	((struct xml_layout_frame*)game_tooltip)->mask |= XML_LAYOUT_FRAME_game_tooltip;
}

static void dtr(struct xml_element *element)
{
	xml_frame_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	return xml_frame_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	return xml_frame_vtable.parse_child(element, child);
}

static struct ui_region *load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return (struct ui_region*)ui_game_tooltip_new(interface, layout_frame->name, parent);
}

const struct xml_vtable xml_game_tooltip_vtable =
{
	.name = "GameTooltip",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
	.load_interface = load_interface,
};
