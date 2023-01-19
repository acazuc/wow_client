#ifndef NET_NETWORK_H
#define NET_NETWORK_H

#include "packet_handler.h"

#include <jks/array.h>

#include <stdbool.h>
#include <stdint.h>

struct net_packet_writer;
struct net_world_socket;
struct net_auth_socket;

enum world_server_flag
{
	WORLD_SERVER_NONE         = 0x00,
	WORLD_SERVER_INVALID      = 0x01,
	WORLD_SERVER_OFFLINE      = 0x02,
	WORLD_SERVER_SPECIFYBUILD = 0x04,
	WORLD_SERVER_UNK1         = 0x08,
	WORLD_SERVER_UNK2         = 0x10,
	WORLD_SERVER_NEW_PLAYERS  = 0x20,
	WORLD_SERVER_RECOMMENDED  = 0x40,
	WORLD_SERVER_FULL         = 0x80
};

struct world_server
{
	uint8_t icon;
	uint8_t locked;
	uint8_t flags;
	char *name;
	char *host;
	float load;
	uint8_t characters;
	uint8_t timezone; /* 1 =dev, usa, ocea, latina, tournament, corea, tournament, english, deutch, francais, espanol, russian, taiwan */
};

struct network
{
	struct net_packet_handler packet_handler;
	struct net_world_socket *world_socket;
	struct net_auth_socket *auth_socket;
	struct jks_array world_servers; /* struct world_server */
	uint8_t auth_key[40];
	char *username;
	char *password;
};

struct network *network_new(void);
void network_delete(struct network *network);
void net_tick(struct network *network);
bool net_auth_connect(struct network *network, const char *username, const char *password);
bool net_world_connect(struct network *network, const char *address);
void net_disconnect(struct network *network);
bool net_send_packet(struct network *network, const struct net_packet_writer *packet);
bool net_add_world_server(struct network *network, const struct world_server *world_server);
void net_set_auth_key(struct network *network, const uint8_t *auth_key);

#endif
