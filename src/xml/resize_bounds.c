#include "resize_bounds.h"
#include "internal.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_resize_bounds *resize_bounds = (struct xml_resize_bounds*)element;
	OPTIONAL_UNSET(resize_bounds->min_resize);
	OPTIONAL_UNSET(resize_bounds->max_resize);
}

static void dtr(struct xml_element *element)
{
	struct xml_resize_bounds *resize_bounds = (struct xml_resize_bounds*)element;
	XML_ELEMENT_DTR(resize_bounds->min_resize);
	XML_ELEMENT_DTR(resize_bounds->max_resize);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_resize_bounds *resize_bounds = (struct xml_resize_bounds*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "minResize", resize_bounds->min_resize, &xml_dimension_vtable);
			XML_ELEMENT_CHILD_TEST(child, "maxResize", resize_bounds->max_resize, &xml_dimension_vtable);
			break;
		default:
			break;
	}
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_resize_bounds_vtable =
{
	.name = "ResizeBounds",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
