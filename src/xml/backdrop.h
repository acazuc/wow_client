#ifndef XML_BACKDROP_H
#define XML_BACKDROP_H

#include "xml/element.h"
#include "xml/value.h"
#include "xml/inset.h"
#include "xml/color.h"

struct xml_backdrop
{
	struct xml_element element;
	struct optional_xml_inset background_insets;
	struct optional_xml_value edge_size;
	struct optional_xml_value tile_size;
	char *bg_file;
	char *edge_file;
	struct optional_xml_color color;
	struct optional_xml_color border_color;
	struct optional_bool tile;
};

OPTIONAL_DEF(optional_xml_backdrop, struct xml_backdrop);

extern const struct xml_vtable xml_backdrop_vtable;

#endif
