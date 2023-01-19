#ifndef XML_CHECK_BUTTON_H
#define XML_CHECK_BUTTON_H

#include "xml/button.h"

#ifdef interface
# undef interface
#endif

struct xml_check_button
{
	struct xml_button button;
	struct optional_xml_texture checked_texture;
	struct optional_xml_texture disabled_checked_texture;
	struct optional_bool checked;
};

extern const struct xml_vtable xml_check_button_vtable;

#endif
