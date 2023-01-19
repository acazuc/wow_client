#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "ui/frame.h"
#include "ui/color.h"

#ifdef interface
# undef interface
#endif

struct ui_font_string;
struct ui_texture;
struct ui_font;

struct ui_button
{
	struct ui_frame frame;
	struct ui_font_string *render_text;
	struct ui_texture *normal_texture;
	struct ui_texture *pushed_texture;
	struct ui_texture *disabled_texture;
	struct ui_texture *highlight_texture;
	struct optional_ui_color normal_color;
	struct optional_ui_color disabled_color;
	struct optional_ui_color highlight_color;
	struct ui_font *normal_font;
	struct ui_font *disabled_font;
	struct ui_font *highlight_font;
	char *text;
	bool disabled;
	bool highlight_locked;
};

extern const struct ui_object_vtable ui_button_vtable;

void ui_button_set_text(struct ui_button *button, const char *text);
void ui_button_set_disabled(struct ui_button *button, bool disabled);

#endif
