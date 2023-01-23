#include "xml/color_select.h"
#include "xml/internal.h"

#include "ui/color_select.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_frame_vtable.ctr(element);
	struct xml_color_select *color_select = (struct xml_color_select*)element;
	((struct xml_layout_frame*)color_select)->mask |= XML_LAYOUT_FRAME_color_select;
	OPTIONAL_UNSET(color_select->color_wheel_texture);
	OPTIONAL_UNSET(color_select->color_wheel_thumb_texture);
	OPTIONAL_UNSET(color_select->color_value_texture);
	OPTIONAL_UNSET(color_select->color_value_thumb_texture);
}

static void dtr(struct xml_element *element)
{
	struct xml_color_select *color_select = (struct xml_color_select*)element;
	XML_ELEMENT_DTR(color_select->color_wheel_texture);
	XML_ELEMENT_DTR(color_select->color_wheel_thumb_texture);
	XML_ELEMENT_DTR(color_select->color_value_texture);
	XML_ELEMENT_DTR(color_select->color_value_thumb_texture);
	xml_frame_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	return xml_frame_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_color_select *color_select = (struct xml_color_select*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "ColorWheelTexture", color_select->color_wheel_texture, &xml_texture_vtable);
			XML_ELEMENT_CHILD_TEST(child, "ColorWheelThumbTexture", color_select->color_wheel_thumb_texture, &xml_texture_vtable);
			XML_ELEMENT_CHILD_TEST(child, "ColorValueTexture", color_select->color_value_texture, &xml_texture_vtable);
			XML_ELEMENT_CHILD_TEST(child, "ColorValueThumbTexture", color_select->color_value_thumb_texture, &xml_texture_vtable);
			break;
		default:
			break;
	}
	return xml_frame_vtable.parse_child(element, child);
}

static struct ui_region *load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return (struct ui_region*)ui_color_select_new(interface, layout_frame->name, parent);
}

const struct xml_vtable xml_color_select_vtable =
{
	.name = "ColorSelect",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
	.load_interface = load_interface,
};
