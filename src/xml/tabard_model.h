#ifndef XML_TABARD_MODEL_H
#define XML_TABARD_MODEL_H

#include "xml/player_model.h"

#ifdef interface
# undef interface
#endif

struct xml_tabard_model
{
	struct xml_player_model player_model;
};

extern const struct xml_vtable xml_tabard_model_vtable;

#endif
