#ifndef NET_OPCODE_H
#define NET_OPCODE_H

enum net_opcode
{
#define OPCODE_DEFINE(opcode, val) opcode = val,
# include "opcode_def.h"
#undef OPCODE_DEFINE
};

#endif
