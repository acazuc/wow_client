#include "attributes.h"
#include "internal.h"

#include "memory.h"
#include "log.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

MEMORY_DECL(XML);

static void attribute_ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_attribute *xml_attribute = (struct xml_attribute*)element;
	xml_attribute->name = NULL;
	xml_attribute->type = NULL;
	xml_attribute->value = NULL;
}

static void attribute_dtr(struct xml_element *element)
{
	struct xml_attribute *xml_attribute = (struct xml_attribute*)element;
	mem_free(MEM_XML, xml_attribute->name);
	mem_free(MEM_XML, xml_attribute->type);
	mem_free(MEM_XML, xml_attribute->value);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status attribute_parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_attribute *xml_attribute = (struct xml_attribute*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "name", &xml_attribute->name);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "type", &xml_attribute->type);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "value", &xml_attribute->value);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status attribute_parse_child(struct xml_element *element, const struct xml_node *child)
{
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_attribute_vtable =
{
	.name = "Attribute",
	.ctr = attribute_ctr,
	.dtr = attribute_dtr,
	.parse_attribute = attribute_parse_attribute,
	.parse_child = attribute_parse_child,
};

static void delete_attribute(void *attribute)
{
	xml_element_delete(*(struct xml_element**)attribute);
}

static void ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_attributes *attributes = (struct xml_attributes*)element;
	jks_array_init(&attributes->attributes, sizeof(struct xml_attribute*), delete_attribute, &jks_array_memory_fn_XML);
}

static void dtr(struct xml_element *element)
{
	struct xml_attributes *attributes = (struct xml_attributes*)element;
	jks_array_destroy(&attributes->attributes);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_attributes *attributes = (struct xml_attributes*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			if (!strcmp((const char*)child->name, "Attribute"))
			{
				struct xml_attribute *attribute = mem_malloc(MEM_XML, sizeof(*attribute));
				attribute->element.vtable = &xml_attribute_vtable;
				attribute->element.vtable->ctr(&attribute->element);
				if (!jks_array_push_back(&attributes->attributes, &attribute))
				{
					LOG_ERROR("failed to push attribute");
					xml_element_delete((struct xml_element*)attribute);
					return XML_PARSE_CHILD_INTERNAL;
				}
				xml_element_parse(&attribute->element, child);
				return XML_PARSE_CHILD_OK;
			}
			break;
		default:
			break;
	}
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_attributes_vtable =
{
	.name = "Attributes",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
