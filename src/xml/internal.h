#ifndef XML_INTERNAL_H
#define XML_INTERNAL_H

#include <jks/optional.h>

struct xml_attr
{
	const char *name;
	const char *value;
};

#define XML_ELEMENT_ATTRIBUTE_TEST_STRING(attribute, key, value) \
do \
{ \
	if (!strcmp((attribute)->name, key)) \
		return xml_parse_attribute_string(attribute, value); \
} while (0)

#define XML_ELEMENT_ATTRIBUTE_TEST_BOOLEAN(attribute, key, value) \
do \
{ \
	if (!strcmp((attribute)->name, key)) \
		return xml_parse_attribute_boolean(attribute, value); \
} while (0)

#define XML_ELEMENT_ATTRIBUTE_TEST_INTEGER(attribute, key, value) \
do \
{ \
	if (!strcmp((attribute)->name, key)) \
		return xml_parse_attribute_integer(attribute, value); \
} while (0)

#define XML_ELEMENT_ATTRIBUTE_TEST_FLOAT(attribute, key, value) \
do \
{ \
	if (!strcmp((attribute)->name, key)) \
		return xml_parse_attribute_float(attribute, value); \
} while (0)

#define XML_ELEMENT_CHILD_TEST(child, key, value, vt) \
do \
{ \
	if (!strcmp((child)->name, key)) \
	{ \
		if (OPTIONAL_ISSET(value)) \
			return XML_PARSE_CHILD_ALREADY; \
		OPTIONAL_SET(value); \
		((struct xml_element*)&OPTIONAL_GET(value))->vtable = (vt); \
		(vt)->ctr((struct xml_element*)&OPTIONAL_GET(value)); \
		xml_element_parse((struct xml_element*)&OPTIONAL_GET(value), child); \
		return XML_PARSE_CHILD_OK; \
	} \
} while (0)

#define XML_ELEMENT_DTR(opt) \
do \
{ \
	if (OPTIONAL_ISSET(opt)) \
		((struct xml_element*)&OPTIONAL_GET(opt))->vtable->dtr((struct xml_element*)&OPTIONAL_GET(opt)); \
} while (0)

enum xml_parse_attribute_status xml_parse_attribute_boolean(const struct xml_attr *attribute, struct optional_bool *value);
enum xml_parse_attribute_status xml_parse_attribute_integer(const struct xml_attr *attribute, struct optional_int32 *value);
enum xml_parse_attribute_status xml_parse_attribute_float(const struct xml_attr *attribute, struct optional_float *value);
enum xml_parse_attribute_status xml_parse_attribute_string(const struct xml_attr *attribute, char **value);
#endif
