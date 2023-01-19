#include "layers.h"
#include "internal.h"

#include "xml/font_string.h"
#include "xml/texture.h"

#include "memory.h"
#include "log.h"

#include <string.h>

MEMORY_DECL(XML);

static void delete_frame(void *frame)
{
	xml_element_delete(*(struct xml_element**)frame);
}

static void layer_ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_layer *layer = (struct xml_layer*)element;
	jks_array_init(&layer->elements, sizeof(struct xml_layout_frame*), (jks_array_destructor_t)delete_frame, &jks_array_memory_fn_XML);
	layer->level = NULL;
}

static void layer_dtr(struct xml_element *element)
{
	struct xml_layer *layer = (struct xml_layer*)element;
	jks_array_destroy(&layer->elements);
	mem_free(MEM_XML, layer->level);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status layer_parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_layer *layer = (struct xml_layer*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "level", &layer->level);
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status layer_parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_layer *layer = (struct xml_layer*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			if (!strcmp((const char*)child->name, "Texture"))
			{
				struct xml_element *child_element = mem_malloc(MEM_XML, sizeof(struct xml_texture));
				if (!child_element)
				{
					LOG_ERROR("texture allocation failed");
					return XML_PARSE_CHILD_INTERNAL;
				}
				child_element->vtable = &xml_texture_vtable;
				child_element->vtable->ctr(child_element);
				if (!jks_array_push_back(&layer->elements, &child_element))
				{
					LOG_ERROR("failed to add texture to list");
					xml_element_delete(child_element);
					return XML_PARSE_CHILD_INTERNAL;
				}
				xml_element_parse(child_element, child);
				return XML_PARSE_CHILD_OK;
			}
			else if (!strcmp((const char*)child->name, "FontString"))
			{
				struct xml_element *child_element = mem_malloc(MEM_XML, sizeof(struct xml_font_string));
				if (!child_element)
				{
					LOG_ERROR("font string allocation failed");
					return XML_PARSE_CHILD_INTERNAL;
				}
				child_element->vtable = &xml_font_string_vtable;
				child_element->vtable->ctr(child_element);
				if (!jks_array_push_back(&layer->elements, &child_element))
				{
					LOG_ERROR("failed to add font strong to list");
					xml_element_delete(child_element);
					return XML_PARSE_CHILD_INTERNAL;
				}
				xml_element_parse(child_element, child);
				return XML_PARSE_CHILD_OK;
			}
			break;
		default:
			break;
	}
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_layer_vtable =
{
	.name = "Layer",
	.ctr = layer_ctr,
	.dtr = layer_dtr,
	.parse_attribute = layer_parse_attribute,
	.parse_child = layer_parse_child,
};

static void delete_layer(void *layer)
{
	xml_element_delete(*(struct xml_element**)layer);
}

static void ctr(struct xml_element *element)
{
	struct xml_layers *layers = (struct xml_layers*)element;
	xml_element_vtable.ctr(element);
	jks_array_init(&layers->layers, sizeof(struct xml_layer*), delete_layer, &jks_array_memory_fn_XML);
}

static void dtr(struct xml_element *element)
{
	struct xml_layers *layers = (struct xml_layers*)element;
	jks_array_destroy(&layers->layers);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_layers *layers = (struct xml_layers*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			if (!strcmp((const char*)child->name, "Layer"))
			{
				struct xml_element *layer = mem_malloc(MEM_XML, sizeof(struct xml_layer));
				if (!element)
				{
					LOG_ERROR("layer allocation failed");
					return XML_PARSE_CHILD_INTERNAL;
				}
				layer->vtable = &xml_layer_vtable;
				layer->vtable->ctr(layer);
				if (!jks_array_push_back(&layers->layers, &layer))
				{
					LOG_ERROR("failed to push layer");
					xml_element_delete(layer);
					return XML_PARSE_CHILD_INTERNAL;

				}
				xml_element_parse(layer, child);
				return XML_PARSE_CHILD_OK;
			}
			break;
		default:
			break;
	}
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_layers_vtable =
{
	.name = "Layers",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
