#ifndef UI_COOLDOWN_H
#define UI_COOLDOWN_H

#include "ui/frame.h"

#ifdef interface
# undef interface
#endif

struct ui_cooldown
{
	struct ui_frame frame;
};

extern const struct ui_object_vtable ui_cooldown_vtable;

#endif
