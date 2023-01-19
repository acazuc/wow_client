#include "backdrop.h"
#include "internal.h"

#include "memory.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_backdrop *backdrop = (struct xml_backdrop*)element;
	OPTIONAL_UNSET(backdrop->background_insets);
	OPTIONAL_UNSET(backdrop->edge_size);
	OPTIONAL_UNSET(backdrop->tile_size);
	backdrop->bg_file = NULL;
	backdrop->edge_file = NULL;
	OPTIONAL_UNSET(backdrop->color);
	OPTIONAL_UNSET(backdrop->border_color);
	OPTIONAL_UNSET(backdrop->tile);
}

static void dtr(struct xml_element *element)
{
	struct xml_backdrop *backdrop = (struct xml_backdrop*)element;
	XML_ELEMENT_DTR(backdrop->background_insets);
	XML_ELEMENT_DTR(backdrop->edge_size);
	XML_ELEMENT_DTR(backdrop->tile_size);
	XML_ELEMENT_DTR(backdrop->color);
	XML_ELEMENT_DTR(backdrop->border_color);
	mem_free(MEM_XML, backdrop->bg_file);
	mem_free(MEM_XML, backdrop->edge_file);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_backdrop *backdrop = (struct xml_backdrop*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "bgFile", &backdrop->bg_file);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "edgeFile", &backdrop->edge_file);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "tile", &backdrop->tile);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_backdrop *backdrop = (struct xml_backdrop*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "EdgeSize", backdrop->edge_size, &xml_value_vtable);
			XML_ELEMENT_CHILD_TEST(child, "TileSize", backdrop->tile_size, &xml_value_vtable);
			XML_ELEMENT_CHILD_TEST(child, "BackgroundInsets", backdrop->background_insets, &xml_inset_vtable);
			XML_ELEMENT_CHILD_TEST(child, "Color", backdrop->color, &xml_color_vtable);
			XML_ELEMENT_CHILD_TEST(child, "BorderColor", backdrop->border_color, &xml_color_vtable);
			break;
		default:
			break;
	}
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_backdrop_vtable =
{
	.name = "Backdrop",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
