#include "model.h"
#include "internal.h"

#include "ui/model.h"

#include "memory.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_frame_vtable.ctr(element);
	struct xml_model *model = (struct xml_model*)element;
	((struct xml_layout_frame*)model)->mask |= XML_LAYOUT_FRAME_model;
	OPTIONAL_UNSET(model->fog_color);
	model->file = NULL;
	OPTIONAL_UNSET(model->scale);
	OPTIONAL_UNSET(model->fog_near);
	OPTIONAL_UNSET(model->fog_far);
}

static void dtr(struct xml_element *element)
{
	struct xml_model *model = (struct xml_model*)element;
	XML_ELEMENT_DTR(model->fog_color);
	mem_free(MEM_XML, model->file);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_model *model = (struct xml_model*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "file", &model->file);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "scale", &model->scale);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "fogNear", &model->fog_near);
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "fogFar", &model->fog_far);
	return xml_frame_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_model *model = (struct xml_model*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "FogColor", model->fog_color, &xml_color_vtable);
			break;
		default:
			break;
	}
	return xml_frame_vtable.parse_child(element, child);
}

static struct ui_region *load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return (struct ui_region*)ui_model_new(interface, layout_frame->name, parent);
}

const struct xml_vtable xml_model_vtable =
{
	.name = "Model",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
	.load_interface = load_interface,
};
