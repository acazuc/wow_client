#include "itf/interface.h"
#include "itf/addon.h"

#include "xml/layout_frame.h"
#include "xml/internal.h"
#include "xml/include.h"
#include "xml/script.h"
#include "xml/font.h"
#include "xml/ui.h"

#include "lua/lua_script.h"

#include "ui/object.h"
#include "ui/frame.h"

#include "memory.h"
#include "log.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

MEMORY_DECL(XML);

static void layout_frame_dtr(void *ptr)
{
	struct xml_element *layout_frame = *(struct xml_element**)ptr;
	xml_element_delete(layout_frame);
}

static void font_dtr(void *ptr)
{
	struct xml_element *font = *(struct xml_element**)ptr;
	xml_element_delete(font);
}

struct xml_ui *xml_ui_new(struct addon *addon, const char *filename)
{
	struct xml_ui *ui = mem_malloc(MEM_XML, sizeof(*ui));
	if (!ui)
		return NULL;
	ui->filename = mem_strdup(MEM_XML, filename);
	if (!ui->filename)
	{
		mem_free(MEM_XML, ui);
		return NULL;
	}
	for (size_t i = 0; ui->filename[i]; ++i)
	{
		if (ui->filename[i] == '\\')
			ui->filename[i] = '/';
	}
	ui->addon = addon;
	((struct xml_element*)ui)->vtable = &xml_ui_vtable;
	((struct xml_element*)ui)->vtable->ctr((struct xml_element*)ui);
	return ui;
}

static void ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_ui *ui = (struct xml_ui*)element;
	jks_array_init(&ui->layout_frames, sizeof(struct xml_layout_frame*), layout_frame_dtr, &jks_array_memory_fn_XML);
	jks_array_init(&ui->fonts, sizeof(struct xml_font*), font_dtr, &jks_array_memory_fn_XML);
}

static void dtr(struct xml_element *element)
{
	struct xml_ui *ui = (struct xml_ui*)element;
	jks_array_destroy(&ui->layout_frames);
	jks_array_destroy(&ui->fonts);
	mem_free(MEM_XML, ui->filename);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	if (!strcmp(attribute->name, "xmlns"))
	{
		if (!attribute->value || strcmp(attribute->value, "http://www.blizzard.com/wow/ui/"))
			return XML_PARSE_ATTRIBUTE_INVALID_VALUE;
		return XML_PARSE_ATTRIBUTE_OK;
	}
	if (!strcmp(attribute->name, "xmlns:xsi"))
	{
		if (!attribute->value || strcmp(attribute->value, "http://www.w3.org/2001/XMLSchema-instance"))
			return XML_PARSE_ATTRIBUTE_INVALID_VALUE;
		return XML_PARSE_ATTRIBUTE_OK;
	}
	if (!strcmp(attribute->name, "schemaLocation"))
		return XML_PARSE_ATTRIBUTE_OK; /* bypass xsd file */
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_ui *ui = (struct xml_ui*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
		{
			if (!strcmp((const char*)child->name, "Script"))
			{
				struct xml_element *script = mem_malloc(MEM_XML, sizeof(struct xml_script));
				script->vtable = &xml_script_vtable;
				script->vtable->ctr(script);
				xml_element_parse(script, child);
				if (((struct xml_script*)script)->file)
				{
					ui->addon->vtable->load_lua(ui->addon, ((struct xml_script*)script)->file, ui->filename);
				}
				else if (((struct xml_script*)script)->script)
				{
					struct lua_script *lua_script = lua_script_new(ui->addon->interface->L, ((struct xml_script*)script)->script, NULL);
					lua_script_execute(lua_script);
					lua_script_delete(lua_script);
				}
				script->vtable->dtr(script);
				return XML_PARSE_CHILD_OK;
			}
			if (!strcmp((const char*)child->name, "Include"))
			{
				struct xml_element *include = mem_malloc(MEM_XML, sizeof(struct xml_include));
				include->vtable = &xml_include_vtable;
				include->vtable->ctr(include);
				xml_element_parse(include, child);
				if (((struct xml_include*)include)->file)
					ui->addon->vtable->load_xml(ui->addon, ((struct xml_include*)include)->file, ui->filename);
				else
					LOG_ERROR("no file for Include");
				xml_element_delete(include);
				return XML_PARSE_CHILD_OK;
			}
			if (!strcmp((const char*)child->name, "Font"))
			{
				struct xml_element *font = mem_malloc(MEM_XML, sizeof(struct xml_font));
				font->vtable = &xml_font_vtable;
				font->vtable->ctr(font);
				xml_element_parse(font, child);
				xml_font_load_into_interface((struct xml_font*)font, ui->addon->interface);
				if (!jks_array_push_back(&ui->fonts, &font))
				{
					LOG_ERROR("failed to add font to fonts list");
					return XML_PARSE_CHILD_INTERNAL;
				}
				return XML_PARSE_CHILD_OK;
			}
			struct xml_layout_frame *layout_frame = xml_create_layout_frame((const char*)child->name);
			if (!layout_frame)
				break;
			xml_element_parse((struct xml_element*)layout_frame, child);
			if (OPTIONAL_ISSET(layout_frame->is_virtual) && OPTIONAL_GET(layout_frame->is_virtual))
			{
				if (layout_frame->name)
					interface_register_virtual_layout_frame(ui->addon->interface, layout_frame->name, layout_frame);
				else
					LOG_ERROR("virtual without name");
			}
			else
			{
				struct ui_region *region = ((struct xml_element*)layout_frame)->vtable->load_interface(layout_frame, ui->addon->interface, NULL);
				if (region)
				{
					ui_object_load_xml((struct ui_object*)region, layout_frame);
					ui_object_eval_name((struct ui_object*)region);
					ui_object_post_load((struct ui_object*)region);
				}
			}
			if (!jks_array_push_back(&ui->layout_frames, &layout_frame))
			{
				LOG_ERROR("failed to add layout frame to list");
				return XML_PARSE_CHILD_INTERNAL;
			}
			return XML_PARSE_CHILD_OK;
		}
		default:
			break;
	}
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_ui_vtable =
{
	.name = "Ui",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
