#ifndef NET_AUTH_SOCKET_H
#define NET_AUTH_SOCKET_H

#include "net/socket.h"

#include <stdbool.h>

struct net_packet_reader;
struct bignum_ctx;
struct network;
struct bignum;

enum net_auth_phase
{
	NET_AUTH_NONE,
	NET_AUTH_CONNECTING,
	NET_AUTH_CHALLENGE,
	NET_AUTH_PROOF,
	NET_AUTH_REALMLIST
};

struct net_auth_socket
{
	struct net_socket socket;
	struct network *network;
	enum net_auth_phase auth_phase;
	uint8_t srp_m2[20];
	struct bignum_ctx *bn_ctx;
	struct bignum *srp_N;
	struct bignum *srp_g;
	struct bignum *srp_A;
	struct bignum *srp_a;
	struct bignum *srp_B;
	struct bignum *srp_K;
};

struct net_auth_socket *net_auth_socket_new(struct network *network);
void net_auth_socket_delete(struct net_auth_socket *socket);
bool net_auth_socket_tick(struct net_auth_socket *socket);
bool net_auth_socket_auth(struct net_auth_socket *socket, const char *username, const char *password);

#endif
