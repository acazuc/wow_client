#include "minimap.h"
#include "internal.h"

#include "ui/minimap.h"

#include "memory.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_frame_vtable.ctr(element);
	struct xml_minimap *minimap = (struct xml_minimap*)element;
	((struct xml_layout_frame*)minimap)->mask |= XML_LAYOUT_FRAME_minimap;
	minimap->arrow_model = NULL;
	minimap->player_model = NULL;
}

static void dtr(struct xml_element *element)
{
	struct xml_minimap *minimap = (struct xml_minimap*)element;
	mem_free(MEM_XML, minimap->arrow_model);
	mem_free(MEM_XML, minimap->player_model);
	xml_frame_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_minimap *minimap = (struct xml_minimap*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "minimapArrowModel", &minimap->arrow_model);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "minimapPlayerModel", &minimap->player_model);
	return xml_frame_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	return xml_frame_vtable.parse_child(element, child);
}

static struct ui_region *load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return (struct ui_region*)ui_minimap_new(interface, layout_frame->name, parent);
}

const struct xml_vtable xml_minimap_vtable =
{
	.name = "Minimap",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
	.load_interface = load_interface,
};
