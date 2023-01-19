#ifndef UI_GAME_TOOLTIP_H
#define UI_GAME_TOOLTIP_H

#include "ui/frame.h"

#ifdef interface
# undef interface
#endif

struct ui_game_tooltip
{
	struct ui_frame frame;
	struct ui_frame *owner;
};

extern const struct ui_object_vtable ui_game_tooltip_vtable;

void ui_game_tooltip_set_owner(struct ui_game_tooltip *game_tooltip, struct ui_frame *owner);

#endif
