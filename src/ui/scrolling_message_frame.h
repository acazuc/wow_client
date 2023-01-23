#ifndef UI_SCROLLING_MESSAGE_FRAME_H
#define UI_SCROLLING_MESSAGE_FRAME_H

#include "ui/font_instance.h"
#include "ui/frame.h"

#ifdef interface
# undef interface
#endif

struct ui_font_string;

struct ui_scrolling_message_frame
{
	struct ui_frame frame;
	struct ui_font_instance font_instance;
	struct jks_array messages; /* struct ui_scrolling_message* */
	struct ui_inset text_insets;
	struct ui_font_string *font_string;
};

extern const struct ui_object_vtable ui_scrolling_message_frame_vtable;

#endif
