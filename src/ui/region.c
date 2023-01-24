#include "ui/region.h"
#include "ui/frame.h"

#include "itf/interface.h"

#include "xml/layout_frame.h"

#include "wow_lua.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <jks/vec2.h>

#include <string.h>

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_REGION() LUA_METHOD(Region, region)

#define UI_OBJECT (&region->object)

MEMORY_DECL(UI);

static void calc_coords(struct ui_region *region);
static void register_lua_global(struct ui_region *region);
static void unregister_lua_global(struct ui_region *region);

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_object_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_region *region = (struct ui_region*)object;
	UI_OBJECT->mask |= UI_OBJECT_region;
	region->parent = parent;
	region->hidden = false;
	region->hovered = false;
	region->clicked = false;
	region->mouse_enabled = false;
	region->keyboard_enabled = false;
	region->dirty_coords = true;
	ui_inset_init(&region->hit_rect_insets, 0, 0, 0, 0);
	OPTIONAL_UNSET(region->size);
	if (name)
	{
		region->real_name = mem_strdup(MEM_UI, name);
		if (!region->real_name)
			LOG_ERROR("failed to duplicate real name");
	}
	else
	{
		region->real_name = NULL;
	}
	region->name = NULL;
	jks_array_init(&region->related_anchors, sizeof(struct ui_anchor*), NULL, &jks_array_memory_fn_UI);
	jks_array_init(&region->anchors, sizeof(struct ui_anchor*), NULL, &jks_array_memory_fn_UI);
	ui_region_calc_effective_alpha(region);
	return true;
}

static void dtr(struct ui_object *object)
{
	struct ui_region *region = (struct ui_region*)object;
	if (region->parent && (UI_OBJECT->mask & UI_OBJECT_frame))
		ui_frame_remove_child(ui_object_as_frame((struct ui_object*)region->parent), ui_object_as_frame(UI_OBJECT));
	ui_object_unregister_in_interface(UI_OBJECT);
	unregister_lua_global(region);
	ui_region_clear_anchors(region);
	for (size_t i = 0; i < region->related_anchors.size; ++i)
	{
		struct ui_anchor *anchor = *JKS_ARRAY_GET(&region->related_anchors, i, struct ui_anchor*);
		anchor->relative_to = NULL;
	}
	jks_array_destroy(&region->related_anchors);
	jks_array_destroy(&region->anchors);
	mem_free(MEM_UI, region->real_name);
	mem_free(MEM_UI, region->name);
	ui_object_vtable.dtr(object);
}

static void register_lua_global(struct ui_region *region)
{
	if (!region->name)
		return;
	struct lua_State *L = ui_object_get_L((struct ui_object*)region);
	ui_push_lua_object(L, UI_OBJECT);
	lua_setglobal(L, region->name);
	/* LOG_INFO("registering global %s of type %s", region->name, region->vtable->name); */
}

static void unregister_lua_global(struct ui_region *region)
{
	if (!region->name)
		return;
	struct lua_State *L = ui_object_get_L((struct ui_object*)region);
	lua_pushnil(L);
	lua_setglobal(L, region->name);
	/* LOG_DEBUG("unregistering global %s", region->name); */
}

static void load_xml(struct ui_object *object, const struct xml_layout_frame *layout_frame)
{
	struct ui_region *region = (struct ui_region*)object;
	xml_layout_frame_resolve_inherits((struct xml_layout_frame*)layout_frame);
	for (size_t i = 0; i < layout_frame->inherits_vector.size; ++i)
	{
		const char *inherit = *JKS_ARRAY_GET(&layout_frame->inherits_vector, i, const char*);
		struct xml_layout_frame *inh = interface_get_virtual_layout_frame(UI_OBJECT->interface, inherit);
		if (inh)
		{
			ui_object_load_xml(UI_OBJECT, inh);
		}
		else
		{
			if (!interface_get_font(UI_OBJECT->interface, inherit))
				LOG_WARN("undefined inherit: %s", inherit);
		}
	}
	/*if (!layout_frame->getName().empty())
		region->realName = layout_frame->getName();*/
	if (OPTIONAL_ISSET(layout_frame->anchors))
	{
		for (size_t i = 0; i < OPTIONAL_GET(layout_frame->anchors).anchors.size; ++i)
		{
			ui_region_add_anchor(region, ui_anchor_new_xml(UI_OBJECT->interface, *JKS_ARRAY_GET(&OPTIONAL_GET(layout_frame->anchors).anchors, i, struct xml_anchor*), region));
		}
	}
	if (OPTIONAL_ISSET(layout_frame->size))
	{
		OPTIONAL_SET(region->size);
		ui_dimension_init_xml(&OPTIONAL_GET(region->size), &OPTIONAL_GET(layout_frame->size));
	}
	if (OPTIONAL_ISSET(layout_frame->hidden))
		region->hidden = OPTIONAL_GET(layout_frame->hidden);
	if (OPTIONAL_ISSET(layout_frame->set_all_points) && OPTIONAL_GET(layout_frame->set_all_points))
	{
		if (!region->parent && (!region->name || strcmp(region->name, "GlueParent")))
			LOG_WARN("setallpoints without parent (region: %s)", region->name);
		ui_region_add_anchor(region, ui_anchor_new(UI_OBJECT->interface, NULL, ANCHOR_TOPLEFT, ANCHOR_TOPLEFT, "$parent", region));
		ui_region_add_anchor(region, ui_anchor_new(UI_OBJECT->interface, NULL, ANCHOR_BOTTOMRIGHT, ANCHOR_BOTTOMRIGHT, "$parent", region));
	}
}

static void post_load(struct ui_object *object)
{
	struct ui_region *region = (struct ui_region*)object;
	register_lua_global(region);
	ui_object_register_in_interface(UI_OBJECT);
	for (size_t i = 0; i < region->anchors.size; ++i)
	{
		struct ui_anchor *anchor = *JKS_ARRAY_GET(&region->anchors, i, struct ui_anchor*);
		ui_anchor_load_relative_to(anchor);
	}
}

void ui_region_print_debug(struct ui_region *region)
{
	LOG_INFO("###################################### %s ####################################", region->name);
	if (region->parent)
	{
		LOG_INFO("parent name: %s", ui_object_get_name((struct ui_object*)region->parent));
		LOG_INFO("parent left: %d", ui_region_get_left(region->parent));
		LOG_INFO("parent right: %d", ui_region_get_right(region->parent));
		LOG_INFO("parent top: %d", ui_region_get_top(region->parent));
		LOG_INFO("parent bottom: %d", ui_region_get_bottom(region->parent));
	}
	LOG_INFO("left: %d", ui_region_get_left(region));
	LOG_INFO("right: %d", ui_region_get_right(region));
	LOG_INFO("top: %d", ui_region_get_top(region));
	LOG_INFO("bottom: %d", ui_region_get_bottom(region));
	LOG_INFO("size: %s", OPTIONAL_ISSET(region->size) ? "true" : "false");
	const struct ui_anchor *left_anchor = ui_region_get_left_anchor(region);
	const struct ui_anchor *right_anchor = ui_region_get_right_anchor(region);
	const struct ui_anchor *top_anchor = ui_region_get_top_anchor(region);
	const struct ui_anchor *bottom_anchor = ui_region_get_bottom_anchor(region);
	if (OPTIONAL_ISSET(region->size))
	{
		LOG_INFO("width: %d", OPTIONAL_GET(region->size).abs.x);
		LOG_INFO("height: %d", OPTIONAL_GET(region->size).abs.y);
	}
	LOG_INFO("left anchor: %s", left_anchor ? "true" : "false");
	if (left_anchor)
	{
		LOG_INFO("left anchor x: %d", ui_anchor_get_x(left_anchor));
		LOG_INFO("left anchor abs x: %d", left_anchor->offset.abs.x);
		LOG_INFO("left anchor relative: %s", ui_object_get_name((struct ui_object*)left_anchor->relative_to));
	}
	LOG_INFO("top anchor: %s", top_anchor ? "true" : "false");
	if (top_anchor)
	{
		LOG_INFO("top anchor y: %d", ui_anchor_get_y(top_anchor));
		LOG_INFO("top anchor abs x: %d", top_anchor->offset.abs.y);
		LOG_INFO("top anchor relative: %s", ui_object_get_name((struct ui_object*)top_anchor->relative_to));
	}
	LOG_INFO("right anchor: %s", right_anchor ? "true" : "false");
	if (right_anchor)
	{
		LOG_INFO("right anchor x: %d", ui_anchor_get_x(right_anchor));
		LOG_INFO("right anchor abs x: %d", right_anchor->offset.abs.x);
		LOG_INFO("right anchor relative: %s", ui_object_get_name((struct ui_object*)right_anchor->relative_to));
	}
	LOG_INFO("bottom anchor: %s", bottom_anchor ? "true" : "false");
	if (bottom_anchor)
	{
		LOG_INFO("bottom anchor y: %d", ui_anchor_get_y(bottom_anchor));
		LOG_INFO("bottom anchor abs x: %d", bottom_anchor->offset.abs.y);
		LOG_INFO("bottom anchor relative: %s", ui_object_get_name((struct ui_object*)bottom_anchor->relative_to));
	}
	LOG_INFO("##########################################################################");
}

static void render(struct ui_object *object)
{
	struct ui_region *region = (struct ui_region*)object;
	const char *display_names[] =
	{
		//"CharacterCreateClassText",
		//"OptionsFrameCheckButton19",
		//"OptionsFrameCheckButton6",
		//"OptionsFrameMiscellaneous",
		//"QuestLogMicroButton",
		//"CharacterCreateFactionLabel",
		//"CharacterCreateRaceLabel",
		//"CharacterCreateRaceText",
		//"CharacterCreateRaceAbilityText",
		//"CharacterCreateClassLabel",
		//"CharacterCreateFactionScrollFrame",
		//"CharacterCreateRaceScrollFrame",
		//"CharacterCreateClassScrollFrame",
		//"CharacterCreateRaceScrollFrameScrollBarScrollDownButton",
		//"CharacterCreateRaceScrollFrameScrollBarScrollUpButton",
		//"CharacterCreateRaceScrollFrameScrollBar",
		//"PetTrainingPointLabel",
		//"BrowseButton1",
		//"WorldMapZoomOutButton",
	};
	if (region->name) for (size_t i = 0; i < sizeof(display_names) / sizeof(*display_names); ++i) if (!strcmp(region->name, display_names[i]))
		ui_region_print_debug(region);
}

void ui_region_add_related_anchor(struct ui_region *region, struct ui_anchor *anchor)
{
	if (!jks_array_push_back(&region->related_anchors, &anchor))
		LOG_ERROR("failed to add new related anchor");
}

void ui_region_remove_related_anchor(struct ui_region *region, struct ui_anchor *anchor)
{
	for (size_t i = 0; i < region->related_anchors.size; ++i)
	{
		struct ui_anchor *a = *JKS_ARRAY_GET(&region->related_anchors, i, struct ui_anchor*);
		if (a == anchor)
		{
			jks_array_erase(&region->related_anchors, i);
			return;
		}
	}
}

static void register_in_interface(struct ui_object *object)
{
	struct ui_region *region = (struct ui_region*)object;
	if (region->name)
		interface_register_region(UI_OBJECT->interface, region->name, region);
}

static void unregister_in_interface(struct ui_object *object)
{
	struct ui_region *region = (struct ui_region*)object;
	if (region->name)
		interface_unregister_region(UI_OBJECT->interface, region->name);
}

static void eval_name(struct ui_object *object)
{
	struct ui_region *region = (struct ui_region*)object;
	if (!region->real_name)
		return;
	if (strncmp(region->real_name, "$parent", 7))
	{
		region->name = mem_strdup(MEM_UI, region->real_name);
		if (!region->name)
			LOG_ERROR("failed to duplicate name");
		return;
	}
	struct ui_region *parent = region->parent;
	do
	{
		if (!parent->name)
		{
			LOG_WARN("empty parent name, using parent parents");
			parent = parent->parent;
			continue;
		}
		wow_asprintf(MEM_UI, &region->name, "%s%s", parent->name, region->real_name + 7);
		break;
	} while (parent);
}

static void on_mouse_move(struct ui_object *object, gfx_pointer_event_t *event)
{
	struct ui_region *region = (struct ui_region*)object;
	if (!region->mouse_enabled)
		return;
	if (event->used)
	{
		region->hovered = false;
		return;
	}
	int32_t left = ui_region_get_left(region);
	if (event->x < left + region->hit_rect_insets.abs.left)
	{
		region->hovered = false;
		return;
	}
	int32_t right = ui_region_get_right(region);
	if (event->x > right + region->hit_rect_insets.abs.right)
	{
		region->hovered = false;
		return;
	}
	int32_t top = ui_region_get_top(region);
	if (event->y < top + region->hit_rect_insets.abs.top)
	{
		region->hovered = false;
		return;
	}
	int32_t bottom = ui_region_get_bottom(region);
	if (event->y > bottom + region->hit_rect_insets.abs.bottom)
	{
		region->hovered = false;
		return;
	}
	/*LOG_INFO("mouse hovered " << region->name << ". left: " << getLeft() << ", right: " << getRight() << ", top: " << getTop() << ", bottom: " << getBottom());
	if (region->parent)
		LOG_INFO("parent: " << region->parent->getName() << ". left: " << region->parent->getLeft() << ", right: " << region->parent->getRight() << ", top: " << region->parent->getTop() << ", bottom: " << region->parent->getBottom());*/
	region->hovered = true;
	event->used = true;
}

static void on_mouse_down(struct ui_object *object, gfx_mouse_event_t *event)
{
	struct ui_region *region = (struct ui_region*)object;
	if (!region->mouse_enabled)
		return;
	if (event->used)
		return;
	if (region->hovered)
	{
		region->clicked = true;
		event->used = true;
		return;
	}
}

static void on_mouse_up(struct ui_object *object, gfx_mouse_event_t *event)
{
	struct ui_region *region = (struct ui_region*)object;
	if (!region->mouse_enabled)
		return;
	if (event->used)
	{
		region->clicked = false;
		return;
	}
	if (region->hovered)
	{
		region->clicked = false;
		event->used = true;
		return;
	}
	region->clicked = false;
}

static void on_mouse_scroll(struct ui_object *object, gfx_scroll_event_t *event)
{
	struct ui_region *region = (struct ui_region*)object;
	if (!region->mouse_enabled)
		return;
	if (event->used)
		return;
	if (region->hovered)
		event->used = true;
}

static bool on_key_down(struct ui_object *object, gfx_key_event_t *event)
{
	(void)object;
	(void)event;
	return false;
}

static bool on_key_up(struct ui_object *object, gfx_key_event_t *event)
{
	(void)object;
	(void)event;
	return false;
}

void ui_region_set_size(struct ui_region *region, int32_t width, int32_t height)
{
	OPTIONAL_SET(region->size);
	ui_dimension_init(&OPTIONAL_GET(region->size), width, height);
	ui_object_set_dirty_coords(UI_OBJECT);
}

static void set_hidden(struct ui_object *object, bool hidden)
{
	struct ui_region *region = (struct ui_region*)object;
	region->hidden = hidden;
	if (hidden)
	{
		region->hovered = false;
		region->clicked = false;
	}
}

static void get_size(struct ui_object *object, int32_t *x, int32_t *y)
{
	struct ui_region *region = (struct ui_region*)object;
	if (x)
	{
		if (OPTIONAL_ISSET(region->size) && OPTIONAL_GET(region->size).abs.x)
			*x = OPTIONAL_GET(region->size).abs.x;
		else if (region->parent)
			*x = ui_region_get_width(region->parent);
		else
			*x = UI_OBJECT->interface->width;
	}
	if (y)
	{
		if (OPTIONAL_ISSET(region->size) && OPTIONAL_GET(region->size).abs.y)
			*y = OPTIONAL_GET(region->size).abs.y;
		else if (region->parent)
			*y = ui_region_get_height(region->parent);
		else
			*y = UI_OBJECT->interface->height;
	}
}

static void calc_coords(struct ui_region *region)
{
	const struct ui_anchor *left_anchor = ui_region_get_left_anchor(region);
	const struct ui_anchor *right_anchor = ui_region_get_right_anchor(region);
	const struct ui_anchor *top_anchor = ui_region_get_top_anchor(region);
	const struct ui_anchor *bottom_anchor = ui_region_get_bottom_anchor(region);
	const struct ui_anchor *center_anchor = ui_region_get_center_anchor(region);
	vec2i_t size;
	ui_object_get_size(UI_OBJECT, &size.x, &size.y);
	/* Left */
	if (left_anchor)
		region->left = ui_anchor_get_x(left_anchor);
	else if (right_anchor)
		region->left = ui_anchor_get_x(right_anchor) - size.x;
	else if (center_anchor)
		region->left = ui_anchor_get_x(center_anchor) - size.x / 2;
	else if (top_anchor)
		region->left = ui_anchor_get_x(top_anchor) - size.x / 2;
	else if (bottom_anchor)
		region->left = ui_anchor_get_x(bottom_anchor) - size.x / 2;
	else if (region->parent)
		region->left = ui_region_get_left(region->parent) + (ui_region_get_width(region->parent) - size.x) / 2;
	else
		region->left = 0;
	/* Right */
	if (right_anchor)
		region->right = ui_anchor_get_x(right_anchor);
	else if (left_anchor)
		region->right = ui_anchor_get_x(left_anchor) + size.x;
	else if (center_anchor)
		region->right = ui_anchor_get_x(center_anchor) + size.x / 2;
	else if (top_anchor)
		region->right = ui_anchor_get_x(top_anchor) + size.x / 2;
	else if (bottom_anchor)
		region->right = ui_anchor_get_x(bottom_anchor) + size.x / 2;
	else if (region->parent)
		region->right = ui_region_get_left(region->parent) + (ui_region_get_width(region->parent) + size.x) / 2;
	else
		region->right = size.x;
	/* Top */
	if (top_anchor)
		region->top = ui_anchor_get_y(top_anchor);
	else if (bottom_anchor)
		region->top = ui_anchor_get_y(bottom_anchor) - size.y;
	else if (center_anchor)
		region->top = ui_anchor_get_y(center_anchor) - size.y / 2;
	else if (left_anchor)
		region->top = ui_anchor_get_y(left_anchor) - size.y / 2;
	else if (right_anchor)
		region->top = ui_anchor_get_y(right_anchor) - size.y / 2;
	else if (region->parent)
		region->top = ui_region_get_top(region->parent) + (ui_region_get_height(region->parent) - size.y) / 2;
	else
		region->top = 0;
	/* Bottom */
	if (bottom_anchor)
		region->bottom = ui_anchor_get_y(bottom_anchor);
	else if (top_anchor)
		region->bottom = ui_anchor_get_y(top_anchor) + size.y;
	else if (center_anchor)
		region->bottom = ui_anchor_get_y(center_anchor) + size.y / 2;
	else if (left_anchor)
		region->bottom = ui_anchor_get_y(left_anchor) + size.y / 2;
	else if (right_anchor)
		region->bottom = ui_anchor_get_y(right_anchor) + size.y / 2;
	else if (region->parent)
		region->bottom = ui_region_get_top(region->parent) + (ui_region_get_height(region->parent) + size.y) / 2;
	else
		region->bottom = size.y;
	region->dirty_coords = false;
}

int32_t ui_region_get_left(struct ui_region *region)
{
	if (region->dirty_coords)
		calc_coords(region);
	return region->left;
}

int32_t ui_region_get_right(struct ui_region *region)
{
	if (region->dirty_coords)
		calc_coords(region);
	return region->right;
}

int32_t ui_region_get_top(struct ui_region *region)
{
	if (region->dirty_coords)
		calc_coords(region);
	return region->top;
}

int32_t ui_region_get_bottom(struct ui_region *region)
{
	if (region->dirty_coords)
		calc_coords(region);
	return region->bottom;
}

int32_t ui_region_get_width(struct ui_region *region)
{
	if (region->dirty_coords)
		calc_coords(region);
	return region->right - region->left;
}

int32_t ui_region_get_height(struct ui_region *region)
{
	if (region->dirty_coords)
		calc_coords(region);
	return region->bottom - region->top;
}

static void set_dirty_coords(struct ui_object *object)
{
	struct ui_region *region = (struct ui_region*)object;
	if (region->dirty_coords) /* maybe not a good solution, but it shouldn't cause too much problem, and it avoids freezes sometimes */
		return;
	region->dirty_coords = true;
	for (size_t i = 0; i < region->related_anchors.size; ++i)
	{
		struct ui_anchor *anchor = *JKS_ARRAY_GET(&region->related_anchors, i, struct ui_anchor*);
		ui_object_set_dirty_coords((struct ui_object*)anchor->region);
	}
}

void ui_region_add_anchor(struct ui_region *region, struct ui_anchor *anchor)
{
	if (!anchor)
	{
		LOG_ERROR("adding NULL anchor");
		return;
	}
	for (size_t i = 0; i < region->anchors.size; ++i)
	{
		struct ui_anchor **a = JKS_ARRAY_GET(&region->anchors, i, struct ui_anchor*);
		if ((*a)->point == anchor->point)
		{
			ui_anchor_delete(*a);
			*a = anchor;
			ui_object_set_dirty_coords(UI_OBJECT);
			return;
		}
	}
	if (!jks_array_push_back(&region->anchors, &anchor))
		LOG_ERROR("failed to push anchor");
	ui_object_set_dirty_coords(UI_OBJECT);
}

void ui_region_clear_anchors(struct ui_region *region)
{
	for (size_t i = 0; i < region->anchors.size; ++i)
	{
		struct ui_anchor *anchor = *JKS_ARRAY_GET(&region->anchors, i, struct ui_anchor*);
		ui_anchor_delete(anchor);
	}
	jks_array_resize(&region->anchors, 0);
	ui_object_set_dirty_coords(UI_OBJECT);
}

const struct ui_anchor *ui_region_get_center_anchor(const struct ui_region *region)
{
	for (size_t i = 0; i < region->anchors.size; ++i)
	{
		struct ui_anchor *anchor = *JKS_ARRAY_GET(&region->anchors, i, struct ui_anchor*);
		if (anchor->point == ANCHOR_CENTER)
			return anchor;
	}
	return NULL;
}

const struct ui_anchor *ui_region_get_left_anchor(const struct ui_region *region)
{
	for (size_t i = 0; i < region->anchors.size; ++i)
	{
		struct ui_anchor *anchor = *JKS_ARRAY_GET(&region->anchors, i, struct ui_anchor*);
		switch (anchor->point)
		{
			case ANCHOR_LEFT:
			case ANCHOR_TOPLEFT:
			case ANCHOR_BOTTOMLEFT:
				return anchor;
			default:
				break;
		}
	}
	return NULL;
}

const struct ui_anchor *ui_region_get_right_anchor(const struct ui_region *region)
{
	for (size_t i = 0; i < region->anchors.size; ++i)
	{
		struct ui_anchor *anchor = *JKS_ARRAY_GET(&region->anchors, i, struct ui_anchor*);
		switch (anchor->point)
		{
			case ANCHOR_RIGHT:
			case ANCHOR_TOPRIGHT:
			case ANCHOR_BOTTOMRIGHT:
				return anchor;
			default:
				break;
		}
	}
	return NULL;
}

const struct ui_anchor *ui_region_get_top_anchor(const struct ui_region *region)
{
	for (size_t i = 0; i < region->anchors.size; ++i)
	{
		struct ui_anchor *anchor = *JKS_ARRAY_GET(&region->anchors, i, struct ui_anchor*);
		switch (anchor->point)
		{
			case ANCHOR_TOP:
			case ANCHOR_TOPLEFT:
			case ANCHOR_TOPRIGHT:
				return anchor;
			default:
				break;
		}
	}
	return NULL;
}

const struct ui_anchor *ui_region_get_bottom_anchor(const struct ui_region *region)
{
	for (size_t i = 0; i < region->anchors.size; ++i)
	{
		struct ui_anchor *anchor = *JKS_ARRAY_GET(&region->anchors, i, struct ui_anchor*);
		switch (anchor->point)
		{
			case ANCHOR_BOTTOM:
			case ANCHOR_BOTTOMLEFT:
			case ANCHOR_BOTTOMRIGHT:
				return anchor;
			default:
				break;
		}
	}
	return NULL;
}

bool ui_region_is_visible(struct ui_region *region)
{
	if (region->hidden)
		return false;
	if (!region->parent)
		return true;
	return ui_region_is_visible(region->parent);
}

void ui_region_calc_effective_alpha(struct ui_region *region)
{
	if (region->parent)
		region->effective_alpha = ui_object_get_alpha((struct ui_object*)region->parent) * UI_OBJECT->alpha;
	else
		region->effective_alpha = UI_OBJECT->alpha;
}

static void on_click(struct ui_object *object, enum gfx_mouse_button button)
{
	ui_object_vtable.on_click(object, button);
}

static float get_alpha(struct ui_object *object)
{
	struct ui_region *region = (struct ui_region*)object;
	return region->effective_alpha;
}

static void set_alpha(struct ui_object *object, float alpha)
{
	struct ui_region *region = (struct ui_region*)object;
	ui_object_vtable.set_alpha(object, alpha);
	ui_region_calc_effective_alpha(region);
}

static const char *get_name(struct ui_object *object)
{
	struct ui_region *region = (struct ui_region*)object;
	return region->name;
}

static int lua_Hide(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc != 1)
		return luaL_error(L, "Usage: Region:Hide()");
	ui_object_set_hidden(UI_OBJECT, true);
	return 0;
}

static int lua_Show(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc != 1)
		return luaL_error(L, "Usage: Region:Show()");
	ui_object_set_hidden(UI_OBJECT, false);
	return 0;
}

static int lua_IsShown(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc != 1)
		return luaL_error(L, "Usage: Region:IsShown()");
	lua_pushboolean(L, !region->hidden);
	return 1;
}

static int lua_IsVisible(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc != 1)
		return luaL_error(L, "Usage: Region:IsVisible()");
	lua_pushboolean(L, ui_region_is_visible(region));
	return 1;
}

static int lua_GetCenter(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc != 1)
		return luaL_error(L, "Usage: Region:GetCenter()");
	int x = (ui_region_get_left(region) + ui_region_get_right(region)) / 2;
	int y = UI_OBJECT->interface->height - (ui_region_get_top(region) + ui_region_get_bottom(region)) / 2;
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	return 2;
}

static int lua_GetLeft(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc != 1)
		return luaL_error(L, "Usage: Region:GetLeft()");
	lua_pushnumber(L, ui_region_get_left(region));
	return 1;
}

static int lua_GetRight(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc != 1)
		return luaL_error(L, "Usage: Region:GetRight()");
	lua_pushnumber(L, ui_region_get_right(region));
	return 1;
}

static int lua_GetTop(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc != 1)
		return luaL_error(L, "Usage: Region:GetTop()");
	lua_pushnumber(L, UI_OBJECT->interface->height - ui_region_get_top(region));
	return 1;
}

static int lua_GetBottom(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc != 1)
		return luaL_error(L, "Usage: Region:GetBottom()");
	lua_pushnumber(L, UI_OBJECT->interface->height - ui_region_get_bottom(region));
	return 1;
}

static int lua_GetWidth(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc != 1)
		return luaL_error(L, "Usage: Region:GetWidth()");
	lua_pushnumber(L, ui_region_get_width(region));
	return 1;
}

static int lua_SetWidth(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc != 2)
		return luaL_error(L, "Usage: Region:SetWidth(width)");
	if (!lua_isnumber(L, 2))
		return luaL_argerror(L, 2, "number expected");
	if (!OPTIONAL_ISSET(region->size))
	{
		OPTIONAL_SET(region->size);
		ui_dimension_init(&OPTIONAL_GET(region->size), 0, 0);
	}
	OPTIONAL_GET(region->size).abs.x = lua_tonumber(L, 2);
	ui_object_set_dirty_coords(UI_OBJECT);
	return 0;
}

static int lua_GetHeight(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc != 1)
		return luaL_error(L, "Usage: Region:GetHeight()");
	lua_pushnumber(L, ui_region_get_height(region));
	return 1;
}

static int lua_SetHeight(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc != 2)
		return luaL_error(L, "Usage: Region:SetHeight(height)");
	if (!lua_isnumber(L, 2))
		return luaL_argerror(L, 2, "number expected");
	if (!OPTIONAL_ISSET(region->size))
	{
		OPTIONAL_SET(region->size);
		ui_dimension_init(&OPTIONAL_GET(region->size), 0, 0);
	}
	OPTIONAL_GET(region->size).abs.y = lua_tonumber(L, 2);
	ui_object_set_dirty_coords(UI_OBJECT);
	return 0;
}

static int lua_GetRect(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc != 1)
		return luaL_error(L, "Usage: Region:GetRect()");
	lua_pushnumber(L, ui_region_get_left(region));
	lua_pushnumber(L, UI_OBJECT->interface->height - ui_region_get_bottom(region));
	lua_pushnumber(L, ui_region_get_width(region));
	lua_pushnumber(L, ui_region_get_height(region));
	return 4;
}

static int lua_GetNumPoints(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc != 1)
		return luaL_error(L, "Usage: Region:GetNumPoints()");
	lua_pushinteger(L, region->anchors.size);
	return 1;
}

static int lua_GetPoint(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc > 2)
		return luaL_error(L, "Usage: Region:GetPoint([point])");
	int point;
	if (argc == 2)
	{
		if (!lua_isinteger(L, 2))
			return luaL_argerror(L, 2, "integer expected");
		point = lua_tointeger(L, 2);
		if (point <= 0)
			return 0;
	}
	else
	{
		point = 1;
	}
	if ((size_t)point > region->anchors.size)
		return 0;
	struct ui_anchor *anchor = *JKS_ARRAY_GET(&region->anchors, point - 1, struct ui_anchor*);
	lua_pushstring(L, anchor_point_to_string(anchor->point));
	if (anchor->relative_to)
		ui_push_lua_object(L, (struct ui_object*)anchor->relative_to);
	else
		lua_pushnil(L);
	lua_pushstring(L, anchor_point_to_string(anchor->relative_point));
	lua_pushnumber(L, anchor->offset.abs.x);
	lua_pushnumber(L, anchor->offset.abs.y);
	return 5;
}

static int lua_SetPoint(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc < 2 || argc == 3 || argc > 6)
		return luaL_error(L, "Usage: Region:SetPoint(point)");
	if (!lua_isstring(L, 2))
		return luaL_argerror(L, 2, "string expected");
	const char *point_str = lua_tostring(L, 2);
	if (!point_str)
		return 0;
	enum anchor_point point;
	if (!anchor_point_from_string(point_str, &point))
	{
		char tmp[256];
		snprintf(tmp, sizeof(tmp), "invalid anchor point: %s", point_str);
		return luaL_argerror(L, 2, tmp);
	}
	enum anchor_point rel_point;
	struct ui_dimension offset;
	ui_dimension_init(&offset, 0, 0);
	const char *relative_name;
	if (argc == 2)
	{
		relative_name = "$parent";
		rel_point = point;
	}
	else if (argc == 4 && lua_isnumber(L, 3))
	{
		if (!lua_isnumber(L, 4))
			return luaL_argerror(L, 4, "number expected");
		offset.abs.x = lua_tonumber(L, 3);
		offset.abs.y = lua_tonumber(L, 4);
		relative_name = "$parent";
		rel_point = point;
	}
	else if (argc == 4 || argc == 5 || argc == 6)
	{
		if (lua_isstring(L, 3))
		{
			const char *rel = lua_tostring(L, 3);
			if (!rel)
				return 0;
			relative_name = rel;
		}
		else if (lua_istable(L, 3))
		{
			struct ui_region *relative = ui_get_lua_region(L, 3);
			if (!relative)
				return luaL_argerror(L, 3, "invalid table");
			relative_name = relative->name;
		}
		else if (lua_isnil(L, 3))
		{
			relative_name = NULL;
		}
		else
		{
			char tmp[256];
			snprintf(tmp, sizeof(tmp), "integer, string, table or nil expected, got %s", lua_typename(L, lua_type(L, 3)));
			return luaL_argerror(L, 3, tmp);
		}
		if (argc == 4 || argc == 6)
		{
			const char *rel_point_str = lua_tostring(L, 4);
			if (!rel_point_str)
				return 0;
			if (!anchor_point_from_string(rel_point_str, &rel_point))
			{
				char tmp[256];
				snprintf(tmp, sizeof(tmp), "invalid anchor point: %s", rel_point_str);
				return luaL_argerror(L, 4, tmp);
			}
		}
		else
		{
			rel_point = point;
		}
		if (argc == 5)
		{
			if (!lua_isnumber(L, 4))
				return luaL_argerror(L, 4, "number expected");
			if (!lua_isnumber(L, 5))
				return luaL_argerror(L, 5, "number expected");
			offset.abs.x = lua_tonumber(L, 4);
			offset.abs.y = lua_tonumber(L, 5);
		}
		else if (argc == 6)
		{
			if (!lua_isnumber(L, 5))
				return luaL_argerror(L, 5, "number expected");
			if (!lua_isnumber(L, 6))
				return luaL_argerror(L, 6, "number expected");
			offset.abs.x = lua_tonumber(L, 5);
			offset.abs.y = lua_tonumber(L, 6);
		}
		else
		{
			offset.abs.y = 0;
			offset.abs.y = 0;
		}
	}
	else
	{
		return luaL_argerror(L, 1, "invalid number of arguments");
	}
	ui_region_add_anchor(region, ui_anchor_new(UI_OBJECT->interface, &offset, point, rel_point, relative_name, region));
	return 0;
}

static int lua_ClearAllPoints(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc != 1)
		return luaL_error(L, "Usage: Region:ClearAllPoints()");
	ui_region_clear_anchors(region);
	return 0;
}

static int lua_SetAllPoints(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc < 1 || argc > 2)
		return luaL_error(L, "Usage: Region:SetAllPoints()");
	if (argc == 2)
	{
		struct ui_region *parent;
		switch (lua_type(L, 2))
		{
			case LUA_TSTRING:
			{
				const char *str = lua_tostring(L, 2);
				if (!str)
					return 0;
				parent = interface_get_region(UI_OBJECT->interface, str);
				break;
			}
			case LUA_TTABLE:
				parent = ui_get_lua_region(L, 2);
				break;
			case LUA_TNIL:
				LOG_WARN("unsupported nil parameter");
				return 0;
			default:
				return luaL_argerror(L, 2, "string or table expected");
		}
		if (!parent)
		{
			LOG_ERROR("unknown parent");
			return 0;
		}
		ui_region_add_anchor(region, ui_anchor_new(UI_OBJECT->interface, NULL, ANCHOR_TOPLEFT, ANCHOR_TOPLEFT, "$parent", region));
		ui_region_add_anchor(region, ui_anchor_new(UI_OBJECT->interface, NULL, ANCHOR_BOTTOMRIGHT, ANCHOR_BOTTOMRIGHT, "$parent", region));
	}
	return 0;
}

static int lua_GetParent(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc != 1)
		return luaL_error(L, "Usage: Region:GetParent()");
	if (!region->parent)
	{
		lua_pushnil(L);
		return 1;
	}
	ui_push_lua_object(L, (struct ui_object*)region->parent);
	return 1;
}

static int lua_SetParent(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc != 2)
		return luaL_error(L, "Usage: Region:SetParent(object)");
	LUA_UNIMPLEMENTED_METHOD();
	/* TODO set strata to parent strata */
	return 0;
}

static int lua_IsProtected(lua_State *L)
{
	LUA_METHOD_REGION();
	if (argc != 1)
		return luaL_error(L, "Usage: Region:IsProtected()");
	LUA_UNIMPLEMENTED_METHOD();
	lua_pushboolean(L, true);
	return 1;
}

static bool register_methods(struct jks_array *methods)
{
	/*
	 */
	UI_REGISTER_METHOD(Hide);
	UI_REGISTER_METHOD(Show);
	UI_REGISTER_METHOD(IsShown);
	UI_REGISTER_METHOD(IsVisible);
	UI_REGISTER_METHOD(GetCenter);
	UI_REGISTER_METHOD(GetLeft);
	UI_REGISTER_METHOD(GetRight);
	UI_REGISTER_METHOD(GetTop);
	UI_REGISTER_METHOD(GetBottom);
	UI_REGISTER_METHOD(GetWidth);
	UI_REGISTER_METHOD(SetWidth);
	UI_REGISTER_METHOD(GetHeight);
	UI_REGISTER_METHOD(SetHeight);
	UI_REGISTER_METHOD(GetRect);
	UI_REGISTER_METHOD(GetNumPoints);
	UI_REGISTER_METHOD(GetPoint);
	UI_REGISTER_METHOD(SetPoint);
	UI_REGISTER_METHOD(ClearAllPoints);
	UI_REGISTER_METHOD(SetAllPoints);
	UI_REGISTER_METHOD(GetParent);
	UI_REGISTER_METHOD(SetParent);
	UI_REGISTER_METHOD(IsProtected);
	return ui_object_vtable.register_methods(methods);
}

UI_INH0(object, struct ui_font_instance*, as_font_instance);

const struct ui_object_vtable ui_region_vtable =
{
	UI_OBJECT_VTABLE("Region")
};
