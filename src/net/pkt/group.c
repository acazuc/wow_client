#include "game/group.h"

#include "itf/interface.h"
#include "itf/enum.h"

#include "net/packets.h"
#include "net/packet.h"
#include "net/opcode.h"

#include "memory.h"
#include "log.h"
#include "wow.h"

#include <inttypes.h>
#include <string.h>

bool net_smsg_group_list(struct net_packet_reader *packet)
{
	uint8_t group_type, bg_group, group_id, assistance;
	uint64_t guid;
	uint32_t members_count;
	uint64_t leader;
	uint8_t loot_method;
	uint64_t loot_master;
	uint8_t loot_quality;
	uint8_t difficulty;
	struct
	{
		const char *name;
		uint64_t guid;
		uint8_t status;
		uint8_t group_id;
		uint8_t flags;
	} members[39];
	if (!net_read_u8(packet, &group_type)
	 || !net_read_u8(packet, &bg_group)
	 || !net_read_u8(packet, &group_id)
	 || !net_read_u8(packet, &assistance)
	 || !net_read_u64(packet, &guid)
	 || !net_read_u32(packet, &members_count))
		return false;
	if (members_count > 39)
	{
		LOG_ERROR("too much group members: %" PRIu32, members_count);
		return false;
	}
	for (uint32_t i = 0; i < members_count; ++i)
	{
		if (!net_read_str(packet, &members[i].name)
		 || !net_read_u64(packet, &members[i].guid)
		 || !net_read_u8(packet, &members[i].status)
		 || !net_read_u8(packet, &members[i].group_id)
		 || !net_read_u8(packet, &members[i].flags))
			return false;
	}
	if (!net_read_u64(packet, &leader)
	 || !net_read_u8(packet, &loot_method)
	 || !net_read_u64(packet, &loot_master)
	 || !net_read_u8(packet, &loot_quality)
	 || !net_read_u8(packet, &difficulty))
		return false;
	g_wow->group->in_group = true;
	g_wow->group->guid = guid;
	g_wow->group->leader = leader;
	g_wow->group->loot_method = loot_method;
	g_wow->group->loot_master = loot_master;
	g_wow->group->loot_quality = loot_quality;
	g_wow->group->difficulty = difficulty;
	group_clear_members(g_wow->group);
	g_wow->group->members_count = 0;
	for (size_t i = 0; i < members_count; ++i)
	{
		g_wow->group->members[i].guid = members[i].guid;
		g_wow->group->members[i].name = mem_strdup(MEM_GENERIC, members[i].name);
		if (!g_wow->group->members[i].name)
		{
			LOG_ERROR("allocation failure");
			group_delete(g_wow->group);
			g_wow->group = NULL;
			return false;
		}
		g_wow->group->members[i].status = members[i].status;
		g_wow->group->members[i].group_id = members[i].group_id;
		g_wow->group->members[i].flags = members[i].flags;
		g_wow->group->members_count++;
	}
	interface_execute_event(g_wow->interface, EVENT_PARTY_MEMBERS_CHANGED, 0);
	return true;
}

bool net_smsg_group_destroyed(struct net_packet_reader *packet)
{
	(void)packet;
	g_wow->group->in_group = false;
	group_clear_members(g_wow->group);
	return true;
}

bool net_smsg_group_invite(struct net_packet_reader *packet)
{
	const char *name;
	if (!net_read_str(packet, &name))
		return false;
	if (!group_set_invite(g_wow->group, name))
		return false;
	return true;
}

bool net_cmsg_group_accept(struct net_packet_writer *packet)
{
	net_packet_writer_init(packet, CMSG_GROUP_ACCEPT);
	return true;
}

bool net_cmsg_group_decline(struct net_packet_writer *packet)
{
	net_packet_writer_init(packet, CMSG_GROUP_DECLINE);
	return true;
}

bool net_smsg_party_member_stats(struct net_packet_reader *packet)
{
	/* XXX */
	return true;
}
