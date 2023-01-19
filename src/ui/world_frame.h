#ifndef WORLD_FRAME_H
#define WORLD_FRAME_H

#include "ui/frame.h"

#ifdef interface
# undef interface
#endif

struct ui_world_frame
{
	struct ui_frame frame;
};

extern const struct ui_object_vtable ui_world_frame_vtable;

#endif
