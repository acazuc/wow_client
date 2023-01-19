#include "frame.h"
#include "internal.h"

#include "ui/frame.h"

#include "memory.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_layout_frame_vtable.ctr(element);
	struct xml_frame *frame = (struct xml_frame*)element;
	((struct xml_layout_frame*)frame)->mask |= XML_LAYOUT_FRAME_frame;
	OPTIONAL_UNSET(frame->resize_bounds);
	OPTIONAL_UNSET(frame->attributes);
	OPTIONAL_UNSET(frame->backdrop);
	OPTIONAL_UNSET(frame->scripts);
	OPTIONAL_UNSET(frame->layers);
	OPTIONAL_UNSET(frame->frames);
	OPTIONAL_UNSET(frame->hit_rect_insets);
	OPTIONAL_UNSET(frame->alpha);
	frame->parent = NULL;
	OPTIONAL_UNSET(frame->top_level);
	OPTIONAL_UNSET(frame->movable);
	OPTIONAL_UNSET(frame->resizable);
	frame->frame_strata = NULL;
	OPTIONAL_UNSET(frame->frame_level);
	OPTIONAL_UNSET(frame->id);
	OPTIONAL_UNSET(frame->enable_mouse);
	OPTIONAL_UNSET(frame->enable_keyboard);
	OPTIONAL_UNSET(frame->clamped_to_screen);
	OPTIONAL_UNSET(frame->is_protected);
	OPTIONAL_UNSET(frame->title_region);
}

static void dtr(struct xml_element *element)
{
	struct xml_frame *frame = (struct xml_frame*)element;
	XML_ELEMENT_DTR(frame->resize_bounds);
	XML_ELEMENT_DTR(frame->attributes);
	XML_ELEMENT_DTR(frame->backdrop);
	XML_ELEMENT_DTR(frame->scripts);
	XML_ELEMENT_DTR(frame->layers);
	XML_ELEMENT_DTR(frame->frames);
	XML_ELEMENT_DTR(frame->hit_rect_insets);
	mem_free(MEM_XML, frame->parent);
	mem_free(MEM_XML, frame->frame_strata);
	XML_ELEMENT_DTR(frame->title_region);
	xml_layout_frame_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	struct xml_frame *frame = (struct xml_frame*)element;
	XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, "alpha", &frame->alpha);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "parent", &frame->parent);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "toplevel", &frame->top_level);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "movable", &frame->movable);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "resizable", &frame->resizable);
	XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, "frameStrata", &frame->frame_strata);
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "frameLevel", &frame->frame_level);
	XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, "id", &frame->id);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "enableMouse", &frame->enable_mouse);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "enableKeyboard", &frame->enable_keyboard);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "clampedToScreen", &frame->clamped_to_screen);
	XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, "protected", &frame->is_protected);
	return xml_layout_frame_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_frame *frame = (struct xml_frame*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
			XML_ELEMENT_CHILD_TEST(child, "Layers", frame->layers, &xml_layers_vtable);
			XML_ELEMENT_CHILD_TEST(child, "Scripts", frame->scripts, &xml_scripts_vtable);
			XML_ELEMENT_CHILD_TEST(child, "Frames", frame->frames, &xml_frames_vtable);
			XML_ELEMENT_CHILD_TEST(child, "Backdrop", frame->backdrop, &xml_backdrop_vtable);
			XML_ELEMENT_CHILD_TEST(child, "HitRectInsets", frame->hit_rect_insets, &xml_inset_vtable);
			XML_ELEMENT_CHILD_TEST(child, "Attributes", frame->attributes, &xml_attributes_vtable);
			XML_ELEMENT_CHILD_TEST(child, "ResizeBounds", frame->resize_bounds, &xml_resize_bounds_vtable);
			XML_ELEMENT_CHILD_TEST(child, "TitleRegion", frame->title_region, &xml_layout_frame_vtable);
			break;
		default:
			break;
	}
	return xml_layout_frame_vtable.parse_child(element, child);
}

static struct ui_region *load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return (struct ui_region*)ui_frame_new(interface, layout_frame->name, parent);
}

const struct xml_vtable xml_frame_vtable =
{
	.name = "Frame",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
	.load_interface = load_interface,
};
