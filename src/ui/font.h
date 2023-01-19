#ifndef UI_FONT_H
#define UI_FONT_H

#include "ui/font_instance.h"
#include "ui/object.h"

#ifdef interface
# undef interface
#endif

struct xml_font;

struct ui_font
{
	struct ui_object object;
	struct ui_font_instance font_instance;
	char *name;
	struct jks_array childs; /* struct font_instance* */
};

extern const struct ui_object_vtable ui_font_vtable;

bool ui_font_add_child(struct ui_font *font, struct ui_font_instance *child);
void ui_font_remove_child(struct ui_font *font, struct ui_font_instance *child);

#endif
