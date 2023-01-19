#ifndef XML_SCROLLING_MESSAGE_FRAME_H
#define XML_SCROLLING_MESSAGE_FRAME_H

#include "xml/font_string.h"
#include "xml/inset.h"
#include "xml/frame.h"

struct xml_scrolling_message_frame
{
	struct xml_frame frame;
	struct optional_xml_font_string font_string;
	struct optional_xml_inset text_insets;
	char *font;
	struct optional_bool fade;
	struct optional_float fade_duration;
	struct optional_float display_duration;
	char *insert_mode;
	struct optional_int32 max_lines;
};

extern const struct xml_vtable xml_scrolling_message_frame_vtable;

#endif
