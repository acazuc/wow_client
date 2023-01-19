#ifndef UI_MODEL_FFX_H
#define UI_MODEL_FFX_H

#include "ui/model.h"

#ifdef interface
# undef interface
#endif

struct ui_model_ffx
{
	struct ui_model model;
};

extern const struct ui_object_vtable ui_model_ffx_vtable;

#endif
