#include "anchors.h"
#include "internal.h"

#include "memory.h"
#include "log.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

MEMORY_DECL(XML);

static void anchor_ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_anchor *anchor = (struct xml_anchor*)element;
	OPTIONAL_UNSET(anchor->offset);
	anchor->point = NULL;
	anchor->relative_point = NULL;
	anchor->relative_to = NULL;
}

static void anchor_dtr(struct xml_element *element)
{
	struct xml_anchor *anchor = (struct xml_anchor*)element;
	XML_ELEMENT_DTR(anchor->offset);
	mem_free(MEM_XML, anchor->point);
	mem_free(MEM_XML, anchor->relative_point);
	mem_free(MEM_XML, anchor->relative_to);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status anchor_parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_anchor *anchor = (struct xml_anchor*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "point", &anchor->point);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "relativePoint", &anchor->relative_point);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "relativeTo", &anchor->relative_to);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status anchor_parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_anchor *anchor = (struct xml_anchor*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "Offset", anchor->offset, &xml_dimension_vtable);
			break;
		default:
			break;
	}
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_anchor_vtable =
{
	.name = "Anchor",
	.ctr = anchor_ctr,
	.dtr = anchor_dtr,
	.parse_attribute = anchor_parse_attribute,
	.parse_child = anchor_parse_child,
};

static void anchor_delete(void *anchor)
{
	xml_element_delete(*(struct xml_element**)anchor);
}

static void ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_anchors *anchors = (struct xml_anchors*)element;
	jks_array_init(&anchors->anchors, sizeof(struct xml_anchor*), anchor_delete, &jks_array_memory_fn_XML);
}

static void dtr(struct xml_element *element)
{
	struct xml_anchors *anchors = (struct xml_anchors*)element;
	jks_array_destroy(&anchors->anchors);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_anchors *anchors = (struct xml_anchors*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			if (!strcmp((const char*)child->name, "Anchor"))
			{
				struct xml_element *anchor = mem_malloc(MEM_XML, sizeof(struct xml_anchor));
				if (!anchor)
				{
					LOG_ERROR("anchor allocation failed");
					return XML_PARSE_CHILD_INTERNAL;
				}
				anchor->vtable = &xml_anchor_vtable;
				anchor->vtable->ctr(anchor);
				if (!jks_array_push_back(&anchors->anchors, &anchor))
				{
					LOG_ERROR("failed to add anchor to anchors list");
					xml_element_delete(anchor);
					return XML_PARSE_CHILD_INTERNAL;
				}
				xml_element_parse(anchor, child);
				return XML_PARSE_CHILD_OK;
			}
			break;
		default:
			break;
	}
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_anchors_vtable =
{
	.name = "Anchors",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
