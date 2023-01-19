#ifndef XML_TAXI_ROUTE_FRAME_H
#define XML_TAXI_ROUTE_FRAME_H

#include "xml/frame.h"

#ifdef interface
# undef interface
#endif

struct xml_taxi_route_frame
{
	struct xml_frame frame;
};

extern const struct xml_vtable xml_taxi_route_frame_vtable;

#endif
