#include "xml/scrolling_message_frame.h"
#include "xml/internal.h"

#include "ui/scrolling_message_frame.h"

#include "memory.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_frame_vtable.ctr(element);
	struct xml_scrolling_message_frame *scrolling_message_frame = (struct xml_scrolling_message_frame*)element;
	OPTIONAL_UNSET(scrolling_message_frame->font_string);
	OPTIONAL_UNSET(scrolling_message_frame->text_insets);
	scrolling_message_frame->font = NULL;
	OPTIONAL_UNSET(scrolling_message_frame->fade);
	OPTIONAL_UNSET(scrolling_message_frame->fade_duration);
	OPTIONAL_UNSET(scrolling_message_frame->display_duration);
	scrolling_message_frame->insert_mode = NULL;
	OPTIONAL_UNSET(scrolling_message_frame->max_lines);
}

static void dtr(struct xml_element *element)
{
	struct xml_scrolling_message_frame *scrolling_message_frame = (struct xml_scrolling_message_frame*)element;
	XML_ELEMENT_DTR(scrolling_message_frame->font_string);
	XML_ELEMENT_DTR(scrolling_message_frame->text_insets);
	mem_free(MEM_XML, scrolling_message_frame->font);
	mem_free(MEM_XML, scrolling_message_frame->insert_mode);
	xml_frame_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_scrolling_message_frame *scrolling_message_frame = (struct xml_scrolling_message_frame*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "font", &scrolling_message_frame->font);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "fade", &scrolling_message_frame->fade);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "fadeDuration", &scrolling_message_frame->fade_duration);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "displayDuration", &scrolling_message_frame->display_duration);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "insertMode", &scrolling_message_frame->insert_mode);
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "maxLines", &scrolling_message_frame->max_lines);
	return xml_frame_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_scrolling_message_frame *scrolling_message_frame = (struct xml_scrolling_message_frame*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "TextInsets", scrolling_message_frame->text_insets, &xml_inset_vtable);
			XML_ELEMENT_CHILD_TEST(child, "FontString", scrolling_message_frame->font_string, &xml_font_string_vtable);
			break;
		default:
			break;
	}
	return xml_frame_vtable.parse_child(element, child);
}

static struct ui_region *load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return (struct ui_region*)ui_scrolling_message_frame_new(interface, layout_frame->name, parent);
}

const struct xml_vtable xml_scrolling_message_frame_vtable =
{
	.name = "ScrollingMessageFrame",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
	.load_interface = load_interface,
};
