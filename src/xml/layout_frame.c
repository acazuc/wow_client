#include "layout_frame.h"
#include "internal.h"

#include "itf/interface.h"

#include "xml/taxi_route_frame.h"
#include "xml/message_frame.h"
#include "xml/check_button.h"
#include "xml/game_tooltip.h"
#include "xml/player_model.h"
#include "xml/scroll_frame.h"
#include "xml/tabard_model.h"
#include "xml/color_select.h"
#include "xml/font_string.h"
#include "xml/movie_frame.h"
#include "xml/simple_html.h"
#include "xml/status_bar.h"
#include "xml/model_ffx.h"
#include "xml/cooldown.h"
#include "xml/minimap.h"
#include "xml/edit_box.h"
#include "xml/texture.h"
#include "xml/button.h"
#include "xml/slider.h"
#include "xml/model.h"
#include "xml/frame.h"
#include "xml/font.h"

#include "memory.h"
#include "log.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

MEMORY_DECL(XML);

void xml_layout_frame_resolve_inherits(struct xml_layout_frame *layout_frame)
{
	if (layout_frame->inherits_vector.size)
		return;
	if (!layout_frame->inherits || !layout_frame->inherits[0])
		return;
	char *prv = layout_frame->inherits;
	char *ite;
	while ((ite = strchr(prv, ',')))
	{
		if (prv[0] == ' ')
			prv++;
		char *dup = mem_malloc(MEM_XML, ite - prv + 1);
		if (!dup)
		{
			LOG_ERROR("failed to allocate inherits name");
			return;
		}
		memcpy(dup, prv, ite - prv);
		dup[ite - prv] = '\0';
		if (!jks_array_push_back(&layout_frame->inherits_vector, &dup))
		{
			LOG_ERROR("failed to push inherits");
			mem_free(MEM_XML, dup);
			return;
		}
		prv = ite + 1;
	}
	if (*prv)
	{
		if (prv[0] == ' ')
			prv++;
		if (*prv)
		{
			char *dup = mem_strdup(MEM_XML, prv);
			if (!dup)
			{
				LOG_ERROR("failed to allocate inherits name");
				return;
			}
			if (!jks_array_push_back(&layout_frame->inherits_vector, &dup))
			{
				LOG_ERROR("failed to push inherits");
				mem_free(MEM_XML, dup);
				return;
			}
		}
	}
}

#define ELEMENT_CAST_DEFINE(name) \
const struct xml_##name *xml_clayout_frame_as_##name(const struct xml_layout_frame *layout_frame) \
{ \
	if (!(layout_frame->mask & XML_LAYOUT_FRAME_##name)) \
		return NULL; \
	return (const struct xml_##name*)layout_frame; \
} \
struct xml_##name *xml_layout_frame_as_##name(struct xml_layout_frame *layout_frame) \
{ \
	if (!(layout_frame->mask & XML_LAYOUT_FRAME_##name)) \
		return NULL; \
	return (struct xml_##name*)layout_frame; \
}

ELEMENT_CAST_DEFINE(button);
ELEMENT_CAST_DEFINE(check_button);
ELEMENT_CAST_DEFINE(color_select);
ELEMENT_CAST_DEFINE(cooldown);
ELEMENT_CAST_DEFINE(edit_box);
ELEMENT_CAST_DEFINE(font_string);
ELEMENT_CAST_DEFINE(frame);
ELEMENT_CAST_DEFINE(game_tooltip);
ELEMENT_CAST_DEFINE(message_frame);
ELEMENT_CAST_DEFINE(minimap);
ELEMENT_CAST_DEFINE(model);
ELEMENT_CAST_DEFINE(model_ffx);
ELEMENT_CAST_DEFINE(movie_frame);
ELEMENT_CAST_DEFINE(player_model);
ELEMENT_CAST_DEFINE(scroll_frame);
ELEMENT_CAST_DEFINE(scrolling_message_frame);
ELEMENT_CAST_DEFINE(simple_html);
ELEMENT_CAST_DEFINE(slider);
ELEMENT_CAST_DEFINE(status_bar);
ELEMENT_CAST_DEFINE(tabard_model);
ELEMENT_CAST_DEFINE(taxi_route_frame);
ELEMENT_CAST_DEFINE(texture);

#undef ELEMENT_CAST_DEFINE

static void inherits_dtr(void *ptr)
{
	mem_free(MEM_XML, *(char**)ptr);
}

static void ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_layout_frame *layout_frame = (struct xml_layout_frame*)element;
	layout_frame->mask = 0;
	OPTIONAL_UNSET(layout_frame->anchors);
	OPTIONAL_UNSET(layout_frame->size);
	layout_frame->name = NULL;
	layout_frame->inherits = NULL;
	OPTIONAL_UNSET(layout_frame->is_virtual);
	OPTIONAL_UNSET(layout_frame->set_all_points);
	OPTIONAL_UNSET(layout_frame->hidden);
	jks_array_init(&layout_frame->inherits_vector, sizeof(char*), inherits_dtr, &jks_array_memory_fn_XML);
}

static void dtr(struct xml_element *element)
{
	struct xml_layout_frame *layout_frame = (struct xml_layout_frame*)element;
	XML_ELEMENT_DTR(layout_frame->anchors);
	XML_ELEMENT_DTR(layout_frame->size);
	mem_free(MEM_XML, layout_frame->name);
	mem_free(MEM_XML, layout_frame->inherits);
	jks_array_destroy(&layout_frame->inherits_vector);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_layout_frame *layout_frame = (struct xml_layout_frame*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "name", &layout_frame->name);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "inherits", &layout_frame->inherits);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "virtual", &layout_frame->is_virtual);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "setAllPoints", &layout_frame->set_all_points);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "hidden", &layout_frame->hidden);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_layout_frame *layout_frame = (struct xml_layout_frame*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "Size", layout_frame->size, &xml_dimension_vtable);
			XML_ELEMENT_CHILD_TEST(child, "Anchors", layout_frame->anchors, &xml_anchors_vtable);
			if (!strcmp(child->name, "Animations"))
			{
				LOG_ERROR("unimplemented <XMLLayoutFrame>::<Animations>");
				return XML_PARSE_CHILD_OK;
			}
			break;
		default:
			break;
	}
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_layout_frame_vtable =
{
	.name = "LayoutFrame",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
