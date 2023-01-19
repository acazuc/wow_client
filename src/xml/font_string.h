#ifndef XML_FONT_STRING_H
#define XML_FONT_STRING_H

#include "xml/layout_frame.h"
#include "xml/shadow.h"
#include "xml/value.h"
#include "xml/color.h"

#ifdef interface
# undef interface
#endif

struct xml_font_string
{
	struct xml_layout_frame layout_frame;
	struct optional_xml_value font_height;
	struct optional_xml_color color;
	struct optional_xml_shadow shadow;
	char *font;
	struct optional_int32 bytes;
	char *text;
	struct optional_float spacing;
	char *outline;
	struct optional_bool monochrome;
	struct optional_bool nonspacewrap;
	char *justify_h;
	char *justify_v;
	struct optional_int32 max_lines;
	struct optional_bool indented;
};

OPTIONAL_DEF(optional_xml_font_string, struct xml_font_string);

extern const struct xml_vtable xml_font_string_vtable;

#endif
