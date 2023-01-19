#ifndef XML_FONT_H
#define XML_FONT_H

#include "xml/element.h"
#include "xml/shadow.h"
#include "xml/color.h"
#include "xml/value.h"

#ifdef interface
# undef interface
#endif

struct xml_font
{
	struct xml_element element;
	struct optional_xml_value font_height;
	struct optional_xml_shadow shadow;
	struct optional_xml_color color;
	char *name;
	char *inherits;
	struct optional_bool is_virtual;
	char *font;
	struct optional_float spacing;
	char *outline;
	struct optional_bool monochrome;
	char *justify_v;
	char *justify_h;
};

OPTIONAL_DEF(optional_xml_font, struct xml_font);

extern const struct xml_vtable xml_font_vtable;

struct ui_object *xml_font_load_into_interface(struct xml_font *font, struct interface *interface);

#endif
