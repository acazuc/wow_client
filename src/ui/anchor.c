#include "ui/anchor.h"
#include "ui/region.h"

#include "itf/interface.h"

#include "xml/anchors.h"

#include "memory.h"
#include "log.h"

#include <string.h>
#include <stdio.h>

#ifdef interface
# undef interface
#endif

static void load_xml(struct ui_anchor *anchor, const struct xml_anchor *xml);

struct ui_anchor *ui_anchor_new_xml(struct interface *interface, const struct xml_anchor *xml, struct ui_region *region)
{
	struct ui_anchor *anchor = mem_malloc(MEM_UI, sizeof(*anchor));
	if (!anchor)
	{
		LOG_ERROR("anchor malloc failed");
		return NULL;
	}
	anchor->interface = interface;
	anchor->point = ANCHOR_CENTER;
	anchor->relative_point = anchor->point;
	anchor->relative_to = NULL;
	anchor->region = region;
	ui_dimension_init(&anchor->offset, 0, 0);
	load_xml(anchor, xml);
	return anchor;
}

struct ui_anchor *ui_anchor_new(struct interface *interface, struct ui_dimension *offset, enum anchor_point point, enum anchor_point relative_point, const char *relative_to, struct ui_region *region)
{
	struct ui_anchor *anchor = mem_malloc(MEM_UI, sizeof(*anchor));
	if (!anchor)
	{
		LOG_ERROR("anchor malloc failed");
		return NULL;
	}
	anchor->interface = interface;
	anchor->point = point;
	anchor->relative_point = relative_point;
	anchor->relative_to = NULL;
	anchor->region = region;
	ui_dimension_init(&anchor->offset, offset ? offset->abs.x : 0, offset ? offset->abs.y : 0);
	if (relative_to)
	{
		anchor->real_relative_to = mem_strdup(MEM_UI, relative_to);
		if (!anchor->real_relative_to)
		{
			LOG_ERROR("failed to duplicate relative to");
			mem_free(MEM_UI, anchor);
			return NULL;
		}
	}
	else
	{
		anchor->real_relative_to = NULL;
	}
	ui_anchor_load_relative_to(anchor);
	return anchor;
}

struct ui_anchor *ui_anchor_dup(const struct ui_anchor *dup, struct ui_region *region)
{
	struct ui_anchor *anchor = mem_malloc(MEM_UI, sizeof(*anchor));
	if (!anchor)
	{
		LOG_ERROR("anchor malloc failed");
		return NULL;
	}
	anchor->interface = dup->interface;
	anchor->point = dup->point;
	anchor->relative_point = dup->relative_point;
	anchor->relative_to = NULL;
	anchor->region = region;
	ui_dimension_init(&anchor->offset, dup->offset.abs.x, dup->offset.abs.y);
	anchor->real_relative_to = mem_strdup(MEM_UI, dup->real_relative_to);
	if (!anchor->real_relative_to)
	{
		LOG_ERROR("failed to duplicate relative to");
		mem_free(MEM_UI, anchor);
		return NULL;
	}
	ui_anchor_load_relative_to(anchor);
	return anchor;
}

void ui_anchor_delete(struct ui_anchor *anchor)
{
	if (!anchor)
		return;
	if (anchor->relative_to)
		ui_region_remove_related_anchor(anchor->relative_to, anchor);
	mem_free(MEM_UI, anchor->real_relative_to);
	mem_free(MEM_UI, anchor);
}

static void load_xml(struct ui_anchor *anchor, const struct xml_anchor *xml)
{
	if (OPTIONAL_ISSET(xml->offset))
		ui_dimension_init_xml(&anchor->offset, &OPTIONAL_GET(xml->offset));
	if (xml->point)
	{
		if (!anchor_point_from_string(xml->point, &anchor->point))
			LOG_ERROR("invalid anchor point: %s", xml->point);
	}
	if (xml->relative_point)
	{
		if (!anchor_point_from_string(xml->relative_point, &anchor->relative_point))
			LOG_ERROR("invalid relative anchor point: %s", xml->relative_point);
	}
	else
	{
		anchor->relative_point = anchor->point;
	}
	if (xml->relative_to)
		anchor->real_relative_to = mem_strdup(MEM_UI, xml->relative_to);
	else
		anchor->real_relative_to = mem_strdup(MEM_UI, "$parent");
	if (!anchor->real_relative_to)
		LOG_ERROR("failed to duplicate real relative to");
}

void ui_anchor_load_relative_to(struct ui_anchor *anchor)
{
	if (anchor->relative_to)
		ui_region_remove_related_anchor(anchor->relative_to, anchor);
	if (!anchor->real_relative_to || !anchor->real_relative_to[0])
		return;
	if (!strcmp(anchor->real_relative_to, "$parent"))
	{
		if (!anchor->region->parent)
		{
			/* warn because maybe toplevel frame, in which case parent is root frame */
			LOG_WARN("Anchor relativeTo with $parent without parent (realRelativeTo: %s, region: %s)", anchor->real_relative_to,  ui_object_get_name((struct ui_object*)anchor->region));
			return;
		}
		anchor->relative_to = anchor->region->parent;
	}
	else
	{
		char relative_to[256];
		snprintf(relative_to, sizeof(relative_to), "%s", anchor->real_relative_to);
		if (!strncmp(relative_to, "$parent", 7))
		{
			if (!anchor->region->parent)
			{
				if (ui_object_get_name((struct ui_object*)anchor->region) && !strcmp(ui_object_get_name((struct ui_object*)anchor->region), "GlueParent"))
					LOG_WARN("Anchor relativeTo with $parent without parent (realRelativeTo: %s, region: %s)", anchor->real_relative_to, ui_object_get_name((struct ui_object*)anchor->region));
				return;
			}
			if (ui_object_get_name((struct ui_object*)anchor->region->parent))
			{
				char tmp[256];
				snprintf(tmp, sizeof(tmp), "%s%s", ui_object_get_name((struct ui_object*)anchor->region->parent), relative_to + 7);
				anchor->relative_to = interface_get_region(anchor->interface, tmp);
			}
			else
			{
				LOG_WARN("Anchor relativeTo with empty parent name (realRelativeTo: %s, region: %s)", anchor->real_relative_to, ui_object_get_name((struct ui_object*)anchor->region));
				anchor->relative_to = interface_get_region(anchor->interface, relative_to + 7);
			}
		}
		else
		{
			anchor->relative_to = interface_get_region(anchor->interface, relative_to);
		}
	}
	if (anchor->relative_to)
		ui_region_add_related_anchor(anchor->relative_to, anchor);
}

int32_t ui_anchor_get_x(const struct ui_anchor *anchor)
{
	switch (anchor->relative_point)
	{
		case ANCHOR_LEFT:
		case ANCHOR_TOPLEFT:
		case ANCHOR_BOTTOMLEFT:
			if (anchor->relative_to)
				return ui_region_get_left(anchor->relative_to) + anchor->offset.abs.x;
			if (anchor->region->parent)
				return ui_region_get_left(anchor->region->parent) + anchor->offset.abs.x;
			return anchor->offset.abs.x;
		case ANCHOR_RIGHT:
		case ANCHOR_TOPRIGHT:
		case ANCHOR_BOTTOMRIGHT:
			if (anchor->relative_to)
				return ui_region_get_right(anchor->relative_to) + anchor->offset.abs.x;
			if (anchor->region->parent)
				return ui_region_get_right(anchor->region->parent) + anchor->offset.abs.x;
			return anchor->interface->width + anchor->offset.abs.x;
		default:
			if (anchor->relative_to)
				return ui_region_get_left(anchor->relative_to) + ui_region_get_width(anchor->relative_to) / 2 + anchor->offset.abs.x;
			if (anchor->region->parent)
				return ui_region_get_left(anchor->region->parent) + ui_region_get_width(anchor->region->parent) / 2 + anchor->offset.abs.x;
			return anchor->interface->width / 2 + anchor->offset.abs.x;
	}
	return 0;
}

int32_t ui_anchor_get_y(const struct ui_anchor *anchor)
{
	switch (anchor->relative_point)
	{
		case ANCHOR_TOP:
		case ANCHOR_TOPLEFT:
		case ANCHOR_TOPRIGHT:
			if (anchor->relative_to)
				return ui_region_get_top(anchor->relative_to) - anchor->offset.abs.y;
			if (anchor->region->parent)
				return ui_region_get_top(anchor->region->parent) - anchor->offset.abs.y;
			return -anchor->offset.abs.y;
		case ANCHOR_BOTTOM:
		case ANCHOR_BOTTOMLEFT:
		case ANCHOR_BOTTOMRIGHT:
			if (anchor->relative_to)
				return ui_region_get_bottom(anchor->relative_to) - anchor->offset.abs.y;
			if (anchor->region->parent)
				return ui_region_get_bottom(anchor->region->parent) - anchor->offset.abs.y;
			return anchor->interface->height - anchor->offset.abs.y;
		default:
			if (anchor->relative_to)
				return ui_region_get_top(anchor->relative_to) + ui_region_get_height(anchor->relative_to) / 2 - anchor->offset.abs.y;
			if (anchor->region->parent)
				return ui_region_get_top(anchor->region->parent) + ui_region_get_height(anchor->region->parent) / 2 - anchor->offset.abs.y;
			return anchor->interface->height / 2 - anchor->offset.abs.y;
	}
	return 0;
}
