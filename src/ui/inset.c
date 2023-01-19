#include "inset.h"

#include "xml/inset.h"

#include "log.h"

#include <string.h>

#ifdef interface
# undef interface
#endif

static void load_xml(struct ui_inset *inset, const struct xml_inset *xml);

void ui_inset_init_xml(struct ui_inset *inset, const struct xml_inset *xml)
{
	inset->type = INSET_ABSOLUTE;
	load_xml(inset, xml);
}

void ui_inset_init(struct ui_inset *inset, int32_t left, int32_t right, int32_t top, int32_t bottom)
{
	inset->type = INSET_ABSOLUTE;
	inset->abs.left = left;
	inset->abs.right = right;
	inset->abs.top = top;
	inset->abs.bottom = bottom;
}

static void load_xml(struct ui_inset *inset, const struct xml_inset *xml)
{
	if (OPTIONAL_ISSET(xml->left))
		inset->abs.left = OPTIONAL_GET(xml->left);
	else
		inset->abs.left = 0;
	if (OPTIONAL_ISSET(xml->right))
		inset->abs.right = OPTIONAL_GET(xml->right);
	else
		inset->abs.right = 0;
	if (OPTIONAL_ISSET(xml->top))
		inset->abs.top = OPTIONAL_GET(xml->bottom);
	else
		inset->abs.top = 0;
	if (OPTIONAL_ISSET(xml->bottom))
		inset->abs.bottom = OPTIONAL_GET(xml->bottom);
	else
		inset->abs.bottom = 0;
	if (xml->inset)
	{
		const struct xml_element *ins = xml->inset;
		if (!strcmp(ins->vtable->name, "AbsInset"))
		{
			inset->type = INSET_ABSOLUTE;
			const struct xml_abs_inset *abs_ins = (const struct xml_abs_inset*)ins;
			if (OPTIONAL_ISSET(abs_ins->left))
				inset->abs.left = OPTIONAL_GET(abs_ins->left);
			if (OPTIONAL_ISSET(abs_ins->right))
				inset->abs.right = OPTIONAL_GET(abs_ins->right);
			if (OPTIONAL_ISSET(abs_ins->top))
				inset->abs.top = OPTIONAL_GET(abs_ins->top);
			if (OPTIONAL_ISSET(abs_ins->bottom))
				inset->abs.bottom = OPTIONAL_GET(abs_ins->bottom);
		}
		else if (!strcmp(ins->vtable->name, "RelInset"))
		{
			inset->type = INSET_RELATIVE;
			const struct xml_rel_inset *rel_ins = (const struct xml_rel_inset*)ins;
			if (OPTIONAL_ISSET(rel_ins->left))
				inset->rel.left = OPTIONAL_GET(rel_ins->left);
			else
				inset->rel.left = 1;
			if (OPTIONAL_ISSET(rel_ins->right))
				inset->rel.right = OPTIONAL_GET(rel_ins->right);
			else
				inset->rel.right = 1;
			if (OPTIONAL_ISSET(rel_ins->top))
				inset->rel.top = OPTIONAL_GET(rel_ins->top);
			else
				inset->rel.top = 1;
			if (OPTIONAL_ISSET(rel_ins->bottom))
				inset->rel.bottom = OPTIONAL_GET(rel_ins->bottom);
			else
				inset->rel.bottom = 1;
		}
		else
		{
			LOG_WARN("invalid inset type: %s", ins->vtable->name);
		}
	}
}
