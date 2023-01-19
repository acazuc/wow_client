#include "gameobj.h"

#include "obj/update_fields.h"

#include "gx/m2.h"

#include "memory.h"
#include "const.h"
#include "log.h"
#include "wow.h"
#include "dbc.h"

#include <jks/quaternion.h>

#include <libwow/dbc.h>

#include <stdio.h>

#define WORLDOBJ ((struct worldobj*)object)
#define GAMEOBJ ((struct gameobj*)object)

static void (*g_field_functions[GAMEOBJECT_FIELD_MAX - GAMEOBJECT_FIELD_MIN])(struct object *object) =
{
};

static bool ctr(struct object *object, uint64_t guid)
{
	if (!worldobj_object_vtable.ctr(object, guid))
		return false;
	WORLDOBJ->worldobj_vtable = &gameobj_worldobj_vtable;
	if (!object_fields_resize(&object->fields, GAMEOBJECT_FIELD_MAX))
		return false;
	object_fields_set_u32(&object->fields, OBJECT_FIELD_TYPE, OBJECT_MASK_OBJECT | OBJECT_MASK_GAMEOBJECT);
	return true;
}

static void dtr(struct object *object)
{
	worldobj_object_vtable.dtr(object);
}

static void on_field_changed(struct object *object, uint32_t field)
{
	if (field < GAMEOBJECT_FIELD_MIN)
		return object_vtable.on_field_changed(object, field);
	if (field >= GAMEOBJECT_FIELD_MAX)
		return;
	if (g_field_functions[field - GAMEOBJECT_FIELD_MIN])
		g_field_functions[field - GAMEOBJECT_FIELD_MIN](object);
}

static void add_to_render(struct object *object)
{
	if (object_fields_get_bit(&object->fields, GAMEOBJECT_FIELD_DISPLAYID))
	{
		object_fields_disable_bit(&object->fields, GAMEOBJECT_FIELD_DISPLAYID);
		wow_dbc_row_t row;
		if (dbc_get_row_indexed(g_wow->dbc.game_object_display_info, &row, object_fields_get_u32(&object->fields, GAMEOBJECT_FIELD_DISPLAYID))) 
		{
			char filename[512];
			snprintf(filename, sizeof(filename), "%s", wow_dbc_get_str(&row, 4));
			normalize_m2_filename(filename, sizeof(filename));
			worldobj_set_m2(WORLDOBJ, filename);
		}
		else
		{
			gx_m2_instance_gc(WORLDOBJ->m2);
			WORLDOBJ->m2 = NULL;
		}
	}
	if (!WORLDOBJ->m2)
	{
		worldobj_vtable.add_to_render(object);
		return;
	}
	VEC3_CPY(WORLDOBJ->m2->pos, WORLDOBJ->position);
	struct mat4f mat;
	struct mat4f tmp;
	MAT4_IDENTITY(tmp);
	MAT4_TRANSLATE(mat, tmp, WORLDOBJ->position);
	struct mat4f quat;
	struct vec4f tmp_quat = {object_fields_get_flt(&object->fields, GAMEOBJECT_FIELD_ROTATION_X), object_fields_get_flt(&object->fields, GAMEOBJECT_FIELD_ROTATION_Z), -object_fields_get_flt(&object->fields, GAMEOBJECT_FIELD_ROTATION_Y), object_fields_get_flt(&object->fields, GAMEOBJECT_FIELD_ROTATION_W)};
	QUATERNION_TO_MAT4(float, quat, tmp_quat);
	MAT4_MUL(tmp, mat, quat);
	MAT4_SCALEV(mat, tmp, object_fields_get_flt(&object->fields, OBJECT_FIELD_SCALE));
	gx_m2_instance_set_mat(WORLDOBJ->m2, (struct mat4f*)&mat);
	worldobj_vtable.add_to_render(object);
}

const struct object_vtable gameobj_object_vtable =
{
	.ctr = ctr,
	.dtr = dtr,
	.on_field_changed = on_field_changed,
};

const struct worldobj_vtable gameobj_worldobj_vtable =
{
	.add_to_render = add_to_render,
};

struct gameobj *gameobj_new(uint64_t guid)
{
	struct object *object = mem_malloc(MEM_OBJ, sizeof(struct gameobj));
	if (!object)
		return NULL;
	object->vtable = &gameobj_object_vtable;
	if (!object->vtable->ctr(object, guid))
	{
		object->vtable->dtr(object);
		mem_free(MEM_OBJ, object);
		return NULL;
	}
	return (struct gameobj*)object;
}
