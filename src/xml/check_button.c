#include "check_button.h"
#include "internal.h"

#include "ui/check_button.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_button_vtable.ctr(element);
	struct xml_check_button *check_button = (struct xml_check_button*)element;
	((struct xml_layout_frame*)check_button)->mask |= XML_LAYOUT_FRAME_check_button;
	OPTIONAL_UNSET(check_button->checked_texture);
	OPTIONAL_UNSET(check_button->disabled_checked_texture);
	OPTIONAL_UNSET(check_button->checked);
}

static void dtr(struct xml_element *element)
{
	struct xml_check_button *check_button = (struct xml_check_button*)element;
	XML_ELEMENT_DTR(check_button->checked_texture);
	XML_ELEMENT_DTR(check_button->disabled_checked_texture);
	xml_button_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_check_button *check_button = (struct xml_check_button*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "checked", &check_button->checked);
	return xml_button_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_check_button *check_button = (struct xml_check_button*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "CheckedTexture", check_button->checked_texture, &xml_texture_vtable);
			XML_ELEMENT_CHILD_TEST(child, "DisabledCheckedTexture", check_button->disabled_checked_texture, &xml_texture_vtable);
			break;
		default:
			break;
	}
	return xml_button_vtable.parse_child(element, child);
}

static struct ui_region *load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return (struct ui_region*)ui_check_button_new(interface, layout_frame->name, parent);
}

const struct xml_vtable xml_check_button_vtable =
{
	.name = "CheckButton",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
	.load_interface = load_interface,
};
