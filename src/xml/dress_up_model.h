#ifndef XML_DRESS_UP_MODEL_H
#define XML_DRESS_UP_MODEL_H

#include "xml/player_model.h"

#ifdef interface
# undef interface
#endif

struct xml_dress_up_model
{
	struct xml_player_model player_model;
};

extern const struct xml_vtable xml_dress_up_model_vtable;

#endif
