#include "slider.h"
#include "internal.h"

#include "ui/slider.h"

#include "memory.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_frame_vtable.ctr(element);
	struct xml_slider *slider = (struct xml_slider*)element;
	((struct xml_layout_frame*)slider)->mask |= XML_LAYOUT_FRAME_slider;
	OPTIONAL_UNSET(slider->thumb_texture);
	slider->draw_layer = NULL;
	slider->orientation = NULL;
	OPTIONAL_UNSET(slider->min_value);
	OPTIONAL_UNSET(slider->max_value);
	OPTIONAL_UNSET(slider->value_step);
	OPTIONAL_UNSET(slider->default_value);
}

static void dtr(struct xml_element *element)
{
	struct xml_slider *slider = (struct xml_slider*)element;
	XML_ELEMENT_DTR(slider->thumb_texture);
	mem_free(MEM_XML, slider->draw_layer);
	mem_free(MEM_XML, slider->orientation);
	xml_frame_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_slider *slider = (struct xml_slider*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "drawLayer", &slider->draw_layer);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "minValue", &slider->min_value);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "maxValue", &slider->max_value);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "valueStep", &slider->value_step);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "defaultValue", &slider->default_value);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "orientation", &slider->orientation);
	return xml_frame_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_slider *slider = (struct xml_slider*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "ThumbTexture", slider->thumb_texture, &xml_texture_vtable);
			break;
		default:
			break;
	}
	return xml_frame_vtable.parse_child(element, child);
}

static struct ui_region *load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return (struct ui_region*)ui_slider_new(interface, layout_frame->name, parent);
}

const struct xml_vtable xml_slider_vtable =
{
	.name = "Slider",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
	.load_interface = load_interface,
};
