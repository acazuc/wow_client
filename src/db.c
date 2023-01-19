#include "db.h"

#include "itf/interface.h"

#include "net/packets.h"
#include "net/network.h"
#include "net/packet.h"
#include "net/opcode.h"

#include "obj/object.h"

#include "wow_lua.h"
#include "memory.h"
#include "wow.h"
#include "log.h"

#include <string.h>

MEMORY_DECL(GENERIC);

static void name_dtr(jks_hmap_key_t key, void *data)
{
	(void)key;
	struct db_name *name = data;
	mem_free(MEM_GENERIC, name->name);
}

static void creature_dtr(jks_hmap_key_t key, void *data)
{
	(void)key;
	struct db_creature *creature = data;
	mem_free(MEM_GENERIC, creature->name);
	mem_free(MEM_GENERIC, creature->subname);
	mem_free(MEM_GENERIC, creature->icon);
}

static void gameobject_dtr(jks_hmap_key_t key, void *data)
{
	(void)key;
	struct db_gameobject *gameobject = data;
	mem_free(MEM_GENERIC, gameobject->name);
	mem_free(MEM_GENERIC, gameobject->icon);
	mem_free(MEM_GENERIC, gameobject->cast_bar_caption);
}

static void item_dtr(jks_hmap_key_t key, void *data)
{
	(void)key;
	struct db_item *item = data;
	mem_free(MEM_GENERIC, item->name);
	mem_free(MEM_GENERIC, item->description);
}

static void guild_dtr(jks_hmap_key_t key, void *data)
{
	(void)key;
	struct db_guild *guild = data;
	mem_free(MEM_GENERIC, guild->name);
	for (size_t i = 0; i < sizeof(guild->ranks) / sizeof(*guild->ranks); ++i)
		mem_free(MEM_GENERIC, guild->ranks[i]);
}

struct db *db_new(void)
{
	struct db *db = mem_malloc(MEM_GENERIC, sizeof(*db));
	if (!db)
		return NULL;
	jks_hmap_init(&db->names, sizeof(struct db_name), name_dtr, jks_hmap_hash_u64, jks_hmap_cmp_u64, &jks_hmap_memory_fn_GENERIC);
	jks_hmap_init(&db->creatures, sizeof(struct db_creature), creature_dtr, jks_hmap_hash_u32, jks_hmap_cmp_u32, &jks_hmap_memory_fn_GENERIC);
	jks_hmap_init(&db->gameobjects, sizeof(struct db_gameobject), gameobject_dtr, jks_hmap_hash_u32, jks_hmap_cmp_u32, &jks_hmap_memory_fn_GENERIC);
	jks_hmap_init(&db->items, sizeof(struct db_item), item_dtr, jks_hmap_hash_u32, jks_hmap_cmp_u32, &jks_hmap_memory_fn_GENERIC);
	jks_hmap_init(&db->guilds, sizeof(struct db_guild), guild_dtr, jks_hmap_hash_u32, jks_hmap_cmp_u32, &jks_hmap_memory_fn_GENERIC);
	return db;
}

void db_delete(struct db *db)
{
	if (!db)
		return;
	jks_hmap_destroy(&db->names);
	jks_hmap_destroy(&db->creatures);
	jks_hmap_destroy(&db->gameobjects);
	jks_hmap_destroy(&db->items);
	jks_hmap_destroy(&db->guilds);
	mem_free(MEM_GENERIC, db);
}

bool db_set_name(struct db *db, uint64_t id, struct db_name *name)
{
	if (!jks_hmap_set(&db->names, JKS_HMAP_KEY_U64(id), name))
	{
		LOG_ERROR("failed to add db name");
		return false;
	}
	const char *update = NULL;
	if (g_wow->player && id == object_guid((struct object*)g_wow->player))
		update = "player";
	if (update && g_wow->interface)
	{
		lua_pushnil(g_wow->interface->L);
		lua_pushstring(g_wow->interface->L, update);
		interface_execute_event(g_wow->interface, EVENT_UNIT_NAME_UPDATE, 1);
	}
	return true;
}

const struct db_name *db_get_name(struct db *db, uint64_t id)
{
	const struct db_name *name = jks_hmap_get(&db->names, JKS_HMAP_KEY_U64(id));
	if (name)
		return name;
	struct db_name new_name;
	memset(&new_name, 0, sizeof(new_name));
	if (!jks_hmap_set(&db->names, JKS_HMAP_KEY_U64(id), &new_name))
		LOG_WARN("can't set name hmap");
	struct net_packet_writer packet;
	if (!net_cmsg_name_query(&packet, id)
	 || !net_send_packet(g_wow->network, &packet))
		LOG_WARN("can't send packet");
	net_packet_writer_destroy(&packet);
	return NULL;
}

bool db_set_creature(struct db *db, uint32_t id, struct db_creature *creature)
{
	if (!jks_hmap_set(&db->creatures, JKS_HMAP_KEY_U32(id), creature))
	{
		LOG_ERROR("failed to add db creature");
		return false;
	}
	return true;
}

const struct db_creature *db_get_creature(struct db *db, uint32_t id)
{
	const struct db_creature *creature = jks_hmap_get(&db->creatures, JKS_HMAP_KEY_U32(id));
	if (creature)
		return creature;
	struct db_creature new_creature;
	memset(&new_creature, 0, sizeof(new_creature));
	if (!jks_hmap_set(&db->creatures, JKS_HMAP_KEY_U32(id), &new_creature))
		LOG_WARN("can't set creature hmap");
	struct net_packet_writer packet;
	if (!net_cmsg_creature_query(&packet, id, 0) /* TODO guid */
	 || !net_send_packet(g_wow->network, &packet))
		LOG_WARN("failed to send packet");
	net_packet_writer_destroy(&packet);
	return NULL;
}

bool db_set_item(struct db *db, uint32_t id, struct db_item *item)
{
	if (!jks_hmap_set(&db->items, JKS_HMAP_KEY_U32(id), item))
	{
		LOG_ERROR("failed to add db item");
		return false;
	}
	return true;
}

const struct db_item *db_get_item(struct db *db, uint32_t id)
{
	const struct db_item *item = jks_hmap_get(&db->items, JKS_HMAP_KEY_U32(id));
	if (item)
		return item;
	struct db_item new_item;
	memset(&new_item, 0, sizeof(new_item));
	if (!jks_hmap_set(&db->items, JKS_HMAP_KEY_U32(id), &new_item))
		LOG_WARN("can't set item hmap");
	struct net_packet_writer packet;
	if (!net_cmsg_item_query_single(&packet, id)
	 || !net_send_packet(g_wow->network, &packet))
		LOG_WARN("failed to send packet");
	net_packet_writer_destroy(&packet);
	return NULL;
	return item;
}

bool db_set_guild(struct db *db, uint32_t id, struct db_guild *guild)
{
	if (!jks_hmap_set(&db->guilds, JKS_HMAP_KEY_U32(id), guild))
	{
		LOG_ERROR("failed to add db guild");
		return false;
	}
	return true;
}

const struct db_guild *db_get_guild(struct db *db, uint32_t id)
{
	const struct db_guild *guild = jks_hmap_get(&db->guilds, JKS_HMAP_KEY_U32(id));
	if (guild)
		return guild;
	struct db_guild new_guild;
	memset(&new_guild, 0, sizeof(new_guild));
	if (!jks_hmap_set(&db->guilds, JKS_HMAP_KEY_U32(id), &new_guild))
		LOG_WARN("can't set guild hmap");
	struct net_packet_writer packet;
	if (!net_cmsg_guild_query(&packet, id)
	 || !net_send_packet(g_wow->network, &packet))
		LOG_WARN("failed to send packet");
	net_packet_writer_destroy(&packet);
	return NULL;
}
