#include "ui/shadow.h"

#include "xml/shadow.h"

#ifdef interface
# undef interface
#endif

static void load_xml(struct ui_shadow *shadow, const struct xml_shadow *xml);

void ui_shadow_init_xml(struct ui_shadow *shadow, const struct xml_shadow *xml)
{
	OPTIONAL_UNSET(shadow->color);
	OPTIONAL_UNSET(shadow->offset);
	load_xml(shadow, xml);
}

void ui_shadow_init(struct ui_shadow *shadow, const struct ui_color *color, const struct ui_dimension *offset)
{
	if (color)
		OPTIONAL_CTR(shadow->color, *color);
	else
		OPTIONAL_UNSET(shadow->color);
	if (offset)
		OPTIONAL_CTR(shadow->offset, *offset);
	else
		OPTIONAL_UNSET(shadow->offset);
}

static void load_xml(struct ui_shadow *shadow, const struct xml_shadow *xml)
{
	if (OPTIONAL_ISSET(xml->offset))
	{
		OPTIONAL_SET(shadow->offset);
		ui_dimension_init_xml(&OPTIONAL_GET(shadow->offset), &OPTIONAL_GET(xml->offset));
	}
	if (OPTIONAL_ISSET(xml->color))
	{
		OPTIONAL_SET(shadow->color);
		ui_color_init_xml(&OPTIONAL_GET(shadow->color), &OPTIONAL_GET(xml->color));
	}
}

bool ui_shadow_eq(const struct ui_shadow *s1, const struct ui_shadow *s2)
{
	if (OPTIONAL_ISSET(s1->color))
	{
		if (!OPTIONAL_ISSET(s2->color))
			return false;
		if (!ui_color_eq(&OPTIONAL_GET(s1->color), &OPTIONAL_GET(s2->color)))
			return false;
	}
	else if (OPTIONAL_ISSET(s2->color))
	{
		return false;
	}
	if (OPTIONAL_ISSET(s1->offset))
	{
		if (!OPTIONAL_ISSET(s2->offset))
			return false;
		if (!ui_dimension_eq(&OPTIONAL_GET(s1->offset), &OPTIONAL_GET(s2->offset)))
			return false;
	}
	else if (OPTIONAL_ISSET(s2->offset))
	{
		return false;
	}
	return true;
}
