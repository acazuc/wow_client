#ifndef UI_MESSAGE_FRAME_H
#define UI_MESSAGE_FRAME_H

#include "ui/font_instance.h"
#include "ui/frame.h"

#ifdef interface
# undef interface
#endif

struct ui_message_frame
{
	struct ui_frame frame;
	struct ui_font_instance font_instance;
};

extern const struct ui_object_vtable ui_message_frame_vtable;

#endif
