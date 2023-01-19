#ifndef UI_CHECK_BUTTON_H
#define UI_CHECK_BUTTON_H

#include "ui/button.h"

#ifdef interface
# undef interface
#endif

struct ui_check_button
{
	struct ui_button button;
	struct ui_texture *disabled_checked_texture;
	struct ui_texture *checked_texture;
	bool checked;
};

extern const struct ui_object_vtable ui_check_button_vtable;

void ui_check_button_set_checked(struct ui_check_button *check_button, bool checked);

#endif
