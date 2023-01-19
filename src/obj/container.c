#include "container.h"

#include "obj/update_fields.h"

#include "memory.h"
#include "const.h"
#include "log.h"

#define ITEM ((struct item*)object)
#define CONTAINER ((struct container*)object)

static void (*g_field_functions[CONTAINER_FIELD_MAX - CONTAINER_FIELD_MIN])(struct object *object) =
{
};

static bool ctr(struct object *object, uint64_t guid)
{
	if (!item_object_vtable.ctr(object, guid))
		return false;
	if (!object_fields_resize(&object->fields, CONTAINER_FIELD_MAX))
		return false;
	object_fields_set_u32(&object->fields, OBJECT_FIELD_TYPE, OBJECT_MASK_OBJECT | OBJECT_MASK_ITEM | OBJECT_MASK_CONTAINER);
	return true;
}

static void dtr(struct object *object)
{
	item_object_vtable.dtr(object);
}

static void on_field_changed(struct object *object, uint32_t field)
{
	if (field < CONTAINER_FIELD_MIN)
		return item_object_vtable.on_field_changed(object, field);
	if (field >= CONTAINER_FIELD_MAX)
		return;
	if (g_field_functions[field - CONTAINER_FIELD_MIN])
		g_field_functions[field - CONTAINER_FIELD_MIN](object);
}

const struct object_vtable container_object_vtable =
{
	.ctr = ctr,
	.dtr = dtr,
	.on_field_changed = on_field_changed,
};

struct container *container_new(uint64_t guid)
{
	struct object *object = mem_malloc(MEM_OBJ, sizeof(struct container));
	if (!object)
		return NULL;
	object->vtable = &container_object_vtable;
	if (!object->vtable->ctr(object, guid))
	{
		object->vtable->dtr(object);
		mem_free(MEM_OBJ, object);
		return NULL;
	}
	return (struct container*)object;
}
