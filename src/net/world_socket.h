#ifndef NET_WORLD_SOCKET_H
#define NET_WORLD_SOCKET_H

#include "net/socket.h"

#include <jks/array.h>

#include <stdbool.h>

struct net_packet_writer;
struct net_packet_reader;
struct network;

struct login_character_item
{
	uint32_t display_id;
	uint8_t inventory_type;
	uint32_t aura;
};

struct login_character
{
	uint64_t guid;
	char *name;
	uint8_t race_type;
	uint8_t class_type;
	uint8_t gender;
	uint8_t skin_color;
	uint8_t face;
	uint8_t hair_style;
	uint8_t hair_color;
	uint8_t facial_hair;
	uint8_t level;
	uint32_t zone;
	uint32_t map;
	float x;
	float y;
	float z;
	uint32_t guild;
	uint32_t flags;
	uint8_t first_login;
	uint32_t pet_display;
	uint32_t pet_level;
	uint32_t pet_family;
	struct login_character_item items[19];
	uint32_t bag_display_id;
	uint8_t bag_inventory_type;
	uint32_t enchant;
};

struct net_world_socket
{
	struct net_socket socket;
	struct network *network;
	struct jks_array characters; /* struct login_character */
	uint8_t key[20];
	size_t recv_i;
	size_t recv_j;
	size_t send_i;
	size_t send_j;
	bool ciphered;
};

struct net_world_socket *net_world_socket_new(struct network *network);
void net_world_socket_delete(struct net_world_socket *socket);
bool net_world_socket_tick(struct net_world_socket *socket);
bool net_world_socket_send_packet(struct net_world_socket *socket, const struct net_packet_writer *packet);
void net_world_socket_init_cipher(struct net_world_socket *socket, const uint8_t *key);
bool net_world_socket_add_character(struct net_world_socket *socket, const struct login_character *character);

#endif
