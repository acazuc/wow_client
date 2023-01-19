#ifndef DYNOBJ_H
#define DYNOBJ_H

#include "obj/object.h"

struct dynobj
{
	struct object object;
};

struct dynobj *dynobj_new(uint64_t guid);

extern const struct object_vtable dynobj_object_vtable;

#endif
