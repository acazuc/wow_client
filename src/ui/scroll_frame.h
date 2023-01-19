#ifndef UI_SCROLL_FRAME_H
#define UI_SCROLL_FRAME_H

#include "ui/frame.h"

#ifdef interface
# undef interface
#endif

struct ui_scroll_frame
{
	struct ui_frame frame;
	struct ui_frame *scroll_child;
	float vertical_scroll;
	float horizontal_scroll;
};

extern const struct ui_object_vtable ui_scroll_frame_vtable;

void ui_scroll_frame_set_vertical_scroll(struct ui_scroll_frame *scroll_frame, float scroll);
void ui_scroll_frame_set_horizontal_scroll(struct ui_scroll_frame *scroll_frame, float scroll);

#endif
