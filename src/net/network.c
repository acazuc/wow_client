#include "network.h"

#include "net/world_socket.h"
#include "net/auth_socket.h"

#include "memory.h"
#include "log.h"

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

MEMORY_DECL(NET);

static void world_server_dtr(void *ptr)
{
	struct world_server *world_server = ptr;
	mem_free(MEM_NET, world_server->name);
	mem_free(MEM_NET, world_server->host);
}

static const char *g_host = "127.0.0.1";
static uint16_t g_port = 3724;

static void close_auth_socket(struct network *network);
static void close_world_socket(struct network *network);

struct network *network_new(void)
{
	struct network *network = mem_malloc(MEM_NET, sizeof(*network));
	if (!network)
	{
		LOG_ERROR("alloc failed");
		return NULL;
	}
	network->world_socket = NULL;
	network->auth_socket = NULL;
	network->username = NULL;
	network->password = NULL;
	net_packet_handler_init(&network->packet_handler);
	jks_array_init(&network->world_servers, sizeof(struct world_server), world_server_dtr, &jks_array_memory_fn_NET);
	return network;
}

void network_delete(struct network *network)
{
	if (!network)
		return;
	net_packet_handler_destroy(&network->packet_handler);
	jks_array_destroy(&network->world_servers);
	close_world_socket(network);
	close_auth_socket(network);
	mem_free(MEM_NET, network->username);
	mem_free(MEM_NET, network->password);
	mem_free(MEM_NET, network);
}

static void close_auth_socket(struct network *network)
{
	if (!network->auth_socket)
		return;
	/* somehow avoid potential races */
	struct net_auth_socket *tmp = network->auth_socket;
	network->auth_socket = NULL;
	net_auth_socket_delete(tmp);
}

static void close_world_socket(struct network *network)
{
	if (!network->world_socket)
		return;
	/* somehow avoid potential races */
	struct net_world_socket *tmp = network->world_socket;
	network->world_socket = NULL;
	net_world_socket_delete(tmp);
}

void net_tick(struct network *network)
{
	if (network->auth_socket)
	{
		if (!net_auth_socket_tick(network->auth_socket))
		{
			LOG_ERROR("error on auth socket tick");
			close_auth_socket(network);
		}
	}
	if (network->world_socket)
	{
		if (!net_world_socket_tick(network->world_socket))
		{
			LOG_ERROR("error on world socket tick");
			close_world_socket(network);
		}
	}
}

bool net_auth_connect(struct network *network, const char *username, const char *password)
{
	if (network->auth_socket || network->world_socket)
	{
		LOG_ERROR("should be on no phase");
		goto err;
	}
	network->auth_socket = net_auth_socket_new(network);
	if (!network->auth_socket)
	{
		LOG_ERROR("failed to create auth socket");
		goto err;
	}
	if (!net_socket_connect(&network->auth_socket->socket, g_host, g_port))
	{
		LOG_ERROR("connect() failed");
		goto err;
	}
	network->auth_socket->auth_phase = NET_AUTH_CONNECTING;
	network->username = mem_strdup(MEM_NET, username);
	network->password = mem_strdup(MEM_NET, password);
	if (!network->username || !network->password)
	{
		LOG_ERROR("failed to duplicate username or password");
		mem_free(MEM_NET, network->username);
		mem_free(MEM_NET, network->password);
		network->username = NULL;
		network->password = NULL;
		goto err;
	}
	return true;
err:
	close_auth_socket(network);
	return false;
}

bool net_world_connect(struct network *network, const char *address)
{
	char host[256];
	uint16_t port;
	const char *pos;
	if (network->world_socket)
	{
		LOG_ERROR("should be on auth phase");
		goto err;
	}
	close_auth_socket(network);
	pos = strchr(address, ':');
	if (!pos)
	{
		LOG_ERROR("invalid address");
		goto err;
	}
	snprintf(host, sizeof(host), "%.*s", (int)(pos - address), address);
	port = atoi(pos + 1);
	network->world_socket = net_world_socket_new(network);
	LOG_INFO("connecting to %s:%" PRIu16, host, port);
	if (!net_socket_connect(&network->world_socket->socket, host, port))
	{
		LOG_ERROR("connect() failed");
		goto err;
	}
	return true;
err:
	close_world_socket(network);
	return false;
}

void net_disconnect(struct network *network)
{
	close_auth_socket(network);
	close_world_socket(network);
}

bool net_send_packet(struct network *network, const struct net_packet_writer *packet)
{
	if (!network->world_socket)
		return true;
	return net_world_socket_send_packet(network->world_socket, packet);
}

bool net_add_world_server(struct network *network, const struct world_server *world_server)
{
	if (!jks_array_push_back(&network->world_servers, world_server))
	{
		LOG_ERROR("failed to push world server");
		return false;
	}
	LOG_INFO("adding new server");
	return true;
}

void net_set_auth_key(struct network *network, const uint8_t *auth_key)
{
	memcpy(network->auth_key, auth_key, 40);
}
