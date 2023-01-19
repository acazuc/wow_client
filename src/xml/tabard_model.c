#include "tabard_model.h"
#include "internal.h"

#include "ui/tabard_model.h"

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_player_model_vtable.ctr(element);
	struct xml_tabard_model *tabard_model = (struct xml_tabard_model*)element;
	((struct xml_layout_frame*)tabard_model)->mask |= XML_LAYOUT_FRAME_tabard_model;
}

static void dtr(struct xml_element *element)
{
	xml_player_model_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	return xml_player_model_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	return xml_player_model_vtable.parse_child(element, child);
}

static struct ui_region *load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return (struct ui_region*)ui_tabard_model_new(interface, layout_frame->name, parent);
}

const struct xml_vtable xml_tabard_model_vtable =
{
	.name = "TabardModel",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
	.load_interface = load_interface,
};
