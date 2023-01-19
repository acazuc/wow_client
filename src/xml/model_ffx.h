#ifndef XML_MODEL_FFX_H
#define XML_MODEL_FFX_H

#include "xml/model.h"

#ifdef interface
# undef interface
#endif

struct xml_model_ffx
{
	struct xml_model model;
};

extern const struct xml_vtable xml_model_ffx_vtable;

#endif
