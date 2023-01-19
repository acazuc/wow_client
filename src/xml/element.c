#include "element.h"
#include "internal.h"

#include "xml/scrolling_message_frame.h"
#include "xml/taxi_route_frame.h"
#include "xml/dress_up_model.h"
#include "xml/message_frame.h"
#include "xml/scroll_frame.h"
#include "xml/check_button.h"
#include "xml/game_tooltip.h"
#include "xml/player_model.h"
#include "xml/tabard_model.h"
#include "xml/color_select.h"
#include "xml/font_string.h"
#include "xml/world_frame.h"
#include "xml/simple_html.h"
#include "xml/movie_frame.h"
#include "xml/status_bar.h"
#include "xml/model_ffx.h"
#include "xml/cooldown.h"
#include "xml/edit_box.h"
#include "xml/include.h"
#include "xml/texture.h"
#include "xml/minimap.h"
#include "xml/button.h"
#include "xml/slider.h"
#include "xml/script.h"
#include "xml/frame.h"
#include "xml/model.h"

#include "memory.h"
#include "log.h"

#include <libxml/tree.h>

#include <string.h>
#include <errno.h>

static void ctr(struct xml_element *element)
{
	(void)element;
}

static void dtr(struct xml_element *element)
{
	(void)element;
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	(void)element;
	(void)attribute;
	return XML_PARSE_ATTRIBUTE_INVALID_NAME;
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	(void)element;
	switch (child->type)
	{
		case XML_NODE_COMMENT:
			return XML_PARSE_CHILD_OK;
		default:
			return XML_PARSE_CHILD_INVALID_NAME;
	}
	return XML_PARSE_CHILD_INVALID_NAME;
}

const struct xml_vtable xml_element_vtable =
{
	.name = "Element",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};

enum xml_parse_attribute_status xml_parse_attribute_boolean(const struct xml_attr *attribute, struct optional_bool *value)
{
	if (!attribute->value)
		return XML_PARSE_ATTRIBUTE_INVALID_VALUE;
	if (!strcmp(attribute->value, "true"))
		OPTIONAL_CTR(*value, true);
	else if (!strcmp(attribute->value, "false"))
		OPTIONAL_CTR(*value, false);
	else if (!strcmp(attribute->value, "1"))
		OPTIONAL_CTR(*value, true);
	else if (!strcmp(attribute->value, "0"))
		OPTIONAL_CTR(*value, false);
	else
		return XML_PARSE_ATTRIBUTE_INVALID_VALUE;
	return XML_PARSE_ATTRIBUTE_OK;
}

enum xml_parse_attribute_status xml_parse_attribute_integer(const struct xml_attr *attribute, struct optional_int32 *value)
{
	if (!attribute->value)
		return XML_PARSE_ATTRIBUTE_INVALID_VALUE;
	errno = 0;
	OPTIONAL_CTR(*value, strtol(attribute->value, NULL, 10));
	if (errno)
		return XML_PARSE_ATTRIBUTE_INVALID_VALUE;
	return XML_PARSE_ATTRIBUTE_OK;
}

enum xml_parse_attribute_status xml_parse_attribute_float(const struct xml_attr *attribute, struct optional_float *value)
{
	if (!attribute->value)
		return XML_PARSE_ATTRIBUTE_INVALID_VALUE;
	errno = 0;
	OPTIONAL_CTR(*value, strtof(attribute->value, NULL));
	if (errno)
		return XML_PARSE_ATTRIBUTE_INVALID_VALUE;
	return XML_PARSE_ATTRIBUTE_OK;
}

enum xml_parse_attribute_status xml_parse_attribute_string(const struct xml_attr *attribute, char **value)
{
	if (!attribute->value)
		return XML_PARSE_ATTRIBUTE_INVALID_VALUE;
	free(*value);
	*value = mem_strdup(MEM_XML, attribute->value);
	return XML_PARSE_ATTRIBUTE_OK;
}

static void parse_attributes(struct xml_element *element, const xmlAttr *attributes)
{
	for (const xmlAttr *attribute = attributes; attribute; attribute = attribute->next)
	{
		struct xml_attr attr;
		attr.name = (const char*)attribute->name;
		if (attribute->children)
			attr.value = (const char*)attribute->children->content;
		else
			attr.value = NULL;
		switch (element->vtable->parse_attribute(element, &attr))
		{
			case XML_PARSE_ATTRIBUTE_OK:
				break;
			case XML_PARSE_ATTRIBUTE_INVALID_NAME:
				LOG_ERROR("invalid <%s> attribute: %s", element->vtable->name, attr.name);
				break;
			case XML_PARSE_ATTRIBUTE_INVALID_VALUE:
				LOG_ERROR("invalid <%s> attribute value for %s", element->vtable->name, attr.name);
				break;
		}
	}
}

static void parse_childs(struct xml_element *element, const xmlNode *childs)
{
	for (const xmlNode *child = childs; child; child = child->next)
	{
		struct xml_node node;
		xml_node_parse(&node, child);
		switch (element->vtable->parse_child(element, &node))
		{
			case XML_PARSE_CHILD_OK:
				break;
			case XML_PARSE_CHILD_INVALID_NAME:
				LOG_ERROR("invalid <%s> child: <%s>", element->vtable->name, node.name);
				break;
			case XML_PARSE_CHILD_INVALID_TYPE:
				LOG_ERROR("invalid <%s> child type for <%s>: %u", element->vtable->name, node.name, node.type);
				break;
			case XML_PARSE_CHILD_ALREADY:
				LOG_ERROR("<%s> already contains a <%s>", element->vtable->name, node.name);
				break;
			case XML_PARSE_CHILD_INTERNAL:
				LOG_ERROR("failed to parse <%s> child <%s>: internal error", element->vtable->name, node.name);
				break;
		}
	}
}

void xml_element_parse(struct xml_element *element, const struct xml_node *node)
{
	parse_attributes(element, node->node->properties);
	parse_childs(element, node->node->children);
}

struct xml_layout_frame *xml_create_layout_frame(const char *name)
{
#define LAYOUT_FRAME_TEST(name, key, st) \
do \
{ \
	if (!strcmp(name, key)) \
	{ \
		struct xml_element *layout_frame = mem_malloc(MEM_XML, sizeof(struct xml_##st)); \
		if (!layout_frame) \
		{ \
			LOG_ERROR("failed to allocate layout frame %s", #key); \
			return NULL; \
		} \
		layout_frame->vtable = &xml_##st##_vtable; \
		xml_##st##_vtable.ctr(layout_frame); \
		return (struct xml_layout_frame*)layout_frame; \
	} \
} while (0)

	LAYOUT_FRAME_TEST(name, "Frame", frame);
	LAYOUT_FRAME_TEST(name, "CheckButton", check_button);
	LAYOUT_FRAME_TEST(name, "Button", button);
	LAYOUT_FRAME_TEST(name, "Slider", slider);
	LAYOUT_FRAME_TEST(name, "Texture", texture);
	LAYOUT_FRAME_TEST(name, "ScrollFrame", scroll_frame);
	LAYOUT_FRAME_TEST(name, "StatusBar", status_bar);
	LAYOUT_FRAME_TEST(name, "FontString", font_string);
	LAYOUT_FRAME_TEST(name, "EditBox", edit_box);
	LAYOUT_FRAME_TEST(name, "Cooldown", cooldown);
	LAYOUT_FRAME_TEST(name, "GameTooltip", game_tooltip);
	LAYOUT_FRAME_TEST(name, "TaxiRouteFrame", taxi_route_frame);
	LAYOUT_FRAME_TEST(name, "WorldFrame", world_frame);
	LAYOUT_FRAME_TEST(name, "Minimap", minimap);
	LAYOUT_FRAME_TEST(name, "MessageFrame", message_frame);
	LAYOUT_FRAME_TEST(name, "Model", model);
	LAYOUT_FRAME_TEST(name, "PlayerModel", player_model);
	LAYOUT_FRAME_TEST(name, "DressUpModel", dress_up_model);
	LAYOUT_FRAME_TEST(name, "TabardModel", tabard_model);
	LAYOUT_FRAME_TEST(name, "ColorSelect", color_select);
	LAYOUT_FRAME_TEST(name, "ScrollingMessageFrame", scrolling_message_frame);
	LAYOUT_FRAME_TEST(name, "SimpleHTML", simple_html);
	LAYOUT_FRAME_TEST(name, "ModelFFX", model_ffx);
	LAYOUT_FRAME_TEST(name, "MovieFrame", movie_frame);
	LOG_ERROR("unknown frame type: %s", name);
	return NULL;

#undef LAYOUT_FRAME_TEST
}

void xml_node_parse(struct xml_node *node, const xmlNode *n)
{
	switch (n->type)
	{
		case XML_ELEMENT_NODE:
			node->type = XML_NODE_ELEMENT;
			break;
		case XML_COMMENT_NODE:
			node->type = XML_NODE_COMMENT;
			break;
		case XML_TEXT_NODE:
			node->type = XML_NODE_TEXT;
			break;
		default:
			LOG_INFO("unknown node type: %d", n->type);
			return;
	}
	node->name = (const char*)n->name;
	node->value = (const char*)n->content;
	node->node = n;
}

void xml_element_delete(struct xml_element *element)
{
	if (!element)
		return;
	element->vtable->dtr(element);
	mem_free(MEM_XML, element);
}

struct ui_region *xml_load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return ((struct xml_element*)layout_frame)->vtable->load_interface(layout_frame, interface, parent);
}
