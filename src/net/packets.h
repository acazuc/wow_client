#ifndef NET_PACKETS_H
#define NET_PACKETS_H

#include <stdbool.h>
#include <stdint.h>

struct net_packet_writer;
struct net_packet_reader;

/* auth */
bool net_smsg_auth_challenge(struct net_packet_reader *packet);
bool net_smsg_auth_response(struct net_packet_reader *packet);
bool net_smsg_char_enum(struct net_packet_reader *packet);
bool net_smsg_character_login_failed(struct net_packet_reader *packet);

/* cache */
bool net_cmsg_name_query(struct net_packet_writer *packet, uint64_t guid);
bool net_smsg_name_query_response(struct net_packet_reader *packet);
bool net_cmsg_creature_query(struct net_packet_writer *packet, uint32_t id, uint64_t guid);
bool net_smsg_creature_query_response(struct net_packet_reader *packet);
bool net_cmsg_item_query_single(struct net_packet_writer *packet, uint32_t id);
bool net_smsg_item_query_single_response(struct net_packet_reader *packet);
bool net_cmsg_guild_query(struct net_packet_writer *packet, uint32_t id);
bool net_smsg_guild_query_response(struct net_packet_reader *packet);

/* group */
bool net_smsg_group_list(struct net_packet_reader *packet);
bool net_smsg_group_destroyed(struct net_packet_reader *packet);
bool net_smsg_group_invite(struct net_packet_reader *packet);
bool net_cmsg_group_accept(struct net_packet_writer *packet);
bool net_cmsg_group_decline(struct net_packet_writer *packet);
bool net_smsg_party_member_stats(struct net_packet_reader *packet);

/* misc */
bool net_smsg_login_verify_world(struct net_packet_reader *packet);
bool net_smsg_account_data_times(struct net_packet_reader *packet);
bool net_smsg_feature_system_status(struct net_packet_reader *packet);
bool net_smsg_set_rest_start(struct net_packet_reader *packet);
bool net_smsg_bindpointupdate(struct net_packet_reader *packet);
bool net_smsg_action_buttons(struct net_packet_reader *packet);
bool net_smsg_initialize_factions(struct net_packet_reader *packet);
bool net_smsg_login_settimespeed(struct net_packet_reader *packet);
bool net_smsg_init_world_states(struct net_packet_reader *packet);
bool net_smsg_tutorial_flags(struct net_packet_reader *packet);
bool net_smsg_initial_spells(struct net_packet_reader *packet);
bool net_smsg_time_sync_req(struct net_packet_reader *packet);
bool net_smsg_questgiver_status(struct net_packet_reader *packet);
bool net_smsg_motd(struct net_packet_reader *packet);

/* movement */
bool net_smsg_move(struct net_packet_reader *packet);

/* object */
bool net_smsg_update_object(struct net_packet_reader *packet);
bool net_smsg_destroy_object(struct net_packet_reader *packet);

/* social */
bool net_cmsg_contact_list(struct net_packet_writer *packet, uint32_t flags);
bool net_smsg_contact_list(struct net_packet_reader *packet);
bool net_smsg_friend_status(struct net_packet_reader *packet);
bool net_cmsg_add_friend(struct net_packet_writer *packet, const char *name, const char *note);
bool net_cmsg_del_friend(struct net_packet_writer *packet, uint64_t guid);
bool net_cmsg_set_contact_notes(struct net_packet_writer *packet, uint64_t guid, const char *note);
bool net_cmsg_add_ignore(struct net_packet_writer *packet, const char *name);
bool net_cmsg_del_ignore(struct net_packet_writer *packet, uint64_t guid);

#endif
