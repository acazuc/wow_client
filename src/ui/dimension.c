#include "ui/dimension.h"

#include "xml/dimension.h"

#include "log.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void load_xml(struct ui_dimension *dimension, const struct xml_dimension *xml);

void ui_dimension_init_xml(struct ui_dimension *dimension, const struct xml_dimension *xml)
{
	dimension->type = DIMENSION_ABSOLUTE;
	load_xml(dimension, xml);
}

void ui_dimension_init(struct ui_dimension *dimension, int32_t x, int32_t y)
{
	dimension->type = DIMENSION_ABSOLUTE;
	dimension->abs.x = x;
	dimension->abs.y = y;
}

static void load_xml(struct ui_dimension *dimension, const struct xml_dimension *xml)
{
	if (OPTIONAL_ISSET(xml->x))
		dimension->abs.x = OPTIONAL_GET(xml->x);
	else
		dimension->abs.x = 0;
	if (OPTIONAL_ISSET(xml->y))
		dimension->abs.y = OPTIONAL_GET(xml->y);
	else
		dimension->abs.y = 0;
	if (xml->dimension)
	{
		const struct xml_element *dim = xml->dimension;
		if (!strcmp(dim->vtable->name, "AbsDimension"))
		{
			dimension->type = DIMENSION_ABSOLUTE;
			const struct xml_abs_dimension *abs_dim = (const struct xml_abs_dimension*)dim;
			if (OPTIONAL_ISSET(abs_dim->x))
				dimension->abs.x = OPTIONAL_GET(abs_dim->x);
			if (OPTIONAL_ISSET(abs_dim->y))
				dimension->abs.y = OPTIONAL_GET(abs_dim->y);
		}
		else if (!strcmp(dim->vtable->name, "RelDimension"))
		{
			dimension->type = DIMENSION_RELATIVE;
			const struct xml_rel_dimension *rel_dim = (const struct xml_rel_dimension*)dim;
			if (OPTIONAL_ISSET(rel_dim->x))
				dimension->rel.x = OPTIONAL_GET(rel_dim->x);
			else
				dimension->rel.x = 1;
			if (OPTIONAL_ISSET(rel_dim->y))
				dimension->rel.y = OPTIONAL_GET(rel_dim->y);
			else
				dimension->rel.y = 1;
		}
		else
		{
			LOG_WARN("invalid xml type: %s", dim->vtable->name);
		}
	}
}
