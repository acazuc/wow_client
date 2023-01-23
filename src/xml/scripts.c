#include "xml/internal.h"
#include "xml/scripts.h"

#include "memory.h"
#include "log.h"

#include <libxml/tree.h>

#include <string.h>

MEMORY_DECL(XML);

static void hmap_delete(jks_hmap_key_t key, void *val)
{
	mem_free(MEM_XML, key.ptr);
	mem_free(MEM_XML, *(void**)val);
}

static void ctr(struct xml_element *element)
{
	xml_element_vtable.ctr(element);
	struct xml_scripts *scripts = (struct xml_scripts*)element;
	jks_hmap_init(&scripts->scripts, sizeof(char*), hmap_delete, jks_hmap_hash_str, jks_hmap_cmp_str, &jks_hmap_memory_fn_XML);
}

static void dtr(struct xml_element *element)
{
	struct xml_scripts *scripts = (struct xml_scripts*)element;
	jks_hmap_destroy(&scripts->scripts);
	xml_element_vtable.dtr(element);
}

static enum xml_parse_attribute_status parse_attribute(struct xml_element *element, const struct xml_attr *attribute)
{
	return xml_element_vtable.parse_attribute(element, attribute);
}

static enum xml_parse_child_status parse_child(struct xml_element *element, const struct xml_node *child)
{
	struct xml_scripts *scripts = (struct xml_scripts*)element;
	switch (child->type)
	{
		case XML_NODE_TEXT:
			return XML_PARSE_CHILD_OK;
		case XML_NODE_ELEMENT:
		{
			static const char *values[] =
			{
				"OnEnter", "OnLeave", "OnClick", "OnLoad", "OnEvent", "OnCharComposition",
				"OnShow", "OnHide", "OnValueChanged", "OnUpdate", "OnKeyDown", "OnKeyUp", "OnDragStart",
				"OnDragStop", "OnMouseDown", "OnMouseUp", "OnMouseWheel", "OnReceiveDrag", "OnVerticalScroll",
				"OnChar", "PreClick", "PostClick", "OnScrollRangeChanged", "OnAttributeChanged", "OnTooltipSetUnit",
				"OnTabPressed", "OnEnterPressed", "OnEscapePressed", "OnTextChanged", "OnEditFocusLost",
				"OnEditFocusGained", "OnSpacePressed", "OnTextSet", "OnInputLanguageChanged", "OnTooltipSetItem",
				"OnColorSelect", "OnCursorChanged", "OnAnimFinished", "OnTooltipSetDefaultAnchor", "OnTooltipAddMoney",
				"OnTooltipCleared", "OnHyperlinkClick", "OnDoubleClick", "OnUpdateModel", "OnMovieFinished",
				"OnMovieShowSubtitle", "OnMovieHideSubtitle"
			};
			for (size_t i = 0; i < sizeof(values) / sizeof(*values); ++i)
			{
				if (strcmp(child->name, values[i]))
					continue;
				if (!child->node->children)
					return XML_PARSE_CHILD_INVALID_TYPE;
				jks_hmap_iterator_t iter = jks_hmap_iterator_find(&scripts->scripts, JKS_HMAP_KEY_PTR((void*)values[i]));
				if (!jks_hmap_iterator_is_end(&scripts->scripts, &iter))
					return XML_PARSE_CHILD_ALREADY;
				char *val = NULL;
				for (const xmlNode *node = child->node->children; node; node = node->next)
				{
					if (node->type == XML_COMMENT_NODE)
						continue;
					if (node->type != XML_TEXT_NODE)
					{
						free(val);
						return XML_PARSE_CHILD_INVALID_TYPE;
					}
					size_t val_len = val ? strlen(val) : 0;
					char *newval = mem_realloc(MEM_XML, val, val_len + strlen((const char*)node->content) + 1);
					if (!newval)
					{
						LOG_ERROR("malloc failed");
						free(val);
						return XML_PARSE_CHILD_INTERNAL;
					}
					newval[val_len] = '\0';
					strcat(newval, (const char*)node->content);
					val = newval;
				}
				if (val)
				{
					char *key = mem_strdup(MEM_XML, child->name);
					if (!key || !val)
					{
						LOG_ERROR("malloc failed");
						free(key);
						free(val);
						return XML_PARSE_CHILD_INTERNAL;
					}
					if (!jks_hmap_set(&scripts->scripts, JKS_HMAP_KEY_PTR(key), &val))
					{
						LOG_ERROR("failed to add script");
						free(key);
						free(val);
						return XML_PARSE_CHILD_INTERNAL;
					}
				}
				return XML_PARSE_CHILD_OK;
			}
			break;
		}
		default:
			break;
	}
	return xml_element_vtable.parse_child(element, child);
}

const struct xml_vtable xml_scripts_vtable =
{
	.name = "Scripts",
	.ctr = ctr,
	.dtr = dtr,
	.parse_attribute = parse_attribute,
	.parse_child = parse_child,
};
