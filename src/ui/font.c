#include "ui/font.h"

#include "itf/interface.h"

#include "font/font.h"

#include "xml/font.h"

#include "memory.h"
#include "cache.h"
#include "log.h"
#include "wow.h"

#include <string.h>
#include <stdio.h>

#ifdef interface
# undef interface
#endif

MEMORY_DECL(UI);

#define UI_OBJECT (&font->object)
#define UI_FONT_INSTANCE (&font->font_instance)

static void on_font_height_changed(struct ui_object *object);
static void on_color_changed(struct ui_object *object);
static void on_shadow_changed(struct ui_object *object);
static void on_spacing_changed(struct ui_object *object);
static void on_outline_changed(struct ui_object *object);
static void on_monochrome_changed(struct ui_object *object);
static void on_justify_h_changed(struct ui_object *object);
static void on_justify_v_changed(struct ui_object *object);

static const struct ui_font_instance_callbacks g_font_font_instance_callbacks =
{
	.on_font_height_changed = on_font_height_changed,
	.on_color_changed = on_color_changed,
	.on_shadow_changed = on_shadow_changed,
	.on_spacing_changed = on_spacing_changed,
	.on_outline_changed = on_outline_changed,
	.on_monochrome_changed = on_monochrome_changed,
	.on_justify_h_changed = on_justify_h_changed,
	.on_justify_v_changed = on_justify_v_changed,
};

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_object_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_font *font = (struct ui_font*)object;
	UI_OBJECT->mask |= UI_OBJECT_font;
	font->name = name ? mem_strdup(MEM_UI, name) : NULL;
	if (font->name)
		interface_register_font(UI_OBJECT->interface, font->name, font);
	ui_font_instance_init(interface, UI_FONT_INSTANCE, object, &g_font_font_instance_callbacks);
	jks_array_init(&font->childs, sizeof(struct ui_font_instance*), NULL, &jks_array_memory_fn_UI);
	return true;
}

static void dtr(struct ui_object *object)
{
	struct ui_font *font = (struct ui_font*)object;
	if (font->name)
		interface_unregister_font(UI_OBJECT->interface, font->name);
	mem_free(MEM_UI, font->name);
	if (UI_FONT_INSTANCE->font)
		cache_unref_by_key_font(g_wow->cache, UI_FONT_INSTANCE->font);
	ui_font_instance_destroy(UI_FONT_INSTANCE);
	for (size_t i = 0; i < font->childs.size; ++i)
	{
		struct ui_font_instance *child = *JKS_ARRAY_GET(&font->childs, i, struct ui_font_instance*);
		child->font_instance = NULL;
	}
	jks_array_destroy(&font->childs);
	ui_object_vtable.dtr(object);
}

static void load_xml(struct ui_object *object, const struct xml_layout_frame *layout_frame)
{
	struct ui_font *font = (struct ui_font*)object;
	const struct xml_font *xml_font = (const struct xml_font*)layout_frame;
	if (xml_font->inherits)
	{
		struct ui_font *parent = interface_get_font(UI_OBJECT->interface, xml_font->inherits);
		if (parent)
		{
			if (UI_FONT_INSTANCE->font_instance)
				ui_font_remove_child(UI_FONT_INSTANCE->font_instance, UI_FONT_INSTANCE);
			if (ui_font_add_child(parent, UI_FONT_INSTANCE))
				UI_FONT_INSTANCE->font_instance = parent;
			else
				UI_FONT_INSTANCE->font_instance = NULL;
		}
		else
		{
			LOG_WARN("undefined font: %s", xml_font->inherits);
		}
	}
	if (OPTIONAL_ISSET(xml_font->font_height))
	{
		OPTIONAL_SET(UI_FONT_INSTANCE->font_height);
		ui_value_init_xml(&OPTIONAL_GET(UI_FONT_INSTANCE->font_height), &OPTIONAL_GET(xml_font->font_height));
	}
	if (OPTIONAL_ISSET(xml_font->color))
	{
		OPTIONAL_SET(UI_FONT_INSTANCE->color);
		ui_color_init_xml(&OPTIONAL_GET(UI_FONT_INSTANCE->color), &OPTIONAL_GET(xml_font->color));
	}
	if (xml_font->font)
	{
		char tmp[256];
		snprintf(tmp, sizeof(tmp), "%s", xml_font->font);
		normalize_mpq_filename(tmp, sizeof(tmp));
		UI_FONT_INSTANCE->font = mem_strdup(MEM_UI, tmp);
	}
	if (OPTIONAL_ISSET(xml_font->spacing))
		OPTIONAL_CTR(UI_FONT_INSTANCE->spacing, OPTIONAL_GET(xml_font->spacing));
	if (xml_font->outline)
	{
		enum outline_type outline;
		if (outline_from_string(xml_font->outline, &outline))
			OPTIONAL_CTR(UI_FONT_INSTANCE->outline, outline);
		else
			LOG_ERROR("invalid outline: %s", xml_font->outline);
	}
	if (OPTIONAL_ISSET(xml_font->monochrome))
		OPTIONAL_CTR(UI_FONT_INSTANCE->monochrome, OPTIONAL_GET(xml_font->monochrome));
	if (xml_font->justify_v)
	{
		enum justify_v_type type;
		if (justify_v_from_string(xml_font->justify_v, &type))
			OPTIONAL_CTR(UI_FONT_INSTANCE->justify_v, type);
	}
	if (xml_font->justify_h)
	{
		enum justify_h_type type;
		if (justify_h_from_string(xml_font->justify_h, &type))
			OPTIONAL_CTR(UI_FONT_INSTANCE->justify_h, type);
	}
	if (xml_font->font)
	{
		const struct ui_value *font_height = ui_font_instance_get_font_height(UI_FONT_INSTANCE);
		UI_FONT_INSTANCE->render_font = interface_ref_render_font(UI_OBJECT->interface, UI_FONT_INSTANCE->font, font_height ? font_height->abs : 0);
	}
	if (OPTIONAL_ISSET(xml_font->shadow))
	{
		OPTIONAL_SET(UI_FONT_INSTANCE->shadow);
		ui_shadow_init_xml(&OPTIONAL_GET(UI_FONT_INSTANCE->shadow), &OPTIONAL_GET(xml_font->shadow));
	}
}

static struct ui_font_instance *as_font_instance(struct ui_object *object)
{
	struct ui_font *font = (struct ui_font*)object;
	return UI_FONT_INSTANCE;
}

bool ui_font_add_child(struct ui_font *font, struct ui_font_instance *child)
{
	return jks_array_push_back(&font->childs, &child);
}

void ui_font_remove_child(struct ui_font *font, struct ui_font_instance *child)
{
	for (size_t i = 0; i < font->childs.size; ++i)
	{
		if (child == *JKS_ARRAY_GET(&font->childs, i, struct ui_font_instance*))
		{
			jks_array_erase(&font->childs, i);
			return;
		}
	}
	LOG_WARN("tried to remove non-child font_instance from font");
}

static void on_font_height_changed(struct ui_object *object)
{
	struct ui_font *font = (struct ui_font*)object;
	for (size_t i = 0; i < font->childs.size; ++i)
	{
		struct ui_font_instance *child = *JKS_ARRAY_GET(&font->childs, i, struct ui_font_instance*);
		if (!OPTIONAL_ISSET(child->font_height))
			child->callbacks->on_font_height_changed(child->object);
	}
}

static void on_color_changed(struct ui_object *object)
{
	struct ui_font *font = (struct ui_font*)object;
	for (size_t i = 0; i < font->childs.size; ++i)
	{
		struct ui_font_instance *child = *JKS_ARRAY_GET(&font->childs, i, struct ui_font_instance*);
		if (!OPTIONAL_ISSET(child->color))
			child->callbacks->on_color_changed(child->object);
	}
}

static void on_shadow_changed(struct ui_object *object)
{
	struct ui_font *font = (struct ui_font*)object;
	for (size_t i = 0; i < font->childs.size; ++i)
	{
		struct ui_font_instance *child = *JKS_ARRAY_GET(&font->childs, i, struct ui_font_instance*);
		if (!OPTIONAL_ISSET(child->shadow))
			child->callbacks->on_shadow_changed(child->object);
	}
}

static void on_spacing_changed(struct ui_object *object)
{
	struct ui_font *font = (struct ui_font*)object;
	for (size_t i = 0; i < font->childs.size; ++i)
	{
		struct ui_font_instance *child = *JKS_ARRAY_GET(&font->childs, i, struct ui_font_instance*);
		if (!OPTIONAL_ISSET(child->spacing))
			child->callbacks->on_spacing_changed(child->object);
	}
}

static void on_outline_changed(struct ui_object *object)
{
	struct ui_font *font = (struct ui_font*)object;
	for (size_t i = 0; i < font->childs.size; ++i)
	{
		struct ui_font_instance *child = *JKS_ARRAY_GET(&font->childs, i, struct ui_font_instance*);
		if (!OPTIONAL_ISSET(child->outline))
			child->callbacks->on_outline_changed(child->object);
	}
}

static void on_monochrome_changed(struct ui_object *object)
{
	struct ui_font *font = (struct ui_font*)object;
	for (size_t i = 0; i < font->childs.size; ++i)
	{
		struct ui_font_instance *child = *JKS_ARRAY_GET(&font->childs, i, struct ui_font_instance*);
		if (!OPTIONAL_ISSET(child->monochrome))
			child->callbacks->on_monochrome_changed(child->object);
	}
}

static void on_justify_h_changed(struct ui_object *object)
{
	struct ui_font *font = (struct ui_font*)object;
	for (size_t i = 0; i < font->childs.size; ++i)
	{
		struct ui_font_instance *child = *JKS_ARRAY_GET(&font->childs, i, struct ui_font_instance*);
		if (!OPTIONAL_ISSET(child->justify_h))
			child->callbacks->on_justify_h_changed(child->object);
	}
}

static void on_justify_v_changed(struct ui_object *object)
{
	struct ui_font *font = (struct ui_font*)object;
	for (size_t i = 0; i < font->childs.size; ++i)
	{
		struct ui_font_instance *child = *JKS_ARRAY_GET(&font->childs, i, struct ui_font_instance*);
		if (!OPTIONAL_ISSET(child->justify_v))
			child->callbacks->on_justify_v_changed(child->object);
	}
}

static bool register_methods(struct jks_array *methods)
{
	/*
	   CopyFontObject
	 */
	if (!ui_font_instance_register_methods(methods)) /* FontInstance only fill methods */
		return false;
	return ui_object_vtable.register_methods(methods);
}

UI_INH0(object, void, post_load);
UI_INH0(object, void, register_in_interface);
UI_INH0(object, void, unregister_in_interface);
UI_INH0(object, void, eval_name);
UI_INH0(object, void, render);
UI_INH1(object, void, on_click, enum gfx_mouse_button, button);
UI_INH0(object, float, get_alpha);
UI_INH1(object, void, set_alpha, float, alpha);
UI_INH1(object, void, set_hidden, bool, hidden);
UI_INH2(object, void, get_size, int32_t*, x, int32_t*, y);
UI_INH0(object, void, set_dirty_coords);
UI_INH1(object, void, on_mouse_move, gfx_pointer_event_t*, event);
UI_INH1(object, void, on_mouse_down, gfx_mouse_event_t*, event);
UI_INH1(object, void, on_mouse_up, gfx_mouse_event_t*, event);
UI_INH1(object, void, on_mouse_scroll, gfx_scroll_event_t*, event);
UI_INH1(object, bool, on_key_down, gfx_key_event_t*, event);
UI_INH1(object, bool, on_key_up, gfx_key_event_t*, event);
UI_INH0(object, const char*, get_name);

const struct ui_object_vtable ui_font_vtable =
{
	UI_OBJECT_VTABLE("Font")
};
