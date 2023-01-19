#include "dimension.h"
#include "internal.h"

#include "memory.h"

#include "log.h"

#include <string.h>

static void abs_ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_abs_dimension *abs_dimension = (struct xml_abs_dimension*)element;
	OPTIONAL_UNSET(abs_dimension->x);
	OPTIONAL_UNSET(abs_dimension->y);
}

static void abs_dtr(struct xml_element *element)
{
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status abs_parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_abs_dimension *abs_dimension = (struct xml_abs_dimension*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "x", &abs_dimension->x);
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "y", &abs_dimension->y);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status abs_parse_child(struct xml_element *element, const struct xml_node *child)
{
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_abs_dimension_vtable =
{
	.name = "AbsDimension",
	.ctr = abs_ctr,
	.dtr = abs_dtr,
	.parse_attribute = abs_parse_attribute,
	.parse_child = abs_parse_child,
};

static void rel_ctr(struct xml_element *element)
{
	struct xml_rel_dimension *rel_dimension = (struct xml_rel_dimension*)element;
	xml_element_vtable.ctr(element);
	OPTIONAL_UNSET(rel_dimension->x);
	OPTIONAL_UNSET(rel_dimension->y);
}

static void rel_dtr(struct xml_element *element)
{
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status rel_parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_rel_dimension *rel_dimension = (struct xml_rel_dimension_*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "x", &rel_dimension->x);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "y", &rel_dimension->y);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status rel_parse_child(struct xml_element *element, const struct xml_node *child)
{
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_rel_dimension_vtable =
{
	.name = "RelDimension",
	.ctr = rel_ctr,
	.dtr = rel_dtr,
	.parse_attribute = rel_parse_attribute,
	.parse_child = rel_parse_child,
};

static void ctr(struct xml_element *element)
{
	struct xml_dimension *dimension = (struct xml_dimension*)element;
	xml_element_vtable.ctr(element);
	dimension->dimension = NULL;
	OPTIONAL_UNSET(dimension->x);
	OPTIONAL_UNSET(dimension->y);
}

static void dtr(struct xml_element *element)
{
	struct xml_dimension *dimension = (struct xml_dimension*)element;
	xml_element_delete(dimension->dimension);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_dimension *dimension = (struct xml_dimension*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "x", &dimension->x);
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "y", &dimension->y);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_dimension *dimension = (struct xml_dimension*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			if (!strcmp((const char*)child->name, "AbsDimension"))
			{
				if (dimension->dimension)
					return XML_PARSE_CHILD_ALREADY;
				dimension->dimension = mem_malloc(MEM_XML, sizeof(struct xml_abs_dimension));
				if (!dimension->dimension)
				{
					LOG_ERROR("xml abs dimension allocation failed");
					return XML_PARSE_CHILD_INTERNAL;
				}
				dimension->dimension->vtable = &xml_abs_dimension_vtable;
				dimension->dimension->vtable->ctr(dimension->dimension);
				xml_element_parse(dimension->dimension, child);
				return XML_PARSE_CHILD_OK;
			}
			if (!strcmp((const char*)child->name, "RelDimension"))
			{
				if (dimension->dimension)
					return XML_PARSE_CHILD_ALREADY;
				dimension->dimension = mem_malloc(MEM_XML, sizeof(struct xml_rel_dimension));
				if (!dimension->dimension)
				{
					LOG_ERROR("xml rel dimension allocation failed");
					return XML_PARSE_CHILD_INTERNAL;
				}
				dimension->dimension->vtable = &xml_rel_dimension_vtable;
				dimension->dimension->vtable->ctr(dimension->dimension);
				xml_element_parse(dimension->dimension, child);
				return XML_PARSE_CHILD_OK;
			}
			break;
		default:
			break;
	}
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_dimension_vtable =
{
	.name = "Dimension",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
