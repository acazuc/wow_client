#include "socket.h"

#include "log.h"

#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#ifndef _WIN32
# include <netinet/tcp.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <unistd.h>
# include <fcntl.h>
# include <netdb.h>
#endif

const struct net_protocol net_protocol_ipv4_tcp = {AF_INET , SOCK_STREAM, IPPROTO_TCP};
const struct net_protocol net_protocol_ipv6_tcp = {AF_INET6, SOCK_STREAM, IPPROTO_TCP};
const struct net_protocol net_protocol_ipv4_udp = {AF_INET , SOCK_DGRAM , IPPROTO_UDP};
const struct net_protocol net_protocol_ipv6_udp = {AF_INET6, SOCK_DGRAM , IPPROTO_UDP};

bool net_socket_init(struct net_socket *socket)
{
	socket->protocol = NULL;
	socket->waiting_connection = false;
	socket->connected = false;
	socket->sockfd = -1;
	if (!net_buffer_init(&socket->rbuffer, 1024 * 128))
	{
		LOG_ERROR("failed to init read buffer");
		return false;
	}
	if (!net_buffer_init(&socket->wbuffer, 1024 * 128))
	{
		LOG_ERROR("failed to init write buffer");
		return false;
	}
	net_buffer_flip(&socket->rbuffer);
	return true;
}

void net_socket_destroy(struct net_socket *socket)
{
	if (socket->protocol)
	{
#ifdef _WIN32
		closesocket(socket->sockfd);
#else
		close(socket->sockfd);
#endif
	}
	net_buffer_destroy(&socket->rbuffer);
	net_buffer_destroy(&socket->wbuffer);
}

static bool set_nagle(struct net_socket *socket, bool active)
{
	if (!socket->protocol)
		return false;
	int flag = active ? 1 : 0;
	return setsockopt(socket->sockfd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag)) == 0;
}

static bool set_blocking(struct net_socket *socket, bool blocking)
{
	if (!socket->protocol)
		return false;
#ifdef _WIN32
	u_long mode = blocking ? 0 : 1;
	return ioctlsocket(socket->sockfd, FIONBIO, &mode) == 0;
#else
	int flags = fcntl(socket->sockfd, F_GETFL, 0);
	if (flags < 0)
		return false;
	flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
	return fcntl(socket->sockfd, F_SETFL, flags) == 0;
#endif
}

static bool open_socket(struct net_socket *sock, const struct net_protocol *protocol)
{
	if (sock->protocol)
		return false;
	sock->sockfd = socket(protocol->domain, protocol->type, protocol->protocol);
	if (sock->sockfd == INVALID_SOCKET)
		return false;
	sock->protocol = protocol;
	return true;
}

static bool do_connect(struct net_socket *socket, const char *host, uint16_t port)
{
	if (!socket->protocol)
		return false;
	char service[32];
	snprintf(service, sizeof(service), "%" PRIu16, port);
	struct addrinfo *result = NULL;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = socket->protocol->domain;
	hints.ai_socktype = socket->protocol->type;
	hints.ai_protocol = socket->protocol->protocol;
	int rett = getaddrinfo(host, service, &hints, &result);
	if (rett)
		return false;
	if (!result)
		return false;
	switch (result->ai_family)
	{
		case AF_INET:
			memcpy(&socket->sockaddr_in, result->ai_addr, sizeof(struct sockaddr_in));
			break;
		case AF_INET6:
			memcpy(&socket->sockaddr_in6, result->ai_addr, sizeof(struct sockaddr_in6));
			break;
		default:
			return false;
	}
	int ret = connect(socket->sockfd, result->ai_addr, result->ai_addrlen);
	freeaddrinfo(result);
	if (ret == SOCKET_ERROR)
	{
#ifdef _WIN32
		if (WSAGetLastError() != WSAEWOULDBLOCK)
			return false;
#else
		if (errno != EINPROGRESS)
			return false;
#endif
		socket->waiting_connection = true;
	}
	else
	{
		socket->connected = true;
	}
	return true;
}

bool net_socket_connect(struct net_socket *socket, const char *ip, uint16_t port)
{
	if (socket->sockfd != -1)
		return false;
	if (!open_socket(socket, &net_protocol_ipv4_tcp))
		return false;
	if (!set_nagle(socket, false))
		return false;
	if (!set_blocking(socket, false))
		return false;
	if (!do_connect(socket, ip, port))
		return false;
	return true;
}

int net_socket_get_connection_status(struct net_socket *socket)
{
	if (socket->connected)
			return 1;
	if (!socket->waiting_connection)
		return -1;
	struct timeval tv;
	fd_set fdset_write;
	fd_set fdset_except;
	int ret;
	tv.tv_sec = 0;
	tv.tv_usec = 1;
	FD_ZERO(&fdset_write);
	FD_ZERO(&fdset_except);
	FD_SET(socket->sockfd, &fdset_write);
	FD_SET(socket->sockfd, &fdset_except);
	if ((ret = select(socket->sockfd + 1, NULL, &fdset_write, &fdset_except, &tv)))
	{
		socket->waiting_connection = false;
		if (ret == SOCKET_ERROR)
			return -1;
		int err;
		socklen_t len = sizeof(err);
		if (getsockopt(socket->sockfd, SOL_SOCKET, SO_ERROR, (char*)&err, &len) == SOCKET_ERROR)
			return -1;
#ifdef _WIN32
		if (err && err != WSAEINPROGRESS)
			return -1;
#else

		if (err && err != EINPROGRESS)
			return -1;
#endif
		socket->connected = true;
		return 1;
	}
	return 0;
}

bool net_socket_send(struct net_socket *socket)
{
	if (!socket->connected)
		return false;
	net_buffer_flip(&socket->wbuffer);
	if (socket->wbuffer.position >= socket->wbuffer.limit)
	{
		net_buffer_clear(&socket->wbuffer);
		return true;
	}
	ssize_t sent = send(socket->sockfd, (const char*)socket->wbuffer.data, socket->wbuffer.limit - socket->wbuffer.position, 0);
	if (sent == SOCKET_ERROR)
	{
#ifdef _WIN32
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			LOG_ERROR("socket send: %d", (int)WSAGetLastError());
			sent = -1;
		}
#else
		if (errno != EWOULDBLOCK && errno != EAGAIN)
		{
			LOG_ERROR("socket send: %d", errno);
			sent = -1;
		}
#endif
		else
		{
			sent = 0;
		}
	}
	if (sent > 0)
		socket->wbuffer.position += sent;
	if (socket->wbuffer.limit > socket->wbuffer.position)
	{
		if (socket->wbuffer.position)
			memmove(socket->wbuffer.data, socket->wbuffer.data + socket->wbuffer.position, socket->wbuffer.limit - socket->wbuffer.position);
		socket->wbuffer.position = socket->wbuffer.limit - socket->wbuffer.position;
		socket->wbuffer.limit = socket->wbuffer.size;
	}
	else
	{
		net_buffer_clear(&socket->wbuffer);
	}
	return sent != -1;
}

bool net_socket_recv(struct net_socket *socket)
{
	if (!socket->connected)
		return false;
	if (socket->rbuffer.position < socket->rbuffer.limit)
	{
		if (socket->rbuffer.position)
			memmove(socket->rbuffer.data, socket->rbuffer.data + socket->rbuffer.position, socket->rbuffer.limit - socket->rbuffer.position);
		socket->rbuffer.position = socket->rbuffer.limit - socket->rbuffer.position;
		socket->rbuffer.limit = socket->rbuffer.size;
	}
	else
	{
		net_buffer_clear(&socket->rbuffer);
	}
	ssize_t readed = recv(socket->sockfd, (char*)socket->rbuffer.data + socket->rbuffer.position, socket->rbuffer.limit - socket->rbuffer.position, 0);
	if (readed == SOCKET_ERROR)
	{
#ifdef _WIN32
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			LOG_ERROR("socket recv: %d", (int)WSAGetLastError());
			readed = -1;
		}
#else
		if (errno != EWOULDBLOCK && errno != EAGAIN)
		{
			LOG_ERROR("socket recv: %d", errno);
			readed = -1;
		}
#endif
		else
		{
			readed = 0;
		}
	}
	if (readed > 0)
		socket->rbuffer.position += readed;
	net_buffer_flip(&socket->rbuffer);
	return readed != -1;
}
