#include "social.h"

#include "net/packets.h"
#include "net/network.h"
#include "net/packet.h"
#include "net/opcode.h"

#include "memory.h"
#include "log.h"
#include "wow.h"
#include "db.h"

MEMORY_DECL(GENERIC);

static void friend_dtr(void *ptr)
{
	struct social_friend *friend_s = ptr;
	mem_free(MEM_GENERIC, friend_s->note);
}

static void ignore_dtr(void *ptr)
{
	struct social_ignore *ignore = ptr;
	(void)ignore;
}

struct social *social_new(void)
{
	struct social *social = mem_malloc(MEM_GENERIC, sizeof(*social));
	if (!social)
	{
		LOG_ERROR("malloc failed");
		return NULL;
	}
	social->loaded = false;
	jks_array_init(&social->friends, sizeof(struct social_friend), friend_dtr, &jks_array_memory_fn_GENERIC);
	jks_array_init(&social->ignores, sizeof(struct social_ignore), ignore_dtr, &jks_array_memory_fn_GENERIC);
	social->selected_friend = 1;
	social->selected_ignore = 1;
	return social;
}

void social_delete(struct social *social)
{
	if (!social)
		return;
	jks_array_destroy(&social->friends);
	jks_array_destroy(&social->ignores);
	mem_free(MEM_GENERIC, social);
}

bool social_net_load(struct social *social)
{
	bool ret = false;
	struct net_packet_writer packet;
	if (!net_cmsg_contact_list(&packet, 0x7))
		goto err;
	if (!net_send_packet(g_wow->network, &packet))
		goto err;
	social->loaded = true;
	ret = true;
err:
	net_packet_writer_destroy(&packet);
	return ret;
}

bool social_add_friend(struct social *social, struct social_friend *friend_s)
{
	db_get_name(g_wow->db, friend_s->guid);
	return jks_array_push_back(&social->friends, friend_s);
}

bool social_remove_friend(struct social *social, uint64_t guid)
{
	for (size_t i = 0; i < social->friends.size; ++i)
	{
		struct social_friend *friend_s = JKS_ARRAY_GET(&social->friends, i, struct social_friend);
		if (friend_s->guid != guid)
			continue;
		if (!jks_array_erase(&social->friends, i))
		{
			LOG_ERROR("failed to remove friend");
			return false;
		}
		return true;
	}
	LOG_ERROR("friend not found");
	return false;
}

bool social_add_ignore(struct social *social, struct social_ignore *ignore)
{
	db_get_name(g_wow->db, ignore->guid);
	return jks_array_push_back(&social->ignores, ignore);
}

bool social_remove_ignore(struct social *social, uint64_t guid)
{
	for (size_t i = 0; i < social->ignores.size; ++i)
	{
		struct social_ignore *ignore = JKS_ARRAY_GET(&social->ignores, i, struct social_ignore);
		if (ignore->guid != guid)
			continue;
		if (!jks_array_erase(&social->ignores, i))
		{
			LOG_ERROR("failed to remove ignore");
			return false;
		}
		return true;
	}
	LOG_ERROR("ignore not found");
	return false;
}
