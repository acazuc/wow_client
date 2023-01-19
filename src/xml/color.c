#include "color.h"
#include "internal.h"

#include <string.h>

static void ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_color *color = (struct xml_color*)element;
	OPTIONAL_UNSET(color->r);
	OPTIONAL_UNSET(color->g);
	OPTIONAL_UNSET(color->b);
	OPTIONAL_UNSET(color->a);
}

static void dtr(struct xml_element *element)
{
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_color *color = (struct xml_color*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "r", &color->r);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "g", &color->g);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "b", &color->b);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "a", &color->a);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_color_vtable =
{
	.name = "Color",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
