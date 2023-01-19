#ifndef DB_H
#define DB_H

#include <jks/hmap.h>

#include <stdint.h>

struct db_name
{
	uint64_t id;
	char *name;
	uint8_t race;
	uint8_t gender;
	uint8_t class_id;
};

struct db_creature
{
	uint32_t id;
	char *name;
	char *subname;
	char *icon;
	uint32_t creature_type_flags;
	uint32_t creature_type;
	uint32_t family;
	uint32_t rank;
	uint32_t unk;
	uint32_t pet_spell_data_id;
	uint32_t model_id1;
	uint32_t model_id2;
	uint32_t model_id3;
	uint32_t model_id4;
	float health_multiplier;
	float power_multiplier;
	uint8_t racial_leader;
};

struct db_gameobject
{
	uint32_t id;
	uint32_t type;
	uint32_t displayid;
	char *name;
	char *icon;
	char *cast_bar_caption;
	uint32_t data[24];
	float scale;
};

struct db_item
{
	uint32_t id;
	uint32_t type;
	uint32_t subtype;
	uint32_t unk;
	char *name;
	uint32_t display_id;
	uint32_t quality;
	uint32_t flags;
	uint32_t buy_price;
	uint32_t sell_price;
	uint32_t inventory_slot;
	uint32_t class_mask;
	uint32_t race_mask;
	uint32_t level;
	uint32_t required_level;
	uint32_t required_skill;
	uint32_t required_skill_rank;
	uint32_t required_spell;
	uint32_t required_honor_rank;
	uint32_t required_city_rank;
	uint32_t required_reputation_faction;
	uint32_t required_reputation_rank;
	uint32_t unique_count;
	uint32_t stack_count;
	uint32_t container_slots;
	struct
	{
		uint32_t id;
		int32_t value;
	} attributes[10];
	struct
	{
		float min;
		float max;
		uint32_t type;
	} damages[5];
	uint32_t resistances[7];
	uint32_t weapon_speed;
	uint32_t ammo_type;
	float ranged_mod_range;
	struct
	{
		uint32_t id;
		uint32_t trigger;
		uint32_t charges;
		uint32_t cooldown;
		uint32_t category;
		uint32_t category_cooldown;
	} spells[5];
	uint32_t bonding;
	char *description;
	uint32_t page_text;
	uint32_t language;
	uint32_t page_material;
	uint32_t start_quest;
	uint32_t lock;
	uint32_t material;
	uint32_t sheathe_type;
	uint32_t random_property;
	uint32_t random_suffix;
	uint32_t block;
	uint32_t item_set;
	uint32_t durability;
	uint32_t area;
	uint32_t map;
	uint32_t bag_family;
	uint32_t totem_category;
	struct
	{
		uint32_t color;
		uint32_t content;
	} sockets[3];
	uint32_t socket_bonus;
	uint32_t gem_property;
	uint32_t required_disenchant_skill;
	float armor_damage_modifier;
	uint32_t duration;
};

struct db_guild
{
	uint32_t id;
	char *name;
	char *ranks[10];
	uint32_t emblem_style;
	uint32_t emblem_color;
	uint32_t border_style;
	uint32_t border_color;
	uint32_t background_color;
};

struct db
{
	struct jks_hmap names; /* uint64_t, struct db_name */
	struct jks_hmap creatures; /* uint32_t, struct db_creature */
	struct jks_hmap gameobjects; /* uint32_t, struct db_gameobject */
	struct jks_hmap items; /* uint32_t, struct db_item */
	struct jks_hmap guilds; /* uint32_t, struct db_guild */
};

struct db *db_new(void);
void db_delete(struct db *db);
bool db_set_name(struct db *db, uint64_t id, struct db_name *name);
const struct db_name *db_get_name(struct db *db, uint64_t id);
bool db_set_creature(struct db *db, uint32_t id, struct db_creature *creature);
const struct db_creature *db_get_creature(struct db *db, uint32_t id);
bool db_set_item(struct db *db, uint32_t id, struct db_item *item);
const struct db_item *db_get_item(struct db *db, uint32_t id);
bool db_set_guild(struct db *db, uint32_t id, struct db_guild *guild);
const struct db_guild *db_get_guild(struct db *db, uint32_t id);

#endif
