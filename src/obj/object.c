#include "object.h"

#include "obj/update_fields.h"
#include "obj/container.h"

#include "obj/worldobj.h"
#include "obj/gameobj.h"
#include "obj/player.h"
#include "obj/item.h"
#include "obj/unit.h"

#include "net/packet.h"

#include "memory.h"
#include "const.h"
#include "log.h"
#include "wow.h"

#include <jks/hmap.h>

#include <assert.h>
#include <string.h>

static void (*g_field_functions[OBJECT_FIELD_MAX])(struct object *object) =
{
};

static bool ctr(struct object *object, uint64_t guid)
{
	object_fields_init(&object->fields);
	if (!object_fields_resize(&object->fields, OBJECT_FIELD_MAX))
		return false;
	object_fields_set_u32(&object->fields, OBJECT_FIELD_TYPE, OBJECT_MASK_OBJECT);
	object_fields_set_u64(&object->fields, OBJECT_FIELD_GUID, guid);
	object_fields_set_flt(&object->fields, OBJECT_FIELD_SCALE, 1);
	if (!wow_set_object(guid, object))
		return false;
	return true;
}

static void dtr(struct object *object)
{
	wow_set_object(object_guid(object), NULL);
	object_fields_destroy(&object->fields);
}

static void on_field_changed(struct object *object, uint32_t field)
{
	if (field >= OBJECT_FIELD_MAX)
		return;
	if (g_field_functions[field])
		g_field_functions[field](object);
}

bool object_read_update_data(struct object *object, struct net_packet_reader *packet)
{
	uint8_t flags;
	if (!net_read_u8(packet, &flags))
		return false;
	if (flags & 1)
	{
		if (object_is_player(object))
			wow_set_player(object_as_player(object));
		else
			LOG_ERROR("self is not player");
	}
	if (flags & 0x20)
	{
		if (!object_is_worldobj(object))
		{
			LOG_ERROR("update 0x20 of non-worldobject");
			return false;
		}
		struct worldobj *worldobj = object_as_worldobj(object);
		if (!worldobj_read_movement_data(worldobj, packet))
		{
			LOG_ERROR("can't read movement data");
			return false;
		}
		float walk_speed, run_speed, walk_back_speed, swim_speed, swim_back_speed, flight_speed, flight_back_speed, turn_rate;
		if (!net_read_flt(packet, &walk_speed)
		 || !net_read_flt(packet, &run_speed)
		 || !net_read_flt(packet, &walk_back_speed)
		 || !net_read_flt(packet, &swim_speed)
		 || !net_read_flt(packet, &swim_back_speed)
		 || !net_read_flt(packet, &flight_speed)
		 || !net_read_flt(packet, &flight_back_speed)
		 || !net_read_flt(packet, &turn_rate))
			return false;
		worldobj->walk_speed = walk_speed;
		worldobj->run_speed = run_speed;
		worldobj->walk_back_speed = walk_back_speed;
		worldobj->swim_speed = swim_speed;
		worldobj->swim_back_speed = swim_back_speed;
		worldobj->flight_speed = flight_speed;
		worldobj->flight_back_speed = flight_back_speed;
		worldobj->turn_rate = turn_rate;
	}
	else if (flags & 0x40)
	{
		if (!object_is_worldobj(object))
		{
			LOG_ERROR("update 0x40 of non-worldobject");
			return false;
		}
		struct worldobj *worldobj = object_as_worldobj(object);
		float x, y, z, w;
		if (!net_read_flt(packet, &x)
		 || !net_read_flt(packet, &y)
		 || !net_read_flt(packet, &z)
		 || !net_read_flt(packet, &w))
			return false;
		worldobj_set_position(worldobj, (struct vec3f){x, z, -y});
		worldobj_set_orientation(worldobj, w);
	}
	if (flags & 0x8)
	{
		uint32_t lowguid;
		if (!net_read_u32(packet, &lowguid))
			return false;
	}
	if (flags & 0x10)
	{
		uint32_t highguid;
		if (!net_read_u32(packet, &highguid))
			return false;
	}
	if (flags & 0x4)
	{
		uint64_t target_guid;
		if (!net_read_guid(packet, &target_guid))
			return false;
	}
	if (flags & 0x2)
	{
		uint32_t worldtime;
		if (!net_read_u32(packet, &worldtime))
			return false;
	}
	return true;
}

bool object_read_fields(struct object *object, struct net_packet_reader *packet)
{
	uint32_t mask[256];
	uint8_t length;
	if (!net_read_u8(packet, &length))
		return false;
	if (!net_read_bytes(packet, mask, length * sizeof(*mask)))
		return false;
	for (size_t i = 0; i < length * sizeof(*mask) * 8; ++i)
	{
		if (mask[i / (sizeof(*mask) * 8)] & (1u << (i % (sizeof(*mask) * 8))))
		{
			uint32_t field;
			if (!net_read_u32(packet, &field))
				return false;
			object_fields_set_u32(&object->fields, i, field);
		}
	}
	return true;
}

uint64_t object_guid(struct object *object)
{
	return object_fields_get_u64(&object->fields, OBJECT_FIELD_GUID);
}

bool object_is_unit(struct object *object)
{
	return object_fields_get_u32(&object->fields, OBJECT_FIELD_TYPE) & OBJECT_MASK_UNIT;
}

bool object_is_player(struct object *object)
{
	return object_fields_get_u32(&object->fields, OBJECT_FIELD_TYPE) & OBJECT_MASK_PLAYER;
}

bool object_is_item(struct object *object)
{
	return object_fields_get_u32(&object->fields, OBJECT_FIELD_TYPE) & OBJECT_MASK_ITEM;
}

bool object_is_container(struct object *object)
{
	return object_fields_get_u32(&object->fields, OBJECT_FIELD_TYPE) & OBJECT_MASK_CONTAINER;
}

bool object_is_gameobj(struct object *object)
{
	return object_fields_get_u32(&object->fields, OBJECT_FIELD_TYPE) & OBJECT_MASK_GAMEOBJECT;
}

bool object_is_worldobj(struct object *object)
{
	return object_is_unit(object) || object_is_gameobj(object);
}

bool object_is_dynobj(struct object *object)
{
	return object_fields_get_u32(&object->fields, OBJECT_FIELD_TYPE) & OBJECT_MASK_DYNAMICOBJECT;
}

bool object_is_corpse(struct object *object)
{
	return object_fields_get_u32(&object->fields, OBJECT_FIELD_TYPE) & OBJECT_MASK_CORPSE;
}

#define OBJECT_CAST_DEF(type) \
	struct type *object_as_##type(struct object *object) \
	{ \
		if (!object_is_##type(object)) \
			return NULL; \
		return (struct type*)object; \
	} \
	struct type *object_get_##type(uint64_t guid) \
	{ \
		struct object *object = wow_get_object(guid); \
		if (!object) \
			return NULL; \
		return object_as_##type(object); \
	}

OBJECT_CAST_DEF(unit);
OBJECT_CAST_DEF(player);
OBJECT_CAST_DEF(item);
OBJECT_CAST_DEF(container);
OBJECT_CAST_DEF(gameobj);
OBJECT_CAST_DEF(worldobj);
OBJECT_CAST_DEF(dynobj);
OBJECT_CAST_DEF(corpse);

#undef OBJECT_CAST_DEF

const struct object_vtable object_vtable =
{
	.ctr = ctr,
	.dtr = dtr,
	.on_field_changed = on_field_changed,
};

struct object *object_new(uint64_t guid)
{
	struct object *object = mem_malloc(MEM_OBJ, sizeof(struct object));
	if (!object)
		return NULL;
	object->vtable = &object_vtable;
	if (!object->vtable->ctr(object, guid))
	{
		object->vtable->dtr(object);
		mem_free(MEM_OBJ, object);
		return NULL;
	}
	return (struct object*)object;
}

void object_delete(struct object *object)
{
	if (!object)
		return;
	object->vtable->dtr(object);
	mem_free(MEM_OBJ, object);
}
