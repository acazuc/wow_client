#include "value.h"
#include "internal.h"

#include "memory.h"
#include "log.h"

#include <string.h>

static void abs_ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_abs_value *value = (struct xml_abs_value*)element;
	OPTIONAL_UNSET(value->val);
}

static void abs_dtr(struct xml_element *element)
{
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status abs_parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_abs_value *value = (struct xml_abs_value*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "val", &value->val);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status abs_parse_child(struct xml_element *element, const struct xml_node *child)
{
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_abs_value_vtable =
{
	.name = "AbsValue",
	.ctr = abs_ctr,
	.dtr = abs_dtr,
	.parse_attribute = abs_parse_attribute,
	.parse_child = abs_parse_child,
};

static void rel_ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_rel_value *value = (struct xml_rel_value*)element;
	OPTIONAL_UNSET(value->val);
}

static void rel_dtr(struct xml_element *element)
{
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status rel_parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_rel_value *value = (struct xml_rel_value*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "val", &value->val);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status rel_parse_child(struct xml_element *element, const struct xml_node *child)
{
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_rel_value_vtable =
{
	.name = "RelValue",
	.ctr = rel_ctr,
	.dtr = rel_dtr,
	.parse_attribute = rel_parse_attribute,
	.parse_child = rel_parse_child,
};

static void ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_value *value = (struct xml_value*)element;
	value->value = NULL;
	OPTIONAL_UNSET(value->val);
}

static void dtr(struct xml_element *element)
{
	struct xml_value *value = (struct xml_value*)element;
	xml_element_delete(value->value);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_value *value = (struct xml_value*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "val", &value->val);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_value *value = (struct xml_value*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			if (!strcmp((const char*)child->name, "AbsValue"))
			{
				if (value->value)
					return XML_PARSE_CHILD_ALREADY;
				value->value = mem_malloc(MEM_XML, sizeof(struct xml_abs_value));
				if (!value->value)
				{
					LOG_ERROR("xml abs value allocation failed");
					return XML_PARSE_CHILD_INTERNAL;
				}
				value->value->vtable = &xml_abs_value_vtable;
				value->value->vtable->ctr(value->value);
				xml_element_parse(value->value, child);
				return XML_PARSE_CHILD_OK;
			}
			if (!strcmp((const char*)child->name, "RelValue"))
			{
				if (value->value)
					return XML_PARSE_CHILD_ALREADY;
				value->value = mem_malloc(MEM_XML, sizeof(struct xml_rel_value));
				if (!value->value)
				{
					LOG_ERROR("xml rel value allocation failed");
					return XML_PARSE_CHILD_INTERNAL;
				}
				value->value->vtable = &xml_rel_value_vtable;
				value->value->vtable->ctr(value->value);
				xml_element_parse(value->value, child);
				return XML_PARSE_CHILD_OK;
			}
			break;
		default:
			break;
	}
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_value_vtable =
{
	.name = "Value",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
