#ifndef ITEM_H
#define ITEM_H

#include "obj/object.h"

struct item
{
	struct object object;
};

struct item *item_new(uint64_t guid);

extern const struct object_vtable item_object_vtable;

#endif
