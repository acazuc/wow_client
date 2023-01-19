#include "value.h"

#include "xml/value.h"

#include "log.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void load_xml(struct ui_value *value, const struct xml_value *xml);

void ui_value_init_xml(struct ui_value *value, const struct xml_value *xml)
{
	value->type = VALUE_ABSOLUTE;
	load_xml(value, xml);
}

void ui_value_init(struct ui_value *value, int32_t val)
{
	value->type = VALUE_ABSOLUTE;
	value->abs = val;
}

static void load_xml(struct ui_value *value, const struct xml_value *xml)
{
	if (OPTIONAL_ISSET(xml->val))
		value->abs = OPTIONAL_GET(xml->val);
	else
		value->abs = 0;
	if (xml->value)
	{
		const struct xml_element *val = xml->value;
		if (!strcmp(val->vtable->name, "AbsValue"))
		{
			value->type = VALUE_ABSOLUTE;
			const struct xml_abs_value *abs_val = (const struct xml_abs_value*)val;
			if (OPTIONAL_ISSET(abs_val->val))
				value->abs = OPTIONAL_GET(abs_val->val);
		}
		else if (!strcmp(val->vtable->name, "RelValue"))
		{
			value->type = VALUE_RELATIVE;
			const struct xml_rel_value *rel_val = (const struct xml_rel_value*)val;
			if (OPTIONAL_ISSET(rel_val->val))
				value->rel = OPTIONAL_GET(rel_val->val);
			else
				value->rel = 1;
		}
		else
		{
			LOG_WARN("invalid value type: %s", val->vtable->name);
		}
	}
}
