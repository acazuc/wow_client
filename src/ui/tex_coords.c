#include "ui/tex_coords.h"

#include "xml/tex_coords.h"

#ifdef interface
# undef interface
#endif

static void load_xml(struct ui_tex_coords *tex_coords, const struct xml_tex_coords *xml);

void ui_tex_coords_init_xml(struct ui_tex_coords *tex_coords, const struct xml_tex_coords *xml)
{
	VEC2_SET(tex_coords->top_left, 0, 0);
	VEC2_SET(tex_coords->top_right, 1, 0);
	VEC2_SET(tex_coords->bottom_right, 1, 1);
	VEC2_SET(tex_coords->bottom_left, 0, 1);
	load_xml(tex_coords, xml);
}

void ui_tex_coords_init(struct ui_tex_coords *tex_coords, float left, float right, float top, float bottom)
{
	VEC2_SET(tex_coords->top_left, left, top);
	VEC2_SET(tex_coords->top_right, right, top);
	VEC2_SET(tex_coords->bottom_right, right, bottom);
	VEC2_SET(tex_coords->bottom_left, left, bottom);
}

void ui_tex_coords_init_vec(struct ui_tex_coords *tex_coords, struct vec2f top_left, struct vec2f top_right, struct vec2f bottom_right, struct vec2f bottom_left)
{
	VEC2_CPY(tex_coords->top_left, top_left);
	VEC2_CPY(tex_coords->top_right, top_right);
	VEC2_CPY(tex_coords->bottom_right, bottom_right);
	VEC2_CPY(tex_coords->bottom_left, bottom_left);
}

static void load_xml(struct ui_tex_coords *tex_coords, const struct xml_tex_coords *xml)
{
	if (OPTIONAL_ISSET(xml->left))
	{
		tex_coords->top_left.x = OPTIONAL_GET(xml->left);
		tex_coords->bottom_left.x = OPTIONAL_GET(xml->left);
	}
	if (OPTIONAL_ISSET(xml->right))
	{
		tex_coords->top_right.x = OPTIONAL_GET(xml->right);
		tex_coords->bottom_right.x = OPTIONAL_GET(xml->right);
	}
	if (OPTIONAL_ISSET(xml->top))
	{
		tex_coords->top_left.y = OPTIONAL_GET(xml->top);
		tex_coords->top_right.y = OPTIONAL_GET(xml->top);
	}
	if (OPTIONAL_ISSET(xml->bottom))
	{
		tex_coords->bottom_left.y = OPTIONAL_GET(xml->bottom);
		tex_coords->bottom_right.y = OPTIONAL_GET(xml->bottom);
	}
}
