#include "net/packets.h"

#include "itf/interface.h"

#include "net/world_socket.h"
#include "net/network.h"
#include "net/packet.h"
#include "net/opcode.h"

#include "memory.h"
#include "log.h"
#include "wow.h"

#include <jkssl/sha1.h>
#include <jkssl/hmac.h>

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

bool net_smsg_auth_challenge(struct net_packet_reader *packet)
{
	uint32_t server_salt;
	uint32_t client_salt = rand();
	uint32_t osef = 0;
	uint8_t hash[SHA_DIGEST_LENGTH];
	struct sha1_ctx ctx;
	if (!net_read_u32(packet, &server_salt))
		return false;
	if (!sha1_init(&ctx))
	{
		LOG_ERROR("SHA1_Init() failed");
		return false;
	}
	if (!sha1_update(&ctx, g_wow->network->username, strlen(g_wow->network->username)))
	{
		LOG_ERROR("SHA1_Update() failed");
		return false;
	}
	if (!sha1_update(&ctx, &osef, sizeof(osef)))
	{
		LOG_ERROR("SHA1_Update() failed");
		return false;
	}
	if (!sha1_update(&ctx, &client_salt, sizeof(client_salt)))
	{
		LOG_ERROR("SHA1_Update() failed");
		return false;
	}
	if (!sha1_update(&ctx, &server_salt, sizeof(server_salt)))
	{
		LOG_ERROR("SHA1_Update() failed");
		return false;
	}
	if (!sha1_update(&ctx, g_wow->network->auth_key, sizeof(g_wow->network->auth_key)))
	{
		LOG_ERROR("SHA1_Update() failed");
		return false;
	}
	if (!sha1_final(hash, &ctx))
	{
		LOG_ERROR("SHA1_Final() failed");
		return false;
	}
	struct net_packet_writer answer;
	net_packet_writer_init(&answer, CMSG_AUTH_SESSION);
	if (!net_write_u32(&answer, 8606)
	 || !net_write_u32(&answer, 0)
	 || !net_write_str(&answer, g_wow->network->username)
	 || !net_write_u32(&answer, client_salt)
	 || !net_write_bytes(&answer, hash, sizeof(hash))
	 || !net_write_u32(&answer, 0)) /* addons bytes */
		return false;
	if (!net_send_packet(g_wow->network, &answer))
		return false;
	net_packet_writer_destroy(&answer);
	static const uint8_t iv[] = {0x38, 0xA7, 0x83, 0x15, 0xF8, 0x92, 0x25, 0x30, 0x71, 0x98, 0x67, 0xB1, 0x8C, 0x4, 0xE2, 0xAA};
	uint8_t session_key[SHA_DIGEST_LENGTH];
	if (!hmac(evp_sha1(), iv, sizeof(iv), g_wow->network->auth_key, sizeof(g_wow->network->auth_key), session_key, NULL))
	{
		LOG_ERROR("hmac failed");
		return false;
	}
	net_world_socket_init_cipher(g_wow->network->world_socket, session_key);
	return true;
}

bool net_smsg_auth_response(struct net_packet_reader *packet)
{
	uint8_t response;
	if (!net_read_u8(packet, &response))
		return false;
	switch (response)
	{
		case 0xC: /* AUTH_OK */
		{
			uint32_t remaining_billing;
			uint32_t rested_billing;
			uint8_t unk;
			uint8_t expansion;
			if (!net_read_u32(packet, &remaining_billing)
			 || !net_read_u32(packet, &rested_billing)
			 || !net_read_u8(packet, &unk)
			 || !net_read_u8(packet, &expansion))
				return false;
			struct net_packet_writer answer;
			net_packet_writer_init(&answer, CMSG_CHAR_ENUM);
			if (!net_send_packet(g_wow->network, &answer))
				return false;
			net_packet_writer_destroy(&answer);
			break;
		}
		default:
			LOG_ERROR("invalid auth response: %" PRIu8, response);
			return false;
	}
	return true;
}

bool net_smsg_char_enum(struct net_packet_reader *packet)
{
	uint8_t characters;
	if (!net_read_u8(packet, &characters))
		return false;
	for (size_t i = 0; i < characters; ++i)
	{
		struct login_character character;
		if (!net_read_u64(packet, &character.guid)
		 || !net_read_str(packet, (const char**)&character.name)
		 || !net_read_u8(packet, &character.race_type)
		 || !net_read_u8(packet, &character.class_type)
		 || !net_read_u8(packet, &character.gender)
		 || !net_read_u8(packet, &character.skin_color)
		 || !net_read_u8(packet, &character.face)
		 || !net_read_u8(packet, &character.hair_style)
		 || !net_read_u8(packet, &character.hair_color)
		 || !net_read_u8(packet, &character.facial_hair)
		 || !net_read_u8(packet, &character.level)
		 || !net_read_u32(packet, &character.zone)
		 || !net_read_u32(packet, &character.map)
		 || !net_read_flt(packet, &character.x)
		 || !net_read_flt(packet, &character.y)
		 || !net_read_flt(packet, &character.z)
		 || !net_read_u32(packet, &character.guild)
		 || !net_read_u32(packet, &character.flags)
		 || !net_read_u8(packet, &character.first_login)
		 || !net_read_u32(packet, &character.pet_display)
		 || !net_read_u32(packet, &character.pet_level)
		 || !net_read_u32(packet, &character.pet_family))
			return false;
		for (size_t j = 0; j < 19; ++j)
		{
			if (!net_read_u32(packet, &character.items[j].display_id)
			 || !net_read_u8(packet, &character.items[j].inventory_type)
			 || !net_read_u32(packet, &character.items[j].aura))
				return false;
		}
		if (!net_read_u32(packet, &character.bag_display_id)
		 || !net_read_u8(packet, &character.bag_inventory_type)
		 || !net_read_u32(packet, &character.enchant))
			return false;
		character.name = mem_strdup(MEM_NET, character.name);
		if (!character.name)
			return false;
		if (!net_world_socket_add_character(g_wow->network->world_socket, &character))
			return false;
	}
	interface_execute_event(g_wow->interface, EVENT_CHARACTER_LIST_UPDATE, 0);
	return true;
}

bool net_smsg_character_login_failed(struct net_packet_reader *packet)
{
	uint8_t error;
	if (!net_read_u8(packet, &error))
		return false;
	LOG_ERROR("error: %" PRIu8, error);
	return true;
}
