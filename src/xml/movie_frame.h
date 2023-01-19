#ifndef XML_MOVIE_FRAME_H
#define XML_MOVIE_FRAME_H

#include "xml/frame.h"

#ifdef interface
# undef interface
#endif

struct xml_movie_frame
{
	struct xml_frame frame;
};

extern const struct xml_vtable xml_movie_frame_vtable;

#endif
