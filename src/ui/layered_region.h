#ifndef UI_LAYERED_REGION_H
#define UI_LAYERED_REGION_H

#include "ui/region.h"
#include "ui/color.h"

#ifdef interface
# undef interface
#endif

struct ui_layered_region
{
	struct ui_region region;
	enum draw_layer draw_layer;
	struct ui_color vertex_color;
};

extern const struct ui_object_vtable ui_layered_region_vtable;

#endif
