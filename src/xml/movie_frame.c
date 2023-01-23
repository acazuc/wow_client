#include "xml/movie_frame.h"
#include "xml/internal.h"

#include "ui/movie_frame.h"

#ifdef interface
# undef interface
#endif

static void ctr(struct xml_element *element)
{
	xml_frame_vtable.ctr(element);
	struct xml_movie_frame *movie_frame = (struct xml_movie_frame*)element;
	((struct xml_layout_frame*)movie_frame)->mask |= XML_LAYOUT_FRAME_movie_frame;
}

static void dtr(struct xml_element *element)
{
	xml_frame_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	return xml_frame_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	return xml_frame_vtable.parse_child(element, child);
}

static struct ui_region *load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent)
{
	return (struct ui_region*)ui_movie_frame_new(interface, layout_frame->name, parent);
}

const struct xml_vtable xml_movie_frame_vtable =
{
	.name = "MovieFrame",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
	.load_interface = load_interface,
};
