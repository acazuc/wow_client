#include "dynobj.h"

#include "obj/update_fields.h"

#include "memory.h"
#include "const.h"
#include "log.h"

#define DYNOBJ ((struct dynobj*)object)

static bool ctr(struct object *object, uint64_t guid)
{
	if (!object_vtable.ctr(object, guid))
		return false;
	if (!object_fields_resize(&object->fields, DYNAMICOBJECT_FIELD_MAX))
		return false;
	object_fields_set_u32(&object->fields, OBJECT_FIELD_TYPE, OBJECT_MASK_OBJECT | OBJECT_MASK_DYNAMICOBJECT);
	return true;
}

static void dtr(struct object *object)
{
	object_vtable.dtr(object);
}

static void on_field_changed(struct object *object, uint32_t field)
{
	return object_vtable.on_field_changed(object, field);
}

const struct object_vtable dynobj_object_vtable =
{
	.ctr = ctr,
	.dtr = dtr,
	.on_field_changed = on_field_changed,
};

struct dynobj *dynobj_new(uint64_t guid)
{
	struct object *object = mem_malloc(MEM_OBJ, sizeof(struct dynobj));
	if (!object)
		return NULL;
	object->vtable = &dynobj_object_vtable;
	if (!object->vtable->ctr(object, guid))
	{
		object->vtable->dtr(object);
		mem_free(MEM_OBJ, object);
		return NULL;
	}
	return (struct dynobj*)object;
}
