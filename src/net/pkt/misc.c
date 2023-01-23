#include "net/packets.h"
#include "net/packet.h"
#include "net/opcode.h"

#include "itf/interface.h"

#include "wow_lua.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <jks/array.h>

MEMORY_DECL(NET);
MEMORY_DECL(GENERIC);

bool net_smsg_login_verify_world(struct net_packet_reader *packet)
{
	uint32_t mapid;
	float x;
	float y;
	float z;
	float w;
	if (!net_read_u32(packet, &mapid)
	 || !net_read_flt(packet, &x)
	 || !net_read_flt(packet, &y)
	 || !net_read_flt(packet, &z)
	 || !net_read_flt(packet, &w))
		return false;
	g_wow->interface->switch_framescreen = true;
	return true;
}

bool net_smsg_account_data_times(struct net_packet_reader *packet)
{
	for (uint32_t i = 0; i < 32; ++i)
	{
		uint32_t data_time;
		if (!net_read_u32(packet, &data_time))
			return false;
	}
	return true;
}

bool net_smsg_feature_system_status(struct net_packet_reader *packet)
{
	uint8_t unk;
	uint8_t voice_enabled;
	if (!net_read_u8(packet, &unk)
	 || !net_read_u8(packet, &voice_enabled))
		return false;
	return true;
}

bool net_smsg_set_rest_start(struct net_packet_reader *packet)
{
	uint32_t rest_start;
	if (!net_read_u32(packet, &rest_start))
		return false;
	return true;
}

bool net_smsg_bindpointupdate(struct net_packet_reader *packet)
{
	float x;
	float y;
	float z;
	uint32_t map;
	uint32_t area;
	if (!net_read_flt(packet, &x)
	 || !net_read_flt(packet, &y)
	 || !net_read_flt(packet, &z)
	 || !net_read_u32(packet, &map)
	 || !net_read_u32(packet, &area))
		return false;
	return true;
}

bool net_smsg_action_buttons(struct net_packet_reader *packet)
{
	for (uint32_t i = 0; i < 132; ++i)
	{
		uint32_t action;
		if (!net_read_u32(packet, &action))
			return false;
	}
	return true;
}

bool net_smsg_initialize_factions(struct net_packet_reader *packet)
{
	uint32_t factions;
	if (!net_read_u32(packet, &factions))
		return false;
	for (uint32_t i = 0; i < factions; ++i)
	{
		uint8_t flags;
		uint32_t reputation;
		if (!net_read_u8(packet, &flags)
		 || !net_read_u32(packet, &reputation))
			return false;
	}
	return true;
}

bool net_smsg_login_settimespeed(struct net_packet_reader *packet)
{
	uint32_t bitfield;
	float timespeed;
	if (!net_read_u32(packet, &bitfield)
	 || !net_read_flt(packet, &timespeed))
		return false;
	return true;
}

bool net_smsg_init_world_states(struct net_packet_reader *packet)
{
	uint32_t map;
	uint32_t unk1;
	uint32_t unk2;
	uint16_t unk3;
	if (!net_read_u32(packet, &map)
	 || !net_read_u32(packet, &unk1)
	 || !net_read_u32(packet, &unk2)
	 || !net_read_u16(packet, &unk3))
		return false;
	wow_set_map(map);
	return true;
}

bool net_smsg_tutorial_flags(struct net_packet_reader *packet)
{
	for (size_t i = 0; i < 8; ++i)
	{
		uint32_t flags;
		if (!net_read_u32(packet, &flags))
			return false;
	}
	return true;
}

bool net_smsg_initial_spells(struct net_packet_reader *packet)
{
	uint8_t unk;
	uint16_t spell_count;
	uint16_t spell_id1;
	uint16_t spell_unk1;
	uint16_t spell_id2;
	uint16_t spell_unk2;
	uint16_t cooldown_count;
	if (!net_read_u8(packet, &unk)
	 || !net_read_u16(packet, &spell_count)
	 || !net_read_u16(packet, &spell_id1)
	 || !net_read_u16(packet, &spell_unk1)
	 || !net_read_u16(packet, &spell_id2)
	 || !net_read_u16(packet, &spell_unk2)
	 || !net_read_u16(packet, &cooldown_count))
		return false;
	return true;
}

bool net_smsg_time_sync_req(struct net_packet_reader *packet)
{
	uint32_t unk;
	if (!net_read_u32(packet, &unk))
		return false;
	return true;
}

bool net_smsg_questgiver_status(struct net_packet_reader *packet)
{
	uint64_t guid;
	uint8_t status;
	if (!net_read_u64(packet, &guid)
	 || !net_read_u8(packet, &status))
		return false;
	return true;
}

static void motd_destroy(void *ptr)
{
	mem_free(MEM_GENERIC, *(char**)ptr);
}

bool net_smsg_motd(struct net_packet_reader *packet)
{
	struct jks_array lines;
	uint32_t nb;
	if (!net_read_u32(packet, &nb))
		return false;
	jks_array_init(&lines, sizeof(char*), motd_destroy, &jks_array_memory_fn_GENERIC);
	if (!jks_array_reserve(&lines, nb))
	{
		LOG_ERROR("failed to resize MOTD array");
		return false;
	}
	for (uint32_t i = 0; i < nb; ++i)
	{
		const char *line;
		if (!net_read_str(packet, &line))
		{
			jks_array_destroy(&lines);
			return false;
		}
		char *dup = mem_strdup(MEM_GENERIC, line);
		if (!dup)
		{
			jks_array_destroy(&lines);
			return false;
		}
		jks_array_push_back(&lines, &dup);
	}
	jks_array_destroy(&lines);
	return true;
}

bool net_cmsg_messagechat(struct net_packet_writer *packet, uint32_t channel, uint32_t lang, const char *target, const char *message)
{
	net_packet_writer_init(packet, CMSG_MESSAGECHAT);
	if (!net_write_u32(packet, channel)
	 || !net_write_u32(packet, lang)
	 || (target && !net_write_str(packet, target))
	 || !net_write_str(packet, message))
		return false;
	return true;
}

bool net_smsg_messagechat(struct net_packet_reader *packet)
{
	uint8_t channel;
	uint32_t lang;
	if (!net_read_u8(packet, &channel)
	 || !net_read_u32(packet, &lang))
		return false;
	if (channel == CHAT_MSG_MONSTER_SAY || channel == CHAT_MSG_MONSTER_YELL || channel == CHAT_MSG_MONSTER_EMOTE)
	{
		const char *name;
		uint64_t guid;
		const char *msg;
		uint8_t unk;
		if (!net_read_str(packet, &name)
		 || !net_read_u64(packet, &guid)
		 || !net_read_str(packet, &msg)
		 || !net_read_u8(packet, &unk))
			return false;
		/* XXX */
	}
	else
	{
		uint64_t author;
		uint32_t unk;
		if (!net_read_u64(packet, &author)
		 || !net_read_u32(packet, &unk))
			return false;
		if (channel == CHAT_MSG_CHANNEL)
		{
			const char *channel_name;
			if (!net_read_str(packet, &channel_name))
				return false;
		}
		uint64_t target;
		uint32_t msg_size;
		const char *msg;
		uint8_t flags;
		if (!net_read_u64(packet, &target)
		 || !net_read_u32(packet, &msg_size)
		 || !net_read_str(packet, &msg)
		 || !net_read_u8(packet, &flags))
			return false;
		lua_pushnil(g_wow->interface->L);
		lua_pushstring(g_wow->interface->L, msg);
		lua_pushnil(g_wow->interface->L);
		lua_pushstring(g_wow->interface->L, "player");
		lua_pushnil(g_wow->interface->L);
		lua_pushstring(g_wow->interface->L, "Common");
		lua_pushnil(g_wow->interface->L);
		lua_pushstring(g_wow->interface->L, "2. Trade - City");
		lua_pushnil(g_wow->interface->L);
		lua_pushstring(g_wow->interface->L, "");
		lua_pushnil(g_wow->interface->L);
		lua_pushstring(g_wow->interface->L, "");
		lua_pushnil(g_wow->interface->L);
		lua_pushnumber(g_wow->interface->L, 1); /* zone channelID */
		lua_pushnil(g_wow->interface->L);
		lua_pushnumber(g_wow->interface->L, 0); /* channel index */
		lua_pushnil(g_wow->interface->L);
		lua_pushstring(g_wow->interface->L, "Trade - City"); /* base name */
		lua_pushnil(g_wow->interface->L);
		lua_pushnumber(g_wow->interface->L, 0); /* language */
		lua_pushnil(g_wow->interface->L);
		lua_pushnumber(g_wow->interface->L, 0); /* lineID */
		interface_execute_event(g_wow->interface, EVENT_CHAT_MSG_SAY, 11);
	}
	return true;
}
