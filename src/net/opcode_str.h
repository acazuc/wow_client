#ifndef NET_OPCODE_STR_H
#define NET_OPCODE_STR_H

#include "opcode.h"

static const char *net_opcodes_str[OPCODE_MAX + 1] =
{
#define OPCODE_DEFINE(opcode, val) [opcode] = #opcode,
# include "opcode_def.h"
#undef OPCODE_DEFINE
};

#endif
