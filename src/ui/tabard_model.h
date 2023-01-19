#ifndef UI_TABARD_MODEL_H
#define UI_TABARD_MODEL_H

#include "ui/player_model.h"

#ifdef interface
# undef interface
#endif

struct ui_tabard_model
{
	struct ui_player_model player_model;
};

extern const struct ui_object_vtable ui_tabard_model_vtable;

#endif
