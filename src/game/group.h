#ifndef GROUP_H
#define GROUP_H

#include "const.h"

#include <stdbool.h>
#include <stdint.h>

struct group_member
{
	uint64_t guid;
	char *name;
	uint8_t status;
	uint8_t group_id;
	uint8_t flags;
};

struct group
{
	struct group_member members[39];
	uint32_t members_count;
	enum party_loot_method loot_method;
	uint64_t loot_master;
	uint64_t leader;
	uint64_t guid;
	uint8_t loot_quality;
	uint8_t difficulty;
	char *invite;
	bool in_group;
};

struct group *group_new(void);
void group_delete(struct group *group);
void group_clear_members(struct group *group);
bool group_set_invite(struct group *group, const char *name);

#endif
