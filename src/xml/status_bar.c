#include "status_bar.h"
#include "internal.h"

#include "ui/status_bar.h"

#include "memory.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_frame_vtable.ctr(element);
	struct xml_status_bar *status_bar = (struct xml_status_bar*)element;
	((struct xml_layout_frame*)status_bar)->mask |= XML_LAYOUT_FRAME_status_bar;
	OPTIONAL_UNSET(status_bar->bar_texture);
	OPTIONAL_UNSET(status_bar->bar_color);
	status_bar->draw_layer = NULL;
	OPTIONAL_UNSET(status_bar->min_value);
	OPTIONAL_UNSET(status_bar->max_value);
	OPTIONAL_UNSET(status_bar->default_value);
}

static void dtr(struct xml_element *element)
{
	struct xml_status_bar *status_bar = (struct xml_status_bar*)element;
	XML_ELEMENT_DTR(status_bar->bar_texture);
	XML_ELEMENT_DTR(status_bar->bar_color);
	mem_free(MEM_XML, status_bar->draw_layer);
	xml_frame_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_status_bar *status_bar = (struct xml_status_bar*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "drawLayer", &status_bar->draw_layer);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "minValue", &status_bar->min_value);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "maxValue", &status_bar->max_value);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "defaultValue", &status_bar->default_value);
	return xml_frame_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_status_bar *status_bar = (struct xml_status_bar*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "BarTexture", status_bar->bar_texture, &xml_texture_vtable);
			XML_ELEMENT_CHILD_TEST(child, "BarColor", status_bar->bar_color, &xml_color_vtable);
			break;
		default:
			break;
	}
	return xml_frame_vtable.parse_child(element, child);
}

static struct ui_region *load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return (struct ui_region*)ui_status_bar_new(interface, layout_frame->name, parent);
}

const struct xml_vtable xml_status_bar_vtable =
{
	.name = "StatusBar",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
	.load_interface = load_interface,
};
