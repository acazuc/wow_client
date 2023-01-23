#include "xml/internal.h"
#include "xml/inset.h"

#include "memory.h"
#include "log.h"

#include <string.h>

static void abs_ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_abs_inset *inset = (struct xml_abs_inset*)element;
	OPTIONAL_UNSET(inset->left);
	OPTIONAL_UNSET(inset->right);
	OPTIONAL_UNSET(inset->top);
	OPTIONAL_UNSET(inset->bottom);
}

static void abs_dtr(struct xml_element *element)
{
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status abs_parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_abs_inset *inset = (struct xml_abs_inset*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "left", &inset->left);
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "right", &inset->right);
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "top", &inset->top);
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "bottom", &inset->bottom);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status abs_parse_child(struct xml_element *element, const struct xml_node *child)
{
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_abs_inset_vtable =
{
	.name = "AbsInset",
	.ctr = abs_ctr,
	.dtr = abs_dtr,
	.parse_attribute = abs_parse_attribute,
	.parse_child = abs_parse_child,
};

static void rel_ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_abs_inset *inset = (struct xml_abs_inset*)element;
	OPTIONAL_UNSET(inset->left);
	OPTIONAL_UNSET(inset->right);
	OPTIONAL_UNSET(inset->top);
	OPTIONAL_UNSET(inset->bottom);
}

static void rel_dtr(struct xml_element *element)
{
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status rel_parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_rel_inset *inset = (struct xml_rel_inset*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "left", &inset->left);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "right", &inset->right);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "top", &inset->top);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "bottom", &inset->bottom);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status rel_parse_child(struct xml_element *element, const struct xml_node *child)
{
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_rel_inset_vtable =
{
	.name = "RelInset",
	.ctr = rel_ctr,
	.dtr = rel_dtr,
	.parse_attribute = rel_parse_attribute,
	.parse_child = rel_parse_child,
};

static void ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_inset *inset = (struct xml_inset*)element;
	inset->inset = NULL;
	OPTIONAL_UNSET(inset->left);
	OPTIONAL_UNSET(inset->right);
	OPTIONAL_UNSET(inset->top);
	OPTIONAL_UNSET(inset->bottom);
}

static void dtr(struct xml_element *element)
{
	struct xml_inset *inset = (struct xml_inset*)element;
	xml_element_delete(inset->inset);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_inset *inset = (struct xml_inset*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "left", &inset->left);
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "right", &inset->right);
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "top", &inset->top);
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "bottom", &inset->bottom);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_inset *inset = (struct xml_inset*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			if (!strcmp((const char*)child->name, "AbsInset"))
			{
				if (inset->inset)
					return XML_PARSE_CHILD_ALREADY;
				inset->inset = mem_malloc(MEM_XML, sizeof(struct xml_abs_inset));
				if (!inset->inset)
				{
					LOG_ERROR("xml abs inset allocation failed");
					return XML_PARSE_CHILD_INTERNAL;
				}
				inset->inset->vtable = &xml_abs_inset_vtable;
				inset->inset->vtable->ctr(inset->inset);
				xml_element_parse(inset->inset, child);
				return XML_PARSE_CHILD_OK;
			}
			if (!strcmp((const char*)child->name, "RelInset"))
			{
				if (inset->inset)
					return XML_PARSE_CHILD_ALREADY;
				inset->inset = mem_malloc(MEM_XML, sizeof(struct xml_rel_inset));
				if (!inset->inset)
				{
					LOG_ERROR("xml rel inset allocation failed");
					return XML_PARSE_CHILD_INTERNAL;
				}
				inset->inset->vtable = &xml_rel_inset_vtable;
				inset->inset->vtable->ctr(inset->inset);
				xml_element_parse(inset->inset, child);
				return XML_PARSE_CHILD_OK;
			}
			break;
		default:
			break;
	}
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_inset_vtable =
{
	.name = "Inset",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
