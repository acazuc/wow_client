#ifndef XML_EDIT_BOX_H
#define XML_EDIT_BOX_H

#include "xml/font_string.h"
#include "xml/frame.h"

#ifdef interface
# undef interface
#endif

struct xml_edit_box
{
	struct xml_frame frame;
	struct optional_xml_font_string font_string;
	struct optional_xml_inset text_insets;
	char *font;
	struct optional_int32 letters;
	struct optional_float blink_speed;
	struct optional_bool numeric;
	struct optional_bool password;
	struct optional_bool multi_line;
	struct optional_int32 history_lines;
	struct optional_bool auto_focus;
	struct optional_bool ignore_arrows;
};

extern const struct xml_vtable xml_edit_box_vtable;

#endif
