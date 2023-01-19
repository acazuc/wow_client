#include "color.h"

#include "xml/color.h"

#ifdef interface
# undef interface
#endif

static void load_xml(struct ui_color *color, const struct xml_color *xml);

void ui_color_init_xml(struct ui_color *color, const struct xml_color *xml)
{
	color->r = 1;
	color->g = 1;
	color->b = 1;
	color->a = 1;
	load_xml(color, xml);
}

void ui_color_init(struct ui_color *color, float r, float g, float b, float a)
{
	color->r = r;
	color->g = g;
	color->b = b;
	color->a = a;
}

static void load_xml(struct ui_color *color, const struct xml_color *xml)
{
	if (OPTIONAL_ISSET(xml->r))
		color->r = OPTIONAL_GET(xml->r);
	if (OPTIONAL_ISSET(xml->g))
		color->g = OPTIONAL_GET(xml->g);
	if (OPTIONAL_ISSET(xml->b))
		color->b = OPTIONAL_GET(xml->b);
	if (OPTIONAL_ISSET(xml->a))
		color->a = OPTIONAL_GET(xml->a);
}

bool ui_color_eq(const struct ui_color *c1, const struct ui_color *c2)
{
	return c1->r == c2->r && c1->g == c2->g && c1->b == c2->b && c1->a == c2->a;
}
