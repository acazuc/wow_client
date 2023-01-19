#include "corpse.h"

#include "obj/update_fields.h"

#include "memory.h"
#include "const.h"
#include "log.h"

#define CORPSE ((struct corpse*)object)

static void (*g_field_functions[CORPSE_FIELD_MAX - CORPSE_FIELD_MIN])(struct object *object) =
{
};

static bool ctr(struct object *object, uint64_t guid)
{
	if (!object_vtable.ctr(object, guid))
		return false;
	if (!object_fields_resize(&object->fields, CORPSE_FIELD_MAX))
		return false;
	object_fields_set_u32(&object->fields, OBJECT_FIELD_TYPE, OBJECT_MASK_OBJECT | OBJECT_MASK_CORPSE);
	return true;
}

static void dtr(struct object *object)
{
	object_vtable.dtr(object);
}

static void on_field_changed(struct object *object, uint32_t field)
{
	if (field < CORPSE_FIELD_MIN)
		return object_vtable.on_field_changed(object, field);
	if (field >= CORPSE_FIELD_MAX)
		return;
	if (g_field_functions[field - CORPSE_FIELD_MIN])
		g_field_functions[field - CORPSE_FIELD_MIN](object);
}

const struct object_vtable corpse_object_vtable =
{
	.ctr = ctr,
	.dtr = dtr,
	.on_field_changed = on_field_changed,
};

struct corpse *corpse_new(uint64_t guid)
{
	struct object *object = mem_malloc(MEM_OBJ, sizeof(struct corpse));
	if (!object)
		return NULL;
	object->vtable = &corpse_object_vtable;
	if (!object->vtable->ctr(object, guid))
	{
		object->vtable->dtr(object);
		mem_free(MEM_OBJ, object);
		return NULL;
	}
	return (struct corpse*)object;
}
