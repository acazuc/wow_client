#include "xml/font_string.h"
#include "xml/internal.h"

#include "ui/font_string.h"

#include "memory.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_layout_frame_vtable.ctr(element);
	struct xml_font_string *font_string = (struct xml_font_string*)element;
	((struct xml_layout_frame*)font_string)->mask |= XML_LAYOUT_FRAME_font_string;
	OPTIONAL_UNSET(font_string->font_height);
	OPTIONAL_UNSET(font_string->color);
	OPTIONAL_UNSET(font_string->shadow);
	font_string->font = NULL;
	OPTIONAL_UNSET(font_string->bytes);
	font_string->text = NULL;
	OPTIONAL_UNSET(font_string->spacing);
	font_string->outline = NULL;
	OPTIONAL_UNSET(font_string->monochrome);
	OPTIONAL_UNSET(font_string->nonspacewrap);
	font_string->justify_h = NULL;
	font_string->justify_v = NULL;
	OPTIONAL_UNSET(font_string->max_lines);
	OPTIONAL_UNSET(font_string->indented);
}

static void dtr(struct xml_element *element)
{
	struct xml_font_string *font_string = (struct xml_font_string*)element;
	XML_ELEMENT_DTR(font_string->font_height);
	XML_ELEMENT_DTR(font_string->color);
	XML_ELEMENT_DTR(font_string->shadow);
	mem_free(MEM_XML, font_string->font);
	mem_free(MEM_XML, font_string->text);
	mem_free(MEM_XML, font_string->outline);
	mem_free(MEM_XML, font_string->justify_h);
	mem_free(MEM_XML, font_string->justify_v);
	xml_layout_frame_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_font_string *font_string = (struct xml_font_string*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "font", &font_string->font);
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "bytes", &font_string->bytes);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "text", &font_string->text);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "spacing", &font_string->spacing);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "outline", &font_string->outline);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "monochrome", &font_string->monochrome);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "nonspacewrap", &font_string->nonspacewrap);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "justifyH", &font_string->justify_h);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "justifyV", &font_string->justify_v);
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "maxLines", &font_string->max_lines);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "indented", &font_string->indented);
	return xml_layout_frame_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_font_string *font_string = (struct xml_font_string*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "Color", font_string->color, &xml_color_vtable);
			XML_ELEMENT_CHILD_TEST(child, "FontHeight", font_string->font_height, &xml_value_vtable);
			XML_ELEMENT_CHILD_TEST(child, "Shadow", font_string->shadow, &xml_shadow_vtable);
			break;
		default:
			break;
	}
	return xml_layout_frame_vtable.parse_child(element, child);
}

static struct ui_region *load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return (struct ui_region*)ui_font_string_new(interface, layout_frame->name, parent);
}

const struct xml_vtable xml_font_string_vtable =
{
	.name = "FontString",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
	.load_interface = load_interface,
};
