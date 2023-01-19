#ifndef XML_ELEMENTS_H
#define XML_ELEMENTS_H

#include <jks/optional.h>

#ifdef interface
# undef interface
#endif

struct interface;
struct ui_region;
struct ui_object;
struct xml_node;
struct xml_attr;
typedef struct _xmlNode xmlNode;

struct xml_layout_frame;
struct xml_vtable;

enum xml_node_type
{
	XML_NODE_ELEMENT,
	XML_NODE_TEXT,
	XML_NODE_COMMENT,
};

enum xml_parse_attribute_status
{
	XML_PARSE_ATTRIBUTE_OK,
	XML_PARSE_ATTRIBUTE_INVALID_NAME,
	XML_PARSE_ATTRIBUTE_INVALID_VALUE,
};

enum xml_parse_child_status
{
	XML_PARSE_CHILD_OK,
	XML_PARSE_CHILD_INVALID_NAME,
	XML_PARSE_CHILD_INVALID_TYPE,
	XML_PARSE_CHILD_ALREADY,
	XML_PARSE_CHILD_INTERNAL,
};

struct xml_node
{
	enum xml_node_type type;
	const char *name;
	const char *value;
	const xmlNode *node;
};

struct xml_element
{
	const struct xml_vtable *vtable;
};

struct xml_vtable
{
	const char *name;
	void (*ctr)(struct xml_element *element);
	void (*dtr)(struct xml_element *element);
	enum xml_parse_attribute_status (*parse_attribute)(struct xml_element *element, const struct xml_attr *attribute);
	enum xml_parse_child_status (*parse_child)(struct xml_element *element, const struct xml_node *child);
	struct ui_region *(*load_interface)(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent);
};

extern const struct xml_vtable xml_element_vtable;

void xml_element_parse(struct xml_element *element, const struct xml_node *node);
struct xml_layout_frame *xml_create_layout_frame(const char *name);
void xml_node_parse(struct xml_node *node, const xmlNode *n);
void xml_element_delete(struct xml_element *element);
struct ui_region *xml_load_interface(const struct xml_layout_frame *layout_frame, struct interface *interface, struct ui_region *parent);

#endif
