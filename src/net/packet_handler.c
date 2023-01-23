#include "packet_handler.h"

#include "itf/interface.h"

#include "net/world_socket.h"
#include "net/opcode_str.h"
#include "net/packets.h"
#include "net/network.h"
#include "net/packet.h"
#include "net/opcode.h"

#include "memory.h"
#include "const.h"
#include "log.h"
#include "wow.h"
#include "db.h"

#include <inttypes.h>
#include <string.h>

MEMORY_DECL(NET);

bool net_packet_handler_init(struct net_packet_handler *handler)
{
	jks_array_init(&handler->handles, sizeof(struct net_packet_handle), NULL, &jks_array_memory_fn_NET);
	if (!jks_array_resize(&handler->handles, OPCODE_MAX))
	{
		LOG_ERROR("failed to resize handlers");
		return false;
	}
	for (size_t i = 0; i < OPCODE_MAX; ++i)
		JKS_ARRAY_GET(&handler->handles, i, struct net_packet_handle)->handle = NULL;

#define SET_HANDLE(opc, fn) \
	do \
	{ \
		struct net_packet_handle *handle = JKS_ARRAY_GET(&handler->handles, opc, struct net_packet_handle); \
		handle->opcode = opc; \
		handle->name = #opc; \
		handle->handle = fn; \
	} while (0)

	SET_HANDLE(SMSG_AUTH_CHALLENGE, net_smsg_auth_challenge);
	SET_HANDLE(SMSG_AUTH_RESPONSE, net_smsg_auth_response);
	SET_HANDLE(SMSG_CHAR_ENUM, net_smsg_char_enum);
	SET_HANDLE(SMSG_CHARACTER_LOGIN_FAILED, net_smsg_character_login_failed);
	SET_HANDLE(SMSG_LOGIN_VERIFY_WORLD, net_smsg_login_verify_world);
	SET_HANDLE(SMSG_ACCOUNT_DATA_TIMES, net_smsg_account_data_times);
	SET_HANDLE(SMSG_FEATURE_SYSTEM_STATUS, net_smsg_feature_system_status);
	SET_HANDLE(SMSG_SET_REST_START, net_smsg_set_rest_start);
	SET_HANDLE(SMSG_BINDPOINTUPDATE, net_smsg_bindpointupdate);
	SET_HANDLE(SMSG_ACTION_BUTTONS, net_smsg_action_buttons);
	SET_HANDLE(SMSG_INITIALIZE_FACTIONS, net_smsg_initialize_factions);
	SET_HANDLE(SMSG_LOGIN_SETTIMESPEED, net_smsg_login_settimespeed);
	SET_HANDLE(SMSG_INIT_WORLD_STATES, net_smsg_init_world_states);
	SET_HANDLE(SMSG_TUTORIAL_FLAGS, net_smsg_tutorial_flags);
	SET_HANDLE(SMSG_INITIAL_SPELLS, net_smsg_initial_spells);
	SET_HANDLE(SMSG_TIME_SYNC_REQ, net_smsg_time_sync_req);
	SET_HANDLE(SMSG_UPDATE_OBJECT, net_smsg_update_object);
	SET_HANDLE(SMSG_DESTROY_OBJECT, net_smsg_destroy_object);
	SET_HANDLE(SMSG_CONTACT_LIST, net_smsg_contact_list);
	SET_HANDLE(SMSG_NAME_QUERY_RESPONSE, net_smsg_name_query_response);
	SET_HANDLE(SMSG_QUESTGIVER_STATUS, net_smsg_questgiver_status);
	SET_HANDLE(SMSG_FRIEND_STATUS, net_smsg_friend_status);
	SET_HANDLE(SMSG_GROUP_LIST, net_smsg_group_list);
	SET_HANDLE(SMSG_GROUP_DESTROYED, net_smsg_group_destroyed);
	SET_HANDLE(SMSG_CREATURE_QUERY_RESPONSE, net_smsg_creature_query_response);
	SET_HANDLE(SMSG_ITEM_QUERY_SINGLE_RESPONSE, net_smsg_item_query_single_response);
	SET_HANDLE(SMSG_GROUP_INVITE, net_smsg_group_invite);
	SET_HANDLE(SMSG_PARTY_MEMBER_STATS, net_smsg_party_member_stats);
	SET_HANDLE(SMSG_GUILD_QUERY_RESPONSE, net_smsg_guild_query_response);
	SET_HANDLE(SMSG_MOTD, net_smsg_motd);
	SET_HANDLE(MSG_MOVE_START_FORWARD, net_smsg_move);
	SET_HANDLE(MSG_MOVE_START_BACKWARD, net_smsg_move);
	SET_HANDLE(MSG_MOVE_STOP, net_smsg_move);
	SET_HANDLE(MSG_MOVE_START_STRAFE_LEFT, net_smsg_move);
	SET_HANDLE(MSG_MOVE_START_STRAFE_RIGHT, net_smsg_move);
	SET_HANDLE(MSG_MOVE_STOP_STRAFE, net_smsg_move);
	SET_HANDLE(MSG_MOVE_JUMP, net_smsg_move);
	SET_HANDLE(MSG_MOVE_START_TURN_LEFT, net_smsg_move);
	SET_HANDLE(MSG_MOVE_START_TURN_RIGHT, net_smsg_move);
	SET_HANDLE(MSG_MOVE_STOP_TURN, net_smsg_move);
	SET_HANDLE(MSG_MOVE_START_PITCH_UP, net_smsg_move);
	SET_HANDLE(MSG_MOVE_START_PITCH_DOWN, net_smsg_move);
	SET_HANDLE(MSG_MOVE_STOP_PITCH, net_smsg_move);
	SET_HANDLE(MSG_MOVE_SET_RUN_MODE, net_smsg_move);
	SET_HANDLE(MSG_MOVE_SET_WALK_MODE, net_smsg_move);
	SET_HANDLE(MSG_MOVE_FALL_LAND, net_smsg_move);
	SET_HANDLE(MSG_MOVE_START_SWIM, net_smsg_move);
	SET_HANDLE(MSG_MOVE_STOP_SWIM, net_smsg_move);
	SET_HANDLE(MSG_MOVE_SET_FACING, net_smsg_move);
	SET_HANDLE(MSG_MOVE_SET_PITCH, net_smsg_move);
	SET_HANDLE(MSG_MOVE_HEARTBEAT, net_smsg_move);
	SET_HANDLE(SMSG_MESSAGECHAT, net_smsg_messagechat);

#undef SET_HANDLE

	return true;
}

void net_packet_handler_destroy(struct net_packet_handler *handler)
{
	jks_array_destroy(&handler->handles);
}

bool net_packet_handle(struct net_packet_handler *handler, struct net_packet_reader *packet)
{
	if (packet->opcode >= handler->handles.size)
	{
		LOG_ERROR("unsupported opcode: %" PRIx32, packet->opcode);
		return false;
	}
	struct net_packet_handle *handle = JKS_ARRAY_GET(&handler->handles, packet->opcode, struct net_packet_handle);
	if (!handle->handle)
	{
		LOG_ERROR("unhandled %s", net_opcodes_str[packet->opcode]);
		return false;
	}
	bool ret = handle->handle(packet);
	if (packet->pos != packet->size)
		LOG_WARN("packet read not complete (%u / %u)", (unsigned)packet->pos, (unsigned)packet->size);
	return ret;
}
