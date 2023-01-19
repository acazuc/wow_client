#ifndef PLAYER_H
#define PLAYER_H

#include "obj/unit.h"

struct blp_texture;

struct player
{
	struct unit unit;
	struct blp_texture *skin_extra_texture;
	struct blp_texture *skin_texture;
	struct blp_texture *hair_texture;
	struct unit_item left_hand_item;
	struct unit_item right_hand_item;
	bool clean_batches;
	bool db_get;
};

struct player *player_new(uint64_t guid);

extern const struct object_vtable player_object_vtable;
extern const struct unit_vtable player_unit_vtable;
extern const struct worldobj_vtable player_worldobj_vtable;

#endif
