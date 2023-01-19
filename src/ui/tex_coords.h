#ifndef UI_TEX_COORDS_H
#define UI_TEX_COORDS_H

#include <jks/vec2.h>

#include <jks/optional.h>

#ifdef interface
# undef interface
#endif

struct xml_tex_coords;

struct ui_tex_coords
{
	struct vec2f top_left;
	struct vec2f top_right;
	struct vec2f bottom_right;
	struct vec2f bottom_left;
};

OPTIONAL_DEF(optional_ui_tex_coords, struct ui_tex_coords);

void ui_tex_coords_init_xml(struct ui_tex_coords *tex_coords, const struct xml_tex_coords *xml);
void ui_tex_coords_init_vec(struct ui_tex_coords *tex_coords, struct vec2f top_left, struct vec2f top_right, struct vec2f bottom_right, struct vec2f bottom_left);
void ui_tex_coords_init(struct ui_tex_coords *tex_coords, float left, float right, float top, float bottom);

#endif
