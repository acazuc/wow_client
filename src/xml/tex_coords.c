#include "tex_coords.h"
#include "internal.h"

#include <string.h>

static void ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_tex_coords *tex_coords = (struct xml_tex_coords*)element;
	OPTIONAL_UNSET(tex_coords->left);
	OPTIONAL_UNSET(tex_coords->right);
	OPTIONAL_UNSET(tex_coords->top);
	OPTIONAL_UNSET(tex_coords->bottom);
}

static void dtr(struct xml_element *element)
{
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_tex_coords *tex_coords = (struct xml_tex_coords*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "left", &tex_coords->left);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "right", &tex_coords->right);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "top", &tex_coords->top);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "bottom", &tex_coords->bottom);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_tex_coords_vtable =
{
	.name = "TexCoords",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
