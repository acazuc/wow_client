#include "xml/internal.h"
#include "xml/shadow.h"

#include <string.h>

static void ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_shadow *shadow = (struct xml_shadow*)element;
	OPTIONAL_UNSET(shadow->offset);
	OPTIONAL_UNSET(shadow->color);
}

static void dtr(struct xml_element *element)
{
	struct xml_shadow *shadow = (struct xml_shadow*)element;
	XML_ELEMENT_DTR(shadow->offset);
	XML_ELEMENT_DTR(shadow->color);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_shadow *shadow = (struct xml_shadow*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "Offset", shadow->offset, &xml_dimension_vtable);
			XML_ELEMENT_CHILD_TEST(child, "Color", shadow->color, &xml_color_vtable);
			break;
		default:
			break;
	}
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_shadow_vtable =
{
	.name = "Shadow",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
