#ifndef CORPSE_H
#define CORPSE_H

#include "obj/object.h"

struct corpse
{
	struct object object;
};

struct corpse *corpse_new(uint64_t guid);

extern const struct object_vtable corpse_object_vtable;

#endif
