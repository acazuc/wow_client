#ifndef NET_PACKET_HANDLER_H
#define NET_PACKET_HANDLER_H

#include <jks/array.h>

struct net_packet_reader;

struct net_packet_handle
{
	uint16_t opcode;
	const char *name;
	bool (*handle)(struct net_packet_reader *packet);
};

struct net_packet_handler
{
	struct jks_array handles; /* struct net_packet_handle* */
};

bool net_packet_handler_init(struct net_packet_handler *handler);
void net_packet_handler_destroy(struct net_packet_handler *handler);
bool net_packet_handle(struct net_packet_handler *handler, struct net_packet_reader *packet);

#endif
