#include "frames.h"
#include "internal.h"

#include "xml/layout_frame.h"

#include "memory.h"
#include "log.h"

#include <string.h>

MEMORY_DECL(XML);

static void delete_frame(void *frame)
{
	xml_element_delete(*(struct xml_element**)frame);
}

static void ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_frames *frames = (struct xml_frames*)element;
	jks_array_init(&frames->frames, sizeof(struct xml_layout_frame*), delete_frame, &jks_array_memory_fn_XML);
}

static void dtr(struct xml_element *element)
{
	struct xml_frames *frames = (struct xml_frames*)element;
	jks_array_destroy(&frames->frames);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_frames *frames = (struct xml_frames*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
		{
			struct xml_element *frame = (struct xml_element*)xml_create_layout_frame((const char*)child->name);
			if (!frame)
				break;
			xml_element_parse(frame, child);
			if (!jks_array_push_back(&frames->frames, &frame))
			{
				LOG_ERROR("failed to add frame to list");
				xml_element_delete(frame);
				return XML_PARSE_CHILD_INTERNAL;
			}
			return XML_PARSE_CHILD_OK;
		}
		default:
			break;
	}
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_frames_vtable =
{
	.name = "Frames",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
