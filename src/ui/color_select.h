#ifndef UI_COLOR_SELECT_H
#define UI_COLOR_SELECT_H

#include "ui/frame.h"

#ifdef interface
# undef interface
#endif

struct ui_color_select
{
	struct ui_frame frame;
};

extern const struct ui_object_vtable ui_color_select_vtable;

#endif
