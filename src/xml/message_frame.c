#include "xml/message_frame.h"
#include "xml/internal.h"

#include "ui/message_frame.h"

#include "memory.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_frame_vtable.ctr(element);
	struct xml_message_frame *message_frame = (struct xml_message_frame*)element;
	((struct xml_layout_frame*)message_frame)->mask |= XML_LAYOUT_FRAME_message_frame;
	OPTIONAL_UNSET(message_frame->font_string);
	OPTIONAL_UNSET(message_frame->text_insets);
	message_frame->font = NULL;
	OPTIONAL_UNSET(message_frame->fade);
	OPTIONAL_UNSET(message_frame->fade_duration);
	OPTIONAL_UNSET(message_frame->display_duration);
	message_frame->insert_mode = NULL;
}

static void dtr(struct xml_element *element)
{
	struct xml_message_frame *message_frame = (struct xml_message_frame*)element;
	XML_ELEMENT_DTR(message_frame->font_string);
	XML_ELEMENT_DTR(message_frame->text_insets);
	mem_free(MEM_XML, message_frame->font);
	mem_free(MEM_XML, message_frame->insert_mode);
	xml_frame_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_message_frame *message_frame = (struct xml_message_frame*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "font", &message_frame->font);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "fade", &message_frame->fade);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "fadeDuration", &message_frame->fade_duration);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "displayDuration", &message_frame->display_duration);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "insertMode", &message_frame->insert_mode);
	return xml_frame_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_message_frame *message_frame = (struct xml_message_frame*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "FontString", message_frame->font_string, &xml_font_string_vtable);
			XML_ELEMENT_CHILD_TEST(child, "TextInsets", message_frame->text_insets, &xml_inset_vtable);
			break;
		default:
			break;
	}
	return xml_frame_vtable.parse_child(element, child);
}

static struct ui_region *load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return (struct ui_region*)ui_message_frame_new(interface, layout_frame->name, parent);
}

const struct xml_vtable xml_message_frame_vtable =
{
	.name = "MessageFrame",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
	.load_interface = load_interface,
};
