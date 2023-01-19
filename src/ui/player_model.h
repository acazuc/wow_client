#ifndef PLAYER_MODEL_H
#define PLAYER_MODEL_H

#include "ui/model.h"

#ifdef interface
# undef interface
#endif

struct ui_player_model
{
	struct ui_model model;
};

extern const struct ui_object_vtable ui_player_model_vtable;

#endif
