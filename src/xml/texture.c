#include "xml/internal.h"
#include "xml/texture.h"

#include "ui/texture.h"

#include "memory.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_layout_frame_vtable.ctr(element);
	struct xml_texture *texture = (struct xml_texture*)element;
	((struct xml_layout_frame*)texture)->mask |= XML_LAYOUT_FRAME_texture;
	OPTIONAL_UNSET(texture->tex_coords);
	OPTIONAL_UNSET(texture->gradient);
	OPTIONAL_UNSET(texture->color);
	texture->file = NULL;
	texture->alpha_mode = NULL;
}

static void dtr(struct xml_element *element)
{
	struct xml_texture *texture = (struct xml_texture*)element;
	XML_ELEMENT_DTR(texture->tex_coords);
	XML_ELEMENT_DTR(texture->gradient);
	XML_ELEMENT_DTR(texture->color);
	mem_free(MEM_XML, texture->file);
	mem_free(MEM_XML, texture->alpha_mode);
	xml_layout_frame_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_texture *texture = (struct xml_texture*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "file", &texture->file);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "alphaMode", &texture->alpha_mode);
	return xml_layout_frame_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_texture *texture = (struct xml_texture*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "Color", texture->color, &xml_color_vtable);
			XML_ELEMENT_CHILD_TEST(child, "TexCoords", texture->tex_coords, &xml_tex_coords_vtable);
			XML_ELEMENT_CHILD_TEST(child, "Gradient", texture->gradient, &xml_gradient_vtable);
			break;
		default:
			break;
	}
	return xml_layout_frame_vtable.parse_child(element, child);
}

static struct ui_region *load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return (struct ui_region*)ui_texture_new(interface, layout_frame->name, parent);
}

const struct xml_vtable xml_texture_vtable =
{
	.name = "Texture",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
	.load_interface = load_interface,
};
