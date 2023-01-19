#ifndef GAMEOBJ_H
#define GAMEOBJ_H

#include "obj/worldobj.h"

struct gameobj
{
	struct worldobj worldobj;
};

struct gameobj *gameobj_new(uint64_t guid);

extern const struct object_vtable gameobj_object_vtable;
extern const struct worldobj_vtable gameobj_worldobj_vtable;

#endif
