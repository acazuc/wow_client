#ifndef XML_LAYOUT_FRAME_H
#define XML_LAYOUT_FRAME_H

#include "xml/dimension.h"
#include "xml/element.h"
#include "xml/anchors.h"

#ifdef interface
# undef interface
#endif

enum xml_layout_frame_mask
{
	XML_LAYOUT_FRAME_button                  = 0x000001,
	XML_LAYOUT_FRAME_check_button            = 0x000002,
	XML_LAYOUT_FRAME_color_select            = 0x000004,
	XML_LAYOUT_FRAME_cooldown                = 0x000008,
	XML_LAYOUT_FRAME_dress_up_model          = 0x000010,
	XML_LAYOUT_FRAME_edit_box                = 0x000020,
	XML_LAYOUT_FRAME_font_string             = 0x000040,
	XML_LAYOUT_FRAME_frame                   = 0x000080,
	XML_LAYOUT_FRAME_game_tooltip            = 0x000100,
	XML_LAYOUT_FRAME_message_frame           = 0x000200,
	XML_LAYOUT_FRAME_minimap                 = 0x000400,
	XML_LAYOUT_FRAME_model                   = 0x000800,
	XML_LAYOUT_FRAME_model_ffx               = 0x001000,
	XML_LAYOUT_FRAME_movie_frame             = 0x002000,
	XML_LAYOUT_FRAME_player_model            = 0x004000,
	XML_LAYOUT_FRAME_scroll_frame            = 0x008000,
	XML_LAYOUT_FRAME_scrolling_message_frame = 0x010000,
	XML_LAYOUT_FRAME_simple_html             = 0x020000,
	XML_LAYOUT_FRAME_slider                  = 0x040000,
	XML_LAYOUT_FRAME_status_bar              = 0x080000,
	XML_LAYOUT_FRAME_tabard_model            = 0x100000,
	XML_LAYOUT_FRAME_taxi_route_frame        = 0x200000,
	XML_LAYOUT_FRAME_texture                 = 0x400000,
};

struct xml_scrolling_message_frame;
struct xml_taxi_route_frame;
struct xml_dress_up_model;
struct xml_message_frame;
struct xml_check_button;
struct xml_game_tooltip;
struct xml_player_model;
struct xml_scroll_frame;
struct xml_tabard_model;
struct xml_color_select;
struct xml_font_string;
struct xml_movie_frame;
struct xml_simple_html;
struct xml_status_bar;
struct xml_model_ffx;
struct xml_cooldown;
struct xml_edit_box;
struct xml_minimap;
struct xml_texture;
struct xml_button;
struct xml_slider;
struct xml_frame;
struct xml_model;

struct xml_layout_frame
{
	struct xml_element element;
	struct jks_array inherits_vector; /* char* */
	uint32_t mask;
	struct optional_xml_anchors anchors;
	struct optional_xml_dimension size;
	char *name;
	char *inherits;
	struct optional_bool is_virtual;
	struct optional_bool set_all_points;
	struct optional_bool hidden;
};

OPTIONAL_DEF(optional_xml_layout_frame, struct xml_layout_frame);

void xml_layout_frame_resolve_inherits(struct xml_layout_frame *layout_frame);

#define ELEMENT_DECLARE_CAST(name) \
	const struct xml_##name *xml_clayout_frame_as_##name(const struct xml_layout_frame *layout_frame); \
	struct xml_##name *xml_layout_frame_as_##name(struct xml_layout_frame *layout_frame);

ELEMENT_DECLARE_CAST(button);
ELEMENT_DECLARE_CAST(check_button);
ELEMENT_DECLARE_CAST(color_select);
ELEMENT_DECLARE_CAST(cooldown);
ELEMENT_DECLARE_CAST(dress_up_model);
ELEMENT_DECLARE_CAST(edit_box);
ELEMENT_DECLARE_CAST(font_string);
ELEMENT_DECLARE_CAST(frame);
ELEMENT_DECLARE_CAST(game_tooltip);
ELEMENT_DECLARE_CAST(message_frame);
ELEMENT_DECLARE_CAST(minimap);
ELEMENT_DECLARE_CAST(model);
ELEMENT_DECLARE_CAST(model_ffx);
ELEMENT_DECLARE_CAST(movie_frame);
ELEMENT_DECLARE_CAST(player_model);
ELEMENT_DECLARE_CAST(scroll_frame);
ELEMENT_DECLARE_CAST(scrolling_message_frame);
ELEMENT_DECLARE_CAST(simple_html);
ELEMENT_DECLARE_CAST(slider);
ELEMENT_DECLARE_CAST(status_bar);
ELEMENT_DECLARE_CAST(tabard_model);
ELEMENT_DECLARE_CAST(taxi_route_frame);
ELEMENT_DECLARE_CAST(texture);

#undef ELEMENT_DECLARE_CAST

extern const struct xml_vtable xml_layout_frame_vtable;

#endif
