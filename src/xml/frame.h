#ifndef XML_FRAME_H
#define XML_FRAME_H

#include "xml/resize_bounds.h"
#include "xml/layout_frame.h"
#include "xml/attributes.h"
#include "xml/backdrop.h"
#include "xml/scripts.h"
#include "xml/layers.h"
#include "xml/frames.h"
#include "xml/inset.h"

#ifdef interface
# undef interface
#endif

struct xml_frame
{
	struct xml_layout_frame layout_frame;
	struct optional_xml_resize_bounds resize_bounds;
	struct optional_xml_attributes attributes;
	struct optional_xml_backdrop backdrop;
	struct optional_xml_scripts scripts;
	struct optional_xml_layers layers;
	struct optional_xml_frames frames;
	struct optional_xml_inset hit_rect_insets;
	struct optional_float alpha;
	char *parent;
	struct optional_bool top_level;
	struct optional_bool movable;
	struct optional_bool resizable;
	char *frame_strata;
	struct optional_int32 frame_level;
	struct optional_int32 id;
	struct optional_bool enable_mouse;
	struct optional_bool enable_keyboard;
	struct optional_bool clamped_to_screen;
	struct optional_bool is_protected;
	struct optional_xml_layout_frame title_region;
};

extern const struct xml_vtable xml_frame_vtable;

#endif
