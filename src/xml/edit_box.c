#include "xml/edit_box.h"
#include "xml/internal.h"

#include "ui/edit_box.h"

#include "memory.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_frame_vtable.ctr(element);
	struct xml_edit_box *edit_box = (struct xml_edit_box*)element;
	((struct xml_layout_frame*)edit_box)->mask |= XML_LAYOUT_FRAME_edit_box;
	OPTIONAL_UNSET(edit_box->font_string);
	OPTIONAL_UNSET(edit_box->text_insets);
	edit_box->font = NULL;
	OPTIONAL_UNSET(edit_box->letters);
	OPTIONAL_UNSET(edit_box->blink_speed);
	OPTIONAL_UNSET(edit_box->numeric);
	OPTIONAL_UNSET(edit_box->password);
	OPTIONAL_UNSET(edit_box->multi_line);
	OPTIONAL_UNSET(edit_box->history_lines);
	OPTIONAL_UNSET(edit_box->auto_focus);
	OPTIONAL_UNSET(edit_box->ignore_arrows);
}

static void dtr(struct xml_element *element)
{
	struct xml_edit_box *edit_box = (struct xml_edit_box*)element;
	XML_ELEMENT_DTR(edit_box->font_string);
	XML_ELEMENT_DTR(edit_box->text_insets);
	mem_free(MEM_XML, edit_box->font);
	xml_frame_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_edit_box *edit_box = (struct xml_edit_box*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "font", &edit_box->font);
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "letters", &edit_box->letters);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "blinkSpeed", &edit_box->blink_speed);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "numeric", &edit_box->numeric);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "password", &edit_box->password);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "multiLine", &edit_box->multi_line);
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "historyLines", &edit_box->history_lines);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "autoFocus", &edit_box->auto_focus);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "ignoreArrows", &edit_box->ignore_arrows);
	return xml_frame_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_edit_box *edit_box = (struct xml_edit_box*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "FontString", edit_box->font_string, &xml_font_string_vtable);
			XML_ELEMENT_CHILD_TEST(child, "TextInsets", edit_box->text_insets, &xml_inset_vtable);
			break;
		default:
			break;
	}
	return xml_frame_vtable.parse_child(element, child);
}

static struct ui_region *load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return (struct ui_region*)ui_edit_box_new(interface, layout_frame->name, parent);
}

const struct xml_vtable xml_edit_box_vtable =
{
	.name = "EditBox",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
	.load_interface = load_interface,
};
