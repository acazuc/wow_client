#include "xml/internal.h"
#include "xml/font.h"

#include "ui/font.h"

#include "memory.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

struct ui_object *xml_font_load_into_interface(struct xml_font *font, struct interface *interface)
{
	struct ui_font *ui_font = ui_font_new(interface, font->name, NULL);
	ui_object_load_xml((struct ui_object*)ui_font, (const struct xml_layout_frame*)font);
	return (struct ui_object*)ui_font;
}

static void ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_font *font = (struct xml_font*)element;
	OPTIONAL_UNSET(font->font_height);
	OPTIONAL_UNSET(font->shadow);
	OPTIONAL_UNSET(font->color);
	font->name = NULL;
	font->inherits = NULL;
	OPTIONAL_UNSET(font->is_virtual);
	font->font = NULL;
	OPTIONAL_UNSET(font->spacing);
	font->outline = NULL;
	OPTIONAL_UNSET(font->monochrome);
	font->justify_v = NULL;
	font->justify_h = NULL;
}

static void dtr(struct xml_element *element)
{
	struct xml_font *font = (struct xml_font*)element;
	XML_ELEMENT_DTR(font->font_height);
	XML_ELEMENT_DTR(font->shadow);
	XML_ELEMENT_DTR(font->color);
	mem_free(MEM_XML, font->name);
	mem_free(MEM_XML, font->inherits);
	mem_free(MEM_XML, font->font);
	mem_free(MEM_XML, font->outline);
	mem_free(MEM_XML, font->justify_v);
	mem_free(MEM_XML, font->justify_h);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_font *font = (struct xml_font*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "name", &font->name);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "inherits", &font->inherits);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "virtual", &font->is_virtual);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "font", &font->font);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "spacing", &font->spacing);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "outline", &font->outline);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "monochrome", &font->monochrome);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "justifyH", &font->justify_h);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "justifyV", &font->justify_v);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_font *font = (struct xml_font*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "Color", font->color, &xml_color_vtable);
			XML_ELEMENT_CHILD_TEST(child, "Shadow", font->shadow, &xml_shadow_vtable);
			XML_ELEMENT_CHILD_TEST(child, "FontHeight", font->font_height, &xml_value_vtable);
			break;
		default:
			break;
	}
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_font_vtable =
{
	.name = "Font",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
