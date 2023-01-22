#include "simple_html.h"

#include "itf/interface.h"

#include "xml/simple_html.h"

#include "ui/font_string.h"

#include "wow_lua.h"
#include "memory.h"
#include "cache.h"
#include "log.h"
#include "wow.h"

#include <libwow/mpq.h>

#include <string.h>

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_SIMPLE_HTML() LUA_METHOD(SimpleHTML, simple_html)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_FRAME->region)
#define UI_FRAME (&simple_html->frame)

static void on_font_height_changed(struct ui_object *object);
static void on_color_changed(struct ui_object *object);
static void on_shadow_color_changed(struct ui_object *object);
static void on_spacing_changed(struct ui_object *object);
static void on_outline_changed(struct ui_object *object);
static void on_monochrome_changed(struct ui_object *object);
static void on_justify_h_changed(struct ui_object *object);
static void on_justify_v_changed(struct ui_object *object);

static const struct ui_font_instance_callbacks g_simple_html_font_instance_callbacks =
{
	.on_font_height_changed = on_font_height_changed,
	.on_color_changed = on_color_changed,
	.on_shadow_color_changed = on_shadow_color_changed,
	.on_spacing_changed = on_spacing_changed,
	.on_outline_changed = on_outline_changed,
	.on_monochrome_changed = on_monochrome_changed,
	.on_justify_h_changed = on_justify_h_changed,
	.on_justify_v_changed = on_justify_v_changed,
};

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_frame_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_simple_html *simple_html = (struct ui_simple_html*)object;
	UI_OBJECT->mask |= UI_OBJECT_simple_html;
	simple_html->paragraph_font_string = NULL;
	simple_html->header1_font_string = NULL;
	simple_html->header2_font_string = NULL;
	simple_html->header3_font_string = NULL;
	simple_html->hyperlink_format = mem_strdup(MEM_UI, "|H%s|h%s|h");
	simple_html->font_string = NULL;
	simple_html->text = NULL;
	if (!simple_html->hyperlink_format)
		LOG_ERROR("failed to duplicate hyperlink format");
	ui_font_instance_init(interface, &simple_html->font_instance, object, &g_simple_html_font_instance_callbacks);
	return true;
}

static void dtr(struct ui_object *object)
{
	struct ui_simple_html *simple_html = (struct ui_simple_html*)object;
	ui_object_delete((struct ui_object*)simple_html->font_string);
	mem_free(MEM_UI, simple_html->hyperlink_format);
	mem_free(MEM_UI, simple_html->text);
	ui_font_instance_destroy(&simple_html->font_instance);
	ui_frame_vtable.dtr(object);
}

static void load_xml(struct ui_object *object, const struct xml_layout_frame *layout_frame)
{
	ui_frame_vtable.load_xml(object, layout_frame);
	struct ui_simple_html *simple_html = (struct ui_simple_html*)object;
	const struct xml_simple_html *xml_simple_html = xml_clayout_frame_as_simple_html(layout_frame);
	if (xml_simple_html)
	{
		if (xml_simple_html->font)
			simple_html->font = interface_get_font(UI_OBJECT->interface, xml_simple_html->font);
		if (xml_simple_html->file)
		{
			char filename[256];
			snprintf(filename, sizeof(filename), "Data\\%s\\%s", g_wow->locale, xml_simple_html->file);
			normalize_mpq_filename(filename, sizeof(filename));
			struct wow_mpq_file *file;
			if (cache_ref_by_key_mpq(g_wow->cache, filename, &file))
			{
				simple_html->text = mem_malloc(MEM_UI, file->size + 1);
				if (simple_html->text)
				{
					memcpy(simple_html->text, file->data, file->size);
					simple_html->text[file->size] = '\0';
				}
				else
				{
					LOG_ERROR("malloc failed");
				}
				cache_unref_by_ref_mpq(g_wow->cache, file);
			}
			else
			{
				LOG_ERROR("failed to get %s", filename);
			}
		}
		if (xml_simple_html->hyperlink_format)
		{
			mem_free(MEM_UI, simple_html->hyperlink_format);
			simple_html->hyperlink_format = mem_strdup(MEM_UI, xml_simple_html->hyperlink_format);
			if (!simple_html->hyperlink_format)
				LOG_ERROR("failed to duplicate hyperlink format");
		}
		if (OPTIONAL_ISSET(xml_simple_html->font_string))
			simple_html->paragraph_font_string = &OPTIONAL_GET(xml_simple_html->font_string);
		if (OPTIONAL_ISSET(xml_simple_html->font_string_header1))
			simple_html->header1_font_string = &OPTIONAL_GET(xml_simple_html->font_string_header1);
		if OPTIONAL_ISSET((xml_simple_html->font_string_header2))
			simple_html->header2_font_string = &OPTIONAL_GET(xml_simple_html->font_string_header2);
		if OPTIONAL_ISSET((xml_simple_html->font_string_header3))
			simple_html->header3_font_string = &OPTIONAL_GET(xml_simple_html->font_string_header3);
	}
}

static void render(struct ui_object *object)
{
	struct ui_simple_html *simple_html = (struct ui_simple_html*)object;
	if (!simple_html->font_string && simple_html->paragraph_font_string)
	{
		simple_html->font_string = ui_font_string_new(UI_OBJECT->interface, NULL, UI_REGION);
		if (simple_html->font_string)
		{
			ui_object_load_xml((struct ui_object*)simple_html->font_string, (const struct xml_layout_frame*)simple_html->paragraph_font_string);
			ui_object_post_load((struct ui_object*)simple_html->font_string);
			ui_font_string_set_text(simple_html->font_string, simple_html->text);
		}
	}
	ui_frame_vtable.render(object);
	if (simple_html->font_string)
		ui_object_render((struct ui_object*)simple_html->font_string);
}

static void on_font_height_changed(struct ui_object *object)
{
	(void)object;
	/* XXX */
}

static void on_color_changed(struct ui_object *object)
{
	(void)object;
	/* XXX */
}

static void on_shadow_color_changed(struct ui_object *object)
{
	(void)object;
	/* XXX */
}

static void on_spacing_changed(struct ui_object *object)
{
	(void)object;
	/* XXX */
}

static void on_outline_changed(struct ui_object *object)
{
	(void)object;
	/* XXX */
}

static void on_monochrome_changed(struct ui_object *object)
{
	(void)object;
	/* XXX */
}

static void on_justify_h_changed(struct ui_object *object)
{
	(void)object;
	/* XXX */
}

static void on_justify_v_changed(struct ui_object *object)
{
	(void)object;
	/* XXX */
}

static int lua_SetText(lua_State *L)
{
	LUA_METHOD_SIMPLE_HTML();
	if (argc < 1 || argc > 2)
		return luaL_error(L, "Usage: ui_simple_html_t::SetText(\"text\")");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static bool register_methods(struct jks_array *methods)
{
	/*
	   GetFont
	   GetFontObject
	   GetHyperlinkFormat
	   GetJustifyH
	   GetJustifyV
	   GetShadowColor
	   GetShadowOffset
	   GetSpacing
	   GetTextColor
	   SetFont
	   SetFontObject
	   SetHyperlinkFormat
	   SetJustifyH
	   SetJustifyV
	   SetShadowColor
	   SetShadowOffset
	   SetSpacing
	   SetTextColor
	 */
	UI_REGISTER_METHOD(SetText);
	if (!ui_font_instance_register_methods(methods)) /* only add to vector */
		return false;
	return ui_frame_vtable.register_methods(methods);
}

UI_INH0(frame, void, post_load);
UI_INH0(frame, void, register_in_interface);
UI_INH0(frame, void, unregister_in_interface);
UI_INH0(frame, void, eval_name);
UI_INH1(frame, void, on_click, enum gfx_mouse_button, button);
UI_INH0(frame, float, get_alpha);
UI_INH1(frame, void, set_alpha, float, alpha);
UI_INH1(frame, void, set_hidden, bool, hidden);
UI_INH2(frame, void, get_size, int32_t*, x, int32_t*, y);
UI_INH0(frame, void, set_dirty_coords);
UI_INH1(frame, void, on_mouse_move, gfx_pointer_event_t*, event);
UI_INH1(frame, void, on_mouse_down, gfx_mouse_event_t*, event);
UI_INH1(frame, void, on_mouse_up, gfx_mouse_event_t*, event);
UI_INH1(frame, void, on_mouse_scroll, gfx_scroll_event_t*, event);
UI_INH1(frame, bool, on_key_down, gfx_key_event_t*, event);
UI_INH1(frame, bool, on_key_up, gfx_key_event_t*, event);
UI_INH0(frame, struct ui_font_instance*, as_font_instance);
UI_INH0(frame, const char*, get_name);

const struct ui_object_vtable ui_simple_html_vtable =
{
	UI_OBJECT_VTABLE("SimpleHTML")
};
