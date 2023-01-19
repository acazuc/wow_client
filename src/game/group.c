#include "group.h"

#include "itf/interface.h"

#include "wow_lua.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

struct group *group_new(void)
{
	struct group *group = mem_zalloc(MEM_GENERIC, sizeof(*group));
	if (!group)
	{
		LOG_ERROR("allocation failed");
		return NULL;
	}
	return group;
}

void group_delete(struct group *group)
{
	if (!group)
		return;
	group_clear_members(group);
	mem_free(MEM_GENERIC, group->invite);
	mem_free(MEM_GENERIC, group);
}

void group_clear_members(struct group *group)
{
	for (size_t i = 0; i < group->members_count; ++i)
		mem_free(MEM_GENERIC, group->members[i].name);
	group->members_count = 0;
}

bool group_set_invite(struct group *group, const char *name)
{
	mem_free(MEM_GENERIC, group->invite);
	group->invite = mem_strdup(MEM_GENERIC, name);
	if (!group->invite)
		return false;
	lua_pushnil(g_wow->interface->L);
	lua_pushstring(g_wow->interface->L, group->invite);
	interface_execute_event(g_wow->interface, EVENT_PARTY_INVITE_REQUEST, 1);
	return true;
}
