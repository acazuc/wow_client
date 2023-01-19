#ifndef XML_MESSAGE_FRAME_H
#define XML_MESSAGE_FRAME_H

#include "xml/font_string.h"
#include "xml/frame.h"
#include "xml/inset.h"

#ifdef interface
# undef interface
#endif

struct xml_message_frame
{
	struct xml_frame frame;
	struct optional_xml_font_string font_string;
	struct optional_xml_inset text_insets;
	char *font;
	struct optional_bool fade;
	struct optional_float fade_duration;
	struct optional_float display_duration;
	char *insert_mode;
};

extern const struct xml_vtable xml_message_frame_vtable;

#endif
