#ifndef OBJECT_H
#define OBJECT_H

#include "obj/fields.h"

#include <jks/array.h>

#include <stdint.h>

struct net_packet_reader;
struct object;

struct object_vtable
{
	bool (*ctr)(struct object *object, uint64_t guid);
	void (*dtr)(struct object *object);
	void (*on_field_changed)(struct object *object, uint32_t field);
};

struct object
{
	const struct object_vtable *vtable;
	struct object_fields fields;
};

struct object *object_new(uint64_t guid);
void object_delete(struct object *object);
bool object_read_update_data(struct object *object, struct net_packet_reader *packet);
bool object_read_fields(struct object *object, struct net_packet_reader *packet);
uint64_t object_guid(struct object *object);

#define OBJECT_CAST_DEF(type) \
	bool object_is_##type(struct object *object); \
	struct type *object_as_##type(struct object *object); \
	struct type *object_get_##type(uint64_t guid);

OBJECT_CAST_DEF(unit);
OBJECT_CAST_DEF(player);
OBJECT_CAST_DEF(item);
OBJECT_CAST_DEF(container);
OBJECT_CAST_DEF(gameobj);
OBJECT_CAST_DEF(worldobj);
OBJECT_CAST_DEF(dynobj);
OBJECT_CAST_DEF(corpse);

#undef OBJECT_CAST_DEF

extern const struct object_vtable object_vtable;

#endif
