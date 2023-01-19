#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include "net/buffer.h"

#include <stdbool.h>

#ifdef _WIN32
# include <winsock2.h>
# include <ws2tcpip.h>
  typedef int SOCKLEN_T;
#else
# include <netinet/in.h>
# define INVALID_SOCKET -1
# define SOCKET_ERROR -1
  typedef socklen_t SOCKLEN_T;
  typedef int SOCKET;
#endif


struct net_protocol
{
	int domain;
	int type;
	int protocol;
};

extern const struct net_protocol net_protocol_ipv4_tcp;
extern const struct net_protocol net_protocol_ipv6_tcp;
extern const struct net_protocol net_protocol_ipv4_udp;
extern const struct net_protocol net_protocol_ipv6_udp;

struct net_socket
{
	union
	{
		struct sockaddr sockaddr;
		struct sockaddr_in sockaddr_in;
		struct sockaddr_in6 sockaddr_in6;
	};
	const struct net_protocol *protocol;
	SOCKET sockfd;
	bool waiting_connection;
	bool connected;
	struct net_buffer rbuffer;
	struct net_buffer wbuffer;
};

bool net_socket_init(struct net_socket *socket);
void net_socket_destroy(struct net_socket *socket);
bool net_socket_connect(struct net_socket *socket, const char *ip, uint16_t port);
int net_socket_get_connection_status(struct net_socket *socket);
bool net_socket_send(struct net_socket *socket);
bool net_socket_recv(struct net_socket *socket);

#endif
