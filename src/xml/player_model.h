#ifndef XML_PLAYER_MODEL_H
#define XML_PLAYER_MODEL_H

#include "xml/model.h"

#ifdef interface
# undef interface
#endif

struct xml_player_model
{
	struct xml_model model;
};

extern const struct xml_vtable xml_player_model_vtable;

#endif
