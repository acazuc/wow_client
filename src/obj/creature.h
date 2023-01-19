#ifndef CREATURE_H
#define CREATURE_H

#include "obj/unit.h"

struct creature
{
	struct unit unit;
	uint8_t skin;
	uint8_t face;
	uint8_t hair_style;
	uint8_t hair_color;
	uint8_t facial_hair;
	bool db_get;
	bool clean_batches;
	struct blp_texture *monster_textures[3];
	struct blp_texture *skin_extra_texture;
	struct blp_texture *skin_texture;
};

struct creature *creature_new(uint64_t guid);

extern const struct object_vtable creature_object_vtable;
extern const struct unit_vtable creature_unit_vtable;
extern const struct worldobj_vtable creature_worldobj_vtable;

#endif
