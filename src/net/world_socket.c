#include "world_socket.h"

#include "net/packet_handler.h"
#include "net/network.h"
#include "net/packet.h"

#include "opcode_str.h"
#include "memory.h"
#include "log.h"

#include <string.h>

MEMORY_DECL(NET);

static bool recv_packet(struct net_world_socket *socket, struct net_packet_reader *packet);

static void login_character_dtr(void *data)
{
	struct login_character *character = data;
	mem_free(MEM_NET, character->name);
}

struct net_world_socket *net_world_socket_new(struct network *network)
{
	struct net_world_socket *socket = mem_malloc(MEM_NET, sizeof(*socket));
	if (!socket)
	{
		LOG_ERROR("failed to malloc socket");
		return NULL;
	}
	if (!net_socket_init(&socket->socket))
	{
		LOG_ERROR("failed to init world sockets socket");
		mem_free(MEM_NET, socket);
		return NULL;
	}
	socket->network = network;
	socket->ciphered = false;
	jks_array_init(&socket->characters, sizeof(struct login_character), login_character_dtr, &jks_array_memory_fn_NET);
	return socket;
}

void net_world_socket_delete(struct net_world_socket *socket)
{
	if (!socket)
		return;
	jks_array_destroy(&socket->characters);
	net_socket_destroy(&socket->socket);
	mem_free(MEM_NET, socket);
}

bool net_world_socket_tick(struct net_world_socket *socket)
{
	switch (net_socket_get_connection_status(&socket->socket))
	{
		case -1:
			LOG_ERROR("failed to connect");
			return false;
		case 0:
			/* XXX timeout */
			return true;
		case 1:
			break;
	}
	if (!net_socket_recv(&socket->socket))
		return false;
	struct net_packet_reader packet;
	while (recv_packet(socket, &packet))
	{
		LOG_INFO("recv \e[1;36m%s\e[0m", net_opcodes_str[packet.opcode]);
		net_packet_handle(&socket->network->packet_handler, &packet);
		net_packet_reader_destroy(&packet);
	}
	if (!net_socket_send(&socket->socket))
		return false;
	return true;
}

static void cipher_recv(struct net_world_socket *socket, uint8_t *ptr)
{
	for (size_t i = 0; i < 4; ++i)
	{
		uint8_t tmp = (ptr[i] - socket->recv_j) ^ socket->key[socket->recv_i];
		socket->recv_i = (socket->recv_i + 1) % sizeof(socket->key);
		socket->recv_j = ptr[i];
		ptr[i] = tmp;
	}
}

static void cipher_send(struct net_world_socket *socket, uint8_t *ptr)
{
	for (size_t i = 0; i < 6; ++i)
	{
		uint8_t tmp = (ptr[i] ^ socket->key[socket->send_i]) + socket->send_j;
		socket->send_i = (socket->send_i + 1) % sizeof(socket->key);
		socket->send_j = tmp;
		ptr[i] = tmp;
	}
}

bool net_world_socket_send_packet(struct net_world_socket *socket, const struct net_packet_writer *packet)
{
	LOG_INFO("send \e[1;35m%s\e[0m", net_opcodes_str[packet->opcode]);
	size_t pos = socket->socket.wbuffer.position;
	uint8_t bytes[6];
	bytes[0] = (packet->data.size + 4) / 0x100;
	bytes[1] = (packet->data.size + 4) & 0xff;
	bytes[2] = packet->opcode & 0xff;
	bytes[3] = packet->opcode / 0x100;
	bytes[4] = 0;
	bytes[5] = 0;
	if (!net_buffer_write(&socket->socket.wbuffer, bytes, sizeof(bytes)))
	{
		LOG_ERROR("failed to write header to buffer");
		return false;
	}
	if (socket->ciphered)
		cipher_send(socket, socket->socket.wbuffer.data + pos);
	if (!net_buffer_write(&socket->socket.wbuffer, packet->data.data, packet->data.size))
	{
		LOG_ERROR("failed to write data to buffer");
		return false;
	}
	return true;
}

static bool recv_packet(struct net_world_socket *socket, struct net_packet_reader *packet)
{
	if (socket->socket.rbuffer.position + 4 > socket->socket.rbuffer.limit + 4)
		return false;
	uint16_t len;
	uint8_t *ptr = socket->socket.rbuffer.data + socket->socket.rbuffer.position;
	if (socket->ciphered)
	{
		uint8_t header[4];
		header[0] = ptr[0];
		header[1] = ptr[1];
		header[2] = ptr[2];
		header[3] = ptr[3];
		size_t old_i = socket->recv_i;
		size_t old_j = socket->recv_j;
		cipher_recv(socket, header);
		socket->recv_i = old_i;
		socket->recv_j = old_j;
		len = ntohs(((uint8_t*)header)[0] | (((int16_t)((uint8_t*)header)[1]) << 8));
	}
	else
	{
		len = ntohs(ptr[0] | ((uint16_t)(ptr[1]) << 8));
	}
	if (socket->socket.rbuffer.position + len + 2 > socket->socket.rbuffer.limit)
		return false;
	if (socket->ciphered)
		cipher_recv(socket, ptr);
	uint16_t opcode = *(ptr + 2);
	opcode |= (uint16_t)*(ptr + 3) << 8;
	if (!net_packet_reader_init(packet, opcode, ptr + 4, len - 2))
		return false;
	socket->socket.rbuffer.position += len + 2;
	return true;
}

void net_world_socket_init_cipher(struct net_world_socket *socket, const uint8_t *key)
{
	memcpy(socket->key, key, sizeof(socket->key));
	socket->recv_i = 0;
	socket->recv_j = 0;
	socket->send_i = 0;
	socket->send_j = 0;
	socket->ciphered = true;
}

bool net_world_socket_add_character(struct net_world_socket *socket, const struct login_character *character)
{
	struct login_character *new_character = jks_array_grow(&socket->characters, 1);
	if (!new_character)
	{
		LOG_ERROR("failed to allocate new character");
		return false;
	}
	memcpy(new_character, character, sizeof(*character));
	return true;
}
