#ifndef XML_UI_H
#define XML_UI_H

#include "xml/element.h"

#include <jks/array.h>

#ifdef interface
# undef interface
#endif

typedef struct lua_State lua_State;
struct addon;

struct xml_ui
{
	struct xml_element element;
	struct jks_array layout_frames; /* struct xml_layout_frame* */
	struct jks_array fonts; /* struct xml_font* */
	struct addon *addon;
	char *filename;
};

struct xml_ui *xml_ui_new(struct addon *addon, const char *filename);

extern const struct xml_vtable xml_ui_vtable;

#endif
