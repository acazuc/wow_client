#ifndef XML_BUTTON_H
#define XML_BUTTON_H

#include "xml/font_string.h"
#include "xml/dimension.h"
#include "xml/texture.h"
#include "xml/frame.h"
#include "xml/font.h"

#ifdef interface
# undef interface
#endif

struct xml_button
{
	struct xml_frame frame;
	struct optional_xml_font_string button_text;
	struct optional_xml_dimension pushed_text_offset;
	struct optional_xml_texture normal_texture;
	struct optional_xml_texture pushed_texture;
	struct optional_xml_texture disabled_texture;
	struct optional_xml_texture highlight_texture;
	struct optional_xml_color normal_color;
	struct optional_xml_color highlight_color;
	struct optional_xml_color disabled_color;
	struct optional_xml_font normal_font;
	struct optional_xml_font highlight_font;
	struct optional_xml_font disabled_font;
	char *text;
};

extern const struct xml_vtable xml_button_vtable;

#endif
