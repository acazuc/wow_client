#ifndef XML_LAYERS_H
#define XML_LAYERS_H

#include "xml/element.h"

#include <jks/array.h>

struct xml_layer
{
	struct xml_element element;
	struct jks_array elements; /* struct xml_layout_frame* */
	char *level;
};

struct xml_layers
{
	struct xml_element element;
	struct jks_array layers; /* struct xml_layer* */
};

OPTIONAL_DEF(optional_xml_layers, struct xml_layers);

extern const struct xml_vtable xml_layer_vtable;
extern const struct xml_vtable xml_layers_vtable;

#endif
