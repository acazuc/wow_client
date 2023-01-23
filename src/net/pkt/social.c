#include "itf/interface.h"

#include "game/social.h"

#include "net/packets.h"
#include "net/packet.h"
#include "net/opcode.h"

#include "memory.h"
#include "const.h"
#include "log.h"
#include "wow.h"

#include <inttypes.h>
#include <string.h>

bool net_cmsg_contact_list(struct net_packet_writer *packet, uint32_t flags)
{
	net_packet_writer_init(packet, CMSG_CONTACT_LIST);
	if (!net_write_u32(packet, flags))
		return false;
	return true;
}

bool net_smsg_contact_list(struct net_packet_reader *packet)
{
	uint32_t flags;
	uint32_t size;
	if (!net_read_u32(packet, &flags)
	 || !net_read_u32(packet, &size))
		return false;
	for (size_t i = 0; i < size; ++i)
	{
		uint64_t guid;
		uint32_t type;
		const char *note;
		if (!net_read_u64(packet, &guid)
		 || !net_read_u32(packet, &type)
		 || !net_read_str(packet, &note))
			return false;
		switch (type)
		{
			case 0x1: /* friend */
			{
				struct social_friend friend_s;
				uint8_t status;
				int32_t zone;
				int32_t level;
				int32_t class_type;
				if (!net_read_u8(packet, &status))
					return false;
				if (status)
				{
					if (!net_read_i32(packet, &zone)
					 || !net_read_i32(packet, &level)
					 || !net_read_i32(packet, &class_type))
						return false;
				}
				else
				{
					zone = 0;
					level = 0;
					class_type = 0;
				}
				friend_s.guid = guid;
				friend_s.zone = zone;
				friend_s.level = level;
				friend_s.class_type = class_type;
				friend_s.status = status;
				friend_s.note = note ? mem_strdup(MEM_GENERIC, note) : NULL;
				if (note && !friend_s.note)
				{
					LOG_ERROR("strdup failed");
					return false;
				}
				social_add_friend(g_wow->social, &friend_s);
				break;
			}
			case 0x2: /* ignore */
			{
				struct social_ignore ignore;
				ignore.guid = guid;
				social_add_ignore(g_wow->social, &ignore);
				break;
			}
			case 0x4: /* mute */
				break;
			default:
				LOG_ERROR("unknown contact type: %" PRIu32, type);
				return false;
		}
	}
	g_wow->social->loaded = true;
	interface_execute_event(g_wow->interface, EVENT_FRIENDLIST_SHOW, 0);
	return true;
}

bool net_smsg_friend_status(struct net_packet_reader *packet)
{
	uint8_t state;
	if (!net_read_u8(packet, &state))
		return false;
	switch (state)
	{
		case FRIEND_RESULT_ONLINE:
		{
			uint64_t guid;
			uint8_t online;
			int32_t zone, level, class_type;
			if (!net_read_u64(packet, &guid)
			 || !net_read_u8(packet, &online)
			 || !net_read_i32(packet, &zone)
			 || !net_read_i32(packet, &level)
			 || !net_read_i32(packet, &class_type))
				return false;
			if (g_wow->interface)
				interface_execute_event(g_wow->interface, EVENT_FRIENDLIST_UPDATE, 0);
			break;
		}
		case FRIEND_RESULT_OFFLINE:
		{
			uint64_t guid;
			if (!net_read_u64(packet, &guid))
				return false;
			if (g_wow->interface)
				interface_execute_event(g_wow->interface, EVENT_FRIENDLIST_UPDATE, 0);
			break;
		}
		case FRIEND_RESULT_ADDED_ONLINE:
		{
			uint64_t guid;
			const char *note;
			uint8_t status;
			int32_t zone, level, class_type;
			if (!net_read_u64(packet, &guid)
			 || !net_read_str(packet, &note)
			 || !net_read_u8(packet, &status)
			 || !net_read_i32(packet, &zone)
			 || !net_read_i32(packet, &level)
			 || !net_read_i32(packet, &class_type))
				return false;
			struct social_friend friend;
			friend.guid = guid;
			friend.level = level;
			friend.class_type = class_type;
			friend.status = status;
			friend.zone = zone;
			friend.note = mem_strdup(MEM_GENERIC, note); /* XXX */
			if (!friend.note)
			{
				LOG_ERROR("allocation failed");
				break;
			}
			if (!social_add_friend(g_wow->social, &friend))
			{
				LOG_ERROR("can't add friend");
				mem_free(MEM_GENERIC, friend.note);
				break;
			}
			if (g_wow->interface)
				interface_execute_event(g_wow->interface, EVENT_FRIENDLIST_UPDATE, 0);
			/* ERR_FRIEND_ADDED_S */
			break;
		}
		case FRIEND_RESULT_ADDED_OFFLINE:
		{
			uint64_t guid;
			const char *note;
			if (!net_read_u64(packet, &guid)
			 || !net_read_str(packet, &note))
				return false;
			struct social_friend friend;
			friend.guid = guid;
			friend.level = 0;
			friend.class_type = 0;
			friend.status = 0;
			friend.zone = 0;
			friend.note = mem_strdup(MEM_GENERIC, note);
			if (!friend.note)
			{
				LOG_ERROR("allocation failed");
				break;
			}
			if (!social_add_friend(g_wow->social, &friend))
			{
				LOG_ERROR("can't add friend");
				mem_free(MEM_GENERIC, friend.note);
				break;
			}
			if (g_wow->interface)
				interface_execute_event(g_wow->interface, EVENT_FRIENDLIST_UPDATE, 0);
			/* ERR_FRIEND_ADDED_S */
			break;
		}
		case FRIEND_RESULT_SELF:
		{
			uint64_t guid;
			if (!net_read_u64(packet, &guid))
				return false;
			/* ERR_FRIEND_SELF */
			break;
		}
		case FRIEND_RESULT_ENEMY:
			break;
		case FRIEND_RESULT_ALREADY:
		{
			uint64_t guid;
			const char *name;
			if (!net_read_u64(packet, &guid)
			 || !net_read_str(packet, &name))
				return false;
			/* ERR_FRIEND_ALREADY_S */
			break;
		}
		case FRIEND_RESULT_DB_ERROR:
			/* ERR_FRIEND_DB_ERROR */
			break;
		case FRIEND_RESULT_LIST_FULL:
			/* ERR_FRIEND_LIST_FULL */
			break;
		case FRIEND_RESULT_NOT_FOUND:
			/* ERR_FRIEND_NOT_FOUND */
			break;
		case FRIEND_RESULT_REMOVED:
		{
			uint64_t guid;
			if (!net_read_u64(packet, &guid))
				return false;
			if (!social_remove_friend(g_wow->social, guid))
			{
				LOG_ERROR("can't remove friend");
				break;
			}
			if (g_wow->interface)
				interface_execute_event(g_wow->interface, EVENT_FRIENDLIST_UPDATE, 0);
			/* ERR_FRIEND_REMOVED_S */
			break;
		}
		case FRIEND_RESULT_IGNORE_ADDED:
		{
			uint64_t guid;
			if (!net_read_u64(packet, &guid))
				return false;
			struct social_ignore ignore;
			ignore.guid = guid;
			if (!social_add_ignore(g_wow->social, &ignore))
			{
				LOG_ERROR("can't add ignore");
				break;
			}
			if (g_wow->interface)
				interface_execute_event(g_wow->interface, EVENT_FRIENDLIST_UPDATE, 0);
			/* ERR_IGNORE_ADDED_S */
			break;
		}
		case FRIEND_RESULT_IGNORE_FULL:
			/* ERR_IGNORE_FULL */
			break;
		case FRIEND_RESULT_IGNORE_SELF:
			/* ERR_IGNORE_SELF */
			break;
		case FRIEND_RESULT_IGNORE_NOT_FOUND:
			/* ERR_IGNORE_NOT_FOUND */
			break;
		case FRIEND_RESULT_IGNORE_ALREADY:
		{
			uint64_t guid;
			const char *name;
			if (!net_read_u64(packet, &guid)
			 || !net_read_str(packet, &name))
				return false;
			/* ERR_IGNORE_ALREADY_S */
			break;
		}
		case FRIEND_RESULT_IGNORE_REMOVED:
		{
			uint64_t guid;
			if (!net_read_u64(packet, &guid))
				return false;
			if (!social_remove_ignore(g_wow->social, guid))
			{
				LOG_ERROR("can't remove ignore");
				break;
			}
			if (g_wow->interface)
				interface_execute_event(g_wow->interface, EVENT_FRIENDLIST_UPDATE, 0);
			/* ERR_IGNORE_REMOVED_S */
			break;
		}
	}
	return true;
}

bool net_cmsg_add_friend(struct net_packet_writer *packet, const char *name, const char *note)
{
	net_packet_writer_init(packet, CMSG_ADD_FRIEND);
	if (!net_write_str(packet, name)
	 || !net_write_str(packet, note))
		return false;
	return true;
}

bool net_cmsg_del_friend(struct net_packet_writer *packet, uint64_t guid)
{
	net_packet_writer_init(packet, CMSG_DEL_FRIEND);
	if (!net_write_u64(packet, guid))
		return false;
	return true;
}

bool net_cmsg_set_contact_notes(struct net_packet_writer *packet, uint64_t guid, const char *note)
{
	net_packet_writer_init(packet, CMSG_SET_CONTACT_NOTES);
	if (!net_write_u64(packet, guid)
	 || !net_write_str(packet, note))
		return false;
	return true;
}

bool net_cmsg_add_ignore(struct net_packet_writer *packet, const char *name)
{
	net_packet_writer_init(packet, CMSG_ADD_IGNORE);
	if (!net_write_str(packet, name))
		return false;
	return true;
}

bool net_cmsg_del_ignore(struct net_packet_writer *packet, uint64_t guid)
{
	net_packet_writer_init(packet, CMSG_DEL_IGNORE);
	if (!net_write_u64(packet, guid))
		return false;
	return true;
}
