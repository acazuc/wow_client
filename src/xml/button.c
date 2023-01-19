#include "button.h"
#include "internal.h"

#include "ui/button.h"

#include "memory.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_frame_vtable.ctr(element);
	struct xml_button *button = (struct xml_button*)element;
	((struct xml_layout_frame*)element)->mask |= XML_LAYOUT_FRAME_button;
	OPTIONAL_UNSET(button->button_text);
	OPTIONAL_UNSET(button->pushed_text_offset);
	OPTIONAL_UNSET(button->normal_texture);
	OPTIONAL_UNSET(button->pushed_texture);
	OPTIONAL_UNSET(button->disabled_texture);
	OPTIONAL_UNSET(button->highlight_texture);
	OPTIONAL_UNSET(button->normal_color);
	OPTIONAL_UNSET(button->highlight_color);
	OPTIONAL_UNSET(button->disabled_color);
	OPTIONAL_UNSET(button->normal_font);
	OPTIONAL_UNSET(button->highlight_font);
	OPTIONAL_UNSET(button->disabled_font);
	button->text = NULL;
}

static void dtr(struct xml_element *element)
{
	struct xml_button *button = (struct xml_button*)element;
	mem_free(MEM_XML, button->text);
	XML_ELEMENT_DTR(button->button_text);
	XML_ELEMENT_DTR(button->normal_texture);
	XML_ELEMENT_DTR(button->pushed_texture);
	XML_ELEMENT_DTR(button->disabled_texture);
	XML_ELEMENT_DTR(button->highlight_texture);
	XML_ELEMENT_DTR(button->normal_color);
	XML_ELEMENT_DTR(button->highlight_color);
	XML_ELEMENT_DTR(button->disabled_color);
	XML_ELEMENT_DTR(button->normal_font);
	XML_ELEMENT_DTR(button->highlight_font);
	XML_ELEMENT_DTR(button->disabled_font);
	xml_frame_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_button *button = (struct xml_button*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "text", &button->text);
	return xml_frame_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_button *button = (struct xml_button*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "NormalTexture", button->normal_texture, &xml_texture_vtable);
			XML_ELEMENT_CHILD_TEST(child, "PushedTexture", button->pushed_texture, &xml_texture_vtable);
			XML_ELEMENT_CHILD_TEST(child, "DisabledTexture", button->disabled_texture, &xml_texture_vtable);
			XML_ELEMENT_CHILD_TEST(child, "HighlightTexture", button->highlight_texture, &xml_texture_vtable);
			XML_ELEMENT_CHILD_TEST(child, "NormalColor", button->normal_color, &xml_color_vtable);
			XML_ELEMENT_CHILD_TEST(child, "HighlightColor", button->highlight_color, &xml_color_vtable);
			XML_ELEMENT_CHILD_TEST(child, "DisabledColor", button->disabled_color, &xml_color_vtable);
			XML_ELEMENT_CHILD_TEST(child, "NormalFont", button->normal_font, &xml_font_vtable);
			XML_ELEMENT_CHILD_TEST(child, "DisabledFont", button->disabled_font, &xml_font_vtable);
			XML_ELEMENT_CHILD_TEST(child, "HighlightFont", button->highlight_font, &xml_font_vtable);
			XML_ELEMENT_CHILD_TEST(child, "ButtonText", button->button_text, &xml_font_string_vtable);
			XML_ELEMENT_CHILD_TEST(child, "PushedTextOffset", button->pushed_text_offset, &xml_dimension_vtable);
			break;
		default:
			break;
	}
	return xml_frame_vtable.parse_child(element, child);
}

static struct ui_region *load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return (struct ui_region*)ui_button_new(interface, layout_frame->name, parent);
}

const struct xml_vtable xml_button_vtable =
{
	.name = "Button",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
	.load_interface = load_interface,
};
