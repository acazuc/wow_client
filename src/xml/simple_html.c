#include "xml/simple_html.h"
#include "xml/internal.h"

#include "ui/simple_html.h"

#include "memory.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_frame_vtable.ctr(element);
	struct xml_simple_html *simple_html = (struct xml_simple_html*)element;
	((struct xml_layout_frame*)simple_html)->mask |= XML_LAYOUT_FRAME_simple_html;
	OPTIONAL_UNSET(simple_html->font_string);
	OPTIONAL_UNSET(simple_html->font_string_header1);
	OPTIONAL_UNSET(simple_html->font_string_header2);
	OPTIONAL_UNSET(simple_html->font_string_header3);
	simple_html->font = NULL;
	simple_html->file = NULL;
	simple_html->hyperlink_format = NULL;
}

static void dtr(struct xml_element *element)
{
	struct xml_simple_html *simple_html = (struct xml_simple_html*)element;
	XML_ELEMENT_DTR(simple_html->font_string);
	XML_ELEMENT_DTR(simple_html->font_string_header1);
	XML_ELEMENT_DTR(simple_html->font_string_header2);
	XML_ELEMENT_DTR(simple_html->font_string_header3);
	mem_free(MEM_XML, simple_html->font);
	mem_free(MEM_XML, simple_html->file);
	mem_free(MEM_XML, simple_html->hyperlink_format);
	xml_frame_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_simple_html *simple_html = (struct xml_simple_html*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "font", &simple_html->font);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "file", &simple_html->file);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "hyperlinkFormat", &simple_html->hyperlink_format);
	return xml_frame_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_simple_html *simple_html = (struct xml_simple_html*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "FontString", simple_html->font_string, &xml_font_string_vtable);
			XML_ELEMENT_CHILD_TEST(child, "FontStringHeader1", simple_html->font_string_header1, &xml_font_string_vtable);
			XML_ELEMENT_CHILD_TEST(child, "FontStringHeader2", simple_html->font_string_header2, &xml_font_string_vtable);
			XML_ELEMENT_CHILD_TEST(child, "FontStringHeader3", simple_html->font_string_header3, &xml_font_string_vtable);
			break;
		default:
			break;
	}
	return xml_frame_vtable.parse_child(element, child);
}

static struct ui_region *load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return (struct ui_region*)ui_simple_html_new(interface, layout_frame->name, parent);
}

const struct xml_vtable xml_simple_html_vtable =
{
	.name = "SimpleHTML",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
	.load_interface = load_interface,
};
