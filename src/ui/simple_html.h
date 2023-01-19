#ifndef UI_SIMPLE_HTML_H
#define UI_SIMPLE_HTML_H

#include "ui/font_instance.h"
#include "ui/frame.h"

#ifdef interface
# undef interface
#endif

struct xml_font_string;
struct ui_font;

struct ui_simple_html
{
	struct ui_frame frame;
	struct ui_font_instance font_instance;
	const struct xml_font_string *paragraph_font_string;
	const struct xml_font_string *header1_font_string;
	const struct xml_font_string *header2_font_string;
	const struct xml_font_string *header3_font_string;
	struct ui_font_string *font_string;
	struct ui_font *font;
	char *hyperlink_format;
	char *text;
};

extern const struct ui_object_vtable ui_simple_html_vtable;

#endif
