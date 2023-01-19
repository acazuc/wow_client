#ifndef XML_SIMPLE_HTML_H
#define XML_SIMPLE_HTML_H

#include "xml/font_string.h"
#include "xml/frame.h"

#ifdef interface
# undef interface
#endif

struct xml_simple_html
{
	struct xml_frame frame;
	struct optional_xml_font_string font_string;
	struct optional_xml_font_string font_string_header1;
	struct optional_xml_font_string font_string_header2;
	struct optional_xml_font_string font_string_header3;
	char *font;
	char *file;
	char *hyperlink_format;
};

extern const struct xml_vtable xml_simple_html_vtable;

#endif
