#ifndef TAXI_ROUTE_FRAME_H
#define TAXI_ROUTE_FRAME_H

#include "ui/frame.h"

#ifdef interface
# undef interface
#endif

struct ui_taxi_route_frame
{
	struct ui_frame frame;
};

extern const struct ui_object_vtable ui_taxi_route_frame_vtable;

#endif
