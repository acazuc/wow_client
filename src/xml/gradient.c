#include "xml/gradient.h"
#include "xml/internal.h"

#include "memory.h"

#include <string.h>

static void ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_gradient *gradient = (struct xml_gradient*)element;
	OPTIONAL_UNSET(gradient->min_color);
	OPTIONAL_UNSET(gradient->max_color);
	gradient->orientation = NULL;
}

static void dtr(struct xml_element *element)
{
	struct xml_gradient *gradient = (struct xml_gradient*)element;
	XML_ELEMENT_DTR(gradient->min_color);
	XML_ELEMENT_DTR(gradient->max_color);
	mem_free(MEM_XML, gradient->orientation);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_gradient *gradient = (struct xml_gradient*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "orientation", &gradient->orientation);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_gradient *gradient = (struct xml_gradient*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "MinColor", gradient->min_color, &xml_color_vtable);
			XML_ELEMENT_CHILD_TEST(child, "MaxColor", gradient->max_color, &xml_color_vtable);
			break;
		default:
			break;
	}
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_gradient_vtable =
{
	.name = "Gradient",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
