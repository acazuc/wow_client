#ifndef UI_DRESS_UP_MODEL_H
#define UI_DRESS_UP_MODEL_H

#include "ui/player_model.h"

#ifdef interface
# undef interface
#endif

struct ui_dress_up_model
{
	struct ui_player_model player_model;
};

extern const struct ui_object_vtable ui_dress_up_model_vtable;

#endif
