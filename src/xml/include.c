#include "xml/internal.h"
#include "xml/include.h"

#include "memory.h"

#include <string.h>

static void ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_include *include = (struct xml_include*)element;
	include->file = NULL;
}

static void dtr(struct xml_element *element)
{
	struct xml_include *include = (struct xml_include*)element;
	mem_free(MEM_XML, include->file);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_include *include = (struct xml_include*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "file", &include->file);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_include_vtable =
{
	.name = "Include",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
