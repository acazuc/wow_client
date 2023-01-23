#include "net/packets.h"
#include "net/packet.h"
#include "net/opcode.h"

#include "memory.h"
#include "log.h"
#include "wow.h"
#include "db.h"

#include <string.h>

bool net_cmsg_name_query(struct net_packet_writer *packet, uint64_t guid)
{
	net_packet_writer_init(packet, CMSG_NAME_QUERY);
	if (!net_write_u64(packet, guid))
		return false;
	return true;
}

bool net_smsg_name_query_response(struct net_packet_reader *packet)
{
	uint64_t guid;
	char *name;
	uint32_t race;
	uint32_t gender;
	uint32_t class_id;
	uint8_t dummy;
	if (!net_read_u64(packet, &guid)
	 || !net_read_str(packet, (const char**)&name)
	 || !net_read_u8(packet, &dummy)
	 || !net_read_u32(packet, &race)
	 || !net_read_u32(packet, &gender)
	 || !net_read_u32(packet, &class_id)
	 || !net_read_u8(packet, &dummy))
		return false;
	struct db_name db_name;
	db_name.name = mem_strdup(MEM_GENERIC, name);
	if (!db_name.name)
	{
		LOG_ERROR("malloc failed");
		return false;
	}
	db_name.race = race;
	db_name.gender = gender;
	db_name.class_id = class_id;
	if (!db_set_name(g_wow->db, guid, &db_name))
	{
		mem_free(MEM_GENERIC, db_name.name);
		LOG_ERROR("failed to add name");
		return false;
	}
	return true;
}

bool net_cmsg_creature_query(struct net_packet_writer *packet, uint32_t id, uint64_t guid)
{
	net_packet_writer_init(packet, CMSG_CREATURE_QUERY);
	if (!net_write_u32(packet, id)
	 || !net_write_u64(packet, guid))
		return false;
	return true;
}

bool net_smsg_creature_query_response(struct net_packet_reader *packet)
{
	uint32_t id;
	const char *name;
	const char *subname;
	const char *icon;
	const char *dummy1;
	const char *dummy2;
	const char *dummy3;
	uint32_t creature_type_flags, creature_type, family, rank, unk, pet_spell_data_id, model_id1, model_id2, model_id3, model_id4;
	float health_multiplier, power_multiplier;
	uint8_t racial_leader;
	struct db_creature creature;
	if (!net_read_u32(packet, &id)
	 || !net_read_str(packet, &name)
	 || !net_read_str(packet, &dummy1)
	 || !net_read_str(packet, &dummy2)
	 || !net_read_str(packet, &dummy3)
	 || !net_read_str(packet, &subname)
	 || !net_read_str(packet, &icon)
	 || !net_read_u32(packet, &creature_type_flags)
	 || !net_read_u32(packet, &creature_type)
	 || !net_read_u32(packet, &family)
	 || !net_read_u32(packet, &rank)
	 || !net_read_u32(packet, &unk)
	 || !net_read_u32(packet, &pet_spell_data_id)
	 || !net_read_u32(packet, &model_id1)
	 || !net_read_u32(packet, &model_id2)
	 || !net_read_u32(packet, &model_id3)
	 || !net_read_u32(packet, &model_id4)
	 || !net_read_flt(packet, &health_multiplier)
	 || !net_read_flt(packet, &power_multiplier)
	 || !net_read_u8(packet, &racial_leader))
		return false;
	creature.id = id;
	creature.name = mem_strdup(MEM_GENERIC, name);
	creature.subname = mem_strdup(MEM_GENERIC, subname);
	creature.icon = mem_strdup(MEM_GENERIC, icon);
	creature.creature_type_flags = creature_type_flags;
	creature.creature_type = creature_type;
	creature.family = family;
	creature.rank = rank;
	creature.unk = unk;
	creature.pet_spell_data_id = pet_spell_data_id;
	creature.model_id1 = model_id1;
	creature.model_id2 = model_id2;
	creature.model_id3 = model_id3;
	creature.model_id4 = model_id4;
	creature.health_multiplier = health_multiplier;
	creature.power_multiplier = power_multiplier;
	creature.racial_leader = racial_leader;
	if (!creature.name
	 || !creature.subname
	 || !creature.icon)
	{
		mem_free(MEM_GENERIC, creature.name);
		mem_free(MEM_GENERIC, creature.subname);
		mem_free(MEM_GENERIC, creature.icon);
		LOG_ERROR("allocation failed");
		return false;
	}
	if (!db_set_creature(g_wow->db, id, &creature))
	{
		mem_free(MEM_GENERIC, creature.name);
		mem_free(MEM_GENERIC, creature.subname);
		mem_free(MEM_GENERIC, creature.icon);
		return false;
	}
	return true;
}

bool net_cmsg_item_query_single(struct net_packet_writer *packet, uint32_t id)
{
	net_packet_writer_init(packet, CMSG_ITEM_QUERY_SINGLE);
	if (!net_write_u32(packet, id))
		return false;
	return true;
}

bool net_smsg_item_query_single_response(struct net_packet_reader *packet)
{
	struct db_item item;
	const char *name;
	const char *dummy1;
	const char *dummy2;
	const char *dummy3;
	const char *description;
	if (!net_read_u32(packet, &item.id)
	 || !net_read_u32(packet, &item.type)
	 || !net_read_u32(packet, &item.subtype)
	 || !net_read_u32(packet, &item.unk)
	 || !net_read_str(packet, &name)
	 || !net_read_str(packet, &dummy1)
	 || !net_read_str(packet, &dummy2)
	 || !net_read_str(packet, &dummy3)
	 || !net_read_u32(packet, &item.display_id)
	 || !net_read_u32(packet, &item.quality)
	 || !net_read_u32(packet, &item.flags)
	 || !net_read_u32(packet, &item.buy_price)
	 || !net_read_u32(packet, &item.sell_price)
	 || !net_read_u32(packet, &item.inventory_slot)
	 || !net_read_u32(packet, &item.class_mask)
	 || !net_read_u32(packet, &item.race_mask)
	 || !net_read_u32(packet, &item.level)
	 || !net_read_u32(packet, &item.required_level)
	 || !net_read_u32(packet, &item.required_skill)
	 || !net_read_u32(packet, &item.required_skill_rank)
	 || !net_read_u32(packet, &item.required_spell)
	 || !net_read_u32(packet, &item.required_honor_rank)
	 || !net_read_u32(packet, &item.required_city_rank)
	 || !net_read_u32(packet, &item.required_reputation_faction)
	 || !net_read_u32(packet, &item.required_reputation_rank)
	 || !net_read_u32(packet, &item.unique_count)
	 || !net_read_u32(packet, &item.stack_count)
	 || !net_read_u32(packet, &item.container_slots))
		return false;
	for (size_t i = 0; i < 10; ++i)
	{
		if (!net_read_u32(packet, &item.attributes[i].id)
		 || !net_read_i32(packet, &item.attributes[i].value))
			return false;
	}
	for (size_t i = 0; i < 5; ++i)
	{
		if (!net_read_flt(packet, &item.damages[i].min)
		 || !net_read_flt(packet, &item.damages[i].max)
		 || !net_read_u32(packet, &item.damages[i].type))
			return false;
	}
	for (size_t i = 0; i < 7; ++i)
	{
		if (!net_read_u32(packet, &item.resistances[i]))
			return false;
	}
	if (!net_read_u32(packet, &item.weapon_speed)
	 || !net_read_u32(packet, &item.ammo_type)
	 || !net_read_flt(packet, &item.ranged_mod_range))
		return false;
	for (size_t i = 0; i < 5; ++i)
	{
		if (!net_read_u32(packet, &item.spells[i].id)
		 || !net_read_u32(packet, &item.spells[i].trigger)
		 || !net_read_u32(packet, &item.spells[i].charges)
		 || !net_read_u32(packet, &item.spells[i].cooldown)
		 || !net_read_u32(packet, &item.spells[i].category)
		 || !net_read_u32(packet, &item.spells[i].category_cooldown))
			return false;
	}
	if (!net_read_u32(packet, &item.bonding)
	 || !net_read_str(packet, &description)
	 || !net_read_u32(packet, &item.page_text)
	 || !net_read_u32(packet, &item.language)
	 || !net_read_u32(packet, &item.page_material)
	 || !net_read_u32(packet, &item.start_quest)
	 || !net_read_u32(packet, &item.lock)
	 || !net_read_u32(packet, &item.material)
	 || !net_read_u32(packet, &item.sheathe_type)
	 || !net_read_u32(packet, &item.random_property)
	 || !net_read_u32(packet, &item.random_suffix)
	 || !net_read_u32(packet, &item.block)
	 || !net_read_u32(packet, &item.item_set)
	 || !net_read_u32(packet, &item.durability)
	 || !net_read_u32(packet, &item.area)
	 || !net_read_u32(packet, &item.map)
	 || !net_read_u32(packet, &item.bag_family)
	 || !net_read_u32(packet, &item.totem_category))
		return false;
	for (size_t i = 0; i < 3; ++i)
	{
		if (!net_read_u32(packet, &item.sockets[i].color)
		 || !net_read_u32(packet, &item.sockets[i].content))
			return false;
	}
	if (!net_read_u32(packet, &item.socket_bonus)
	 || !net_read_u32(packet, &item.gem_property)
	 || !net_read_u32(packet, &item.required_disenchant_skill)
	 || !net_read_flt(packet, &item.armor_damage_modifier)
	 || !net_read_u32(packet, &item.duration))
		return false;
	item.name = mem_strdup(MEM_GENERIC, name);
	item.description = mem_strdup(MEM_GENERIC, description);
	if (!item.name || !item.description)
	{
		LOG_ERROR("allocation failed");
		mem_free(MEM_GENERIC, item.name);
		mem_free(MEM_GENERIC, item.description);
		return false;
	}
	if (!db_set_item(g_wow->db, item.id, &item))
	{
		mem_free(MEM_GENERIC, item.name);
		mem_free(MEM_GENERIC, item.description);
		return false;
	}
	return true;
}

bool net_cmsg_guild_query(struct net_packet_writer *packet, uint32_t id)
{
	net_packet_writer_init(packet, CMSG_GUILD_QUERY);
	if (!net_write_u32(packet, id))
		return false;
	return true;
}

bool net_smsg_guild_query_response(struct net_packet_reader *packet)
{
	uint32_t id;
	char *name;
	char *ranks[10];
	uint32_t emblem_style;
	uint32_t emblem_color;
	uint32_t border_style;
	uint32_t border_color;
	uint32_t background_color;
	if (!net_read_u32(packet, &id)
	 || !net_read_str(packet, (const char**)&name))
		return false;
	for (size_t i = 0; i < sizeof(ranks) / sizeof(*ranks); ++i)
	{
		if (!net_read_str(packet, (const char**)&ranks[i]))
			return false;
	}
	if (!net_read_u32(packet, &emblem_style)
	 || !net_read_u32(packet, &emblem_color)
	 || !net_read_u32(packet, &border_style)
	 || !net_read_u32(packet, &border_color)
	 || !net_read_u32(packet, &background_color))
		return false;
	struct db_guild db_guild;
	db_guild.name = mem_strdup(MEM_GENERIC, name);
	if (!db_guild.name)
	{
		LOG_ERROR("malloc failed");
		return false;
	}
	for (size_t i = 0; i < sizeof(ranks) / sizeof(*ranks); ++i)
	{
		db_guild.ranks[i] = mem_strdup(MEM_GENERIC, ranks[i]);
		if (!db_guild.ranks[i])
		{
			for (size_t j = 0; j < i; ++j)
				mem_free(MEM_GENERIC, db_guild.ranks[j]);
			mem_free(MEM_GENERIC, db_guild.name);
			LOG_ERROR("malloc failed");
			return false;
		}
	}
	db_guild.id = id;
	db_guild.emblem_style = emblem_style;
	db_guild.emblem_color = emblem_color;
	db_guild.border_style = border_style;
	db_guild.border_color = border_color;
	db_guild.background_color = background_color;
	if (!db_set_guild(g_wow->db, id, &db_guild))
	{
		mem_free(MEM_GENERIC, db_guild.name);
		LOG_ERROR("failed to add guild");
		return false;
	}
	return true;
}
