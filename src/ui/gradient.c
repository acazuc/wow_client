#include "ui/gradient.h"

#include "xml/gradient.h"

#include "log.h"

#ifdef interface
# undef interface
#endif

static void load_xml(struct ui_gradient *gradient, const struct xml_gradient *xml);

void ui_gradient_init_xml(struct ui_gradient *gradient, const struct xml_gradient *xml)
{
	ui_color_init(&gradient->min_color, 1, 1, 1, 1);
	ui_color_init(&gradient->max_color, 1, 1, 1, 1);
	gradient->orientation = ORIENTATION_VERTICAL;
	load_xml(gradient, xml);
}

void ui_gradient_init(struct ui_gradient *gradient)
{
	ui_color_init(&gradient->min_color, 1, 1, 1, 1);
	ui_color_init(&gradient->max_color, 1, 1, 1, 1);
	gradient->orientation = ORIENTATION_VERTICAL;
}

static void load_xml(struct ui_gradient *gradient, const struct xml_gradient *xml)
{
	if (OPTIONAL_ISSET(xml->min_color))
		ui_color_init_xml(&gradient->min_color, &OPTIONAL_GET(xml->min_color));
	if (OPTIONAL_ISSET(xml->max_color))
		ui_color_init_xml(&gradient->max_color, &OPTIONAL_GET(xml->max_color));
	if (xml->orientation)
	{
		if (!orientation_from_string(xml->orientation, &gradient->orientation))
			LOG_ERROR("invalid orientation");
	}
}
