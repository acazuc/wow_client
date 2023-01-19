#ifndef CONTAINER_H
#define CONTAINER_H

#include "obj/item.h"

struct container
{
	struct item item;
};

struct container *container_new(uint64_t guid);

extern const struct object_vtable container_object_vtable;

#endif
