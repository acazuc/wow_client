#ifndef UI_ANCHOR_H
#define UI_ANCHOR_H

#include "itf/enum.h"

#include "ui/dimension.h"

#ifdef interface
# undef interface
#endif

struct xml_anchor;
struct interface;
struct ui_region;

struct ui_anchor
{
	struct interface *interface;
	struct ui_dimension offset;
	enum anchor_point point;
	enum anchor_point relative_point;
	struct ui_region *relative_to;
	struct ui_region *region;
	char *real_relative_to;
};

struct ui_anchor *ui_anchor_new_xml(struct interface *interface, const struct xml_anchor *anchor, struct ui_region *region);
struct ui_anchor *ui_anchor_new(struct interface *interface, struct ui_dimension *dimension, enum anchor_point point, enum anchor_point relative_point, const char *relative_to, struct ui_region *region);
struct ui_anchor *ui_anchor_dup(const struct ui_anchor *anchor, struct ui_region *region);
void ui_anchor_delete(struct ui_anchor *anchor);
void ui_anchor_load_relative_to(struct ui_anchor *anchor);
int32_t ui_anchor_get_x(const struct ui_anchor *anchor);
int32_t ui_anchor_get_y(const struct ui_anchor *anchor);

#endif
