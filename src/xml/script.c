#include "script.h"
#include "internal.h"

#include "memory.h"

#include <string.h>

static void ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_script *script = (struct xml_script*)element;
	script->file = NULL;
	script->script = NULL;
}

static void dtr(struct xml_element *element)
{
	struct xml_script *script = (struct xml_script*)element;
	mem_free(MEM_XML, script->file);
	mem_free(MEM_XML, script->script);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_script *script = (struct xml_script*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "file", &script->file);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_script *script = (struct xml_script*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			script->script = mem_strdup(MEM_XML, child->value);
			return XML_PARSE_CHILD_OK;
		default:
			break;
	}
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_script_vtable =
{
	.name = "Script",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
