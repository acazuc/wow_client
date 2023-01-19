#ifndef UI_MOVIE_FRAME_H
#define UI_MOVIE_FRAME_H

#include "ui/frame.h"

#ifdef interface
# undef interface
#endif

struct ui_movie_frame
{
	struct ui_frame frame;
};

extern const struct ui_object_vtable ui_movie_frame_vtable;

#endif
