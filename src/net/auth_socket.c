#include "auth_socket.h"

#include "itf/interface.h"

#include "net/network.h"
#include "net/packet.h"

#include "wow_lua.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <jkssl/bignum.h>
#include <jkssl/sha1.h>

#include <inttypes.h>
#include <string.h>

static void set_auth_phase(struct net_auth_socket *socket, enum net_auth_phase auth_phase);
static bool recv_packet(struct net_auth_socket *socket, struct net_packet_reader *packet);

struct net_auth_socket *net_auth_socket_new(struct network *network)
{
	struct net_auth_socket *socket = mem_malloc(MEM_NET, sizeof(*socket));
	if (!socket)
	{
		LOG_ERROR("failed to malloc auth socket");
		return NULL;
	}
	if (!net_socket_init(&socket->socket))
	{
		LOG_ERROR("failed to init auth sockets socket");
		mem_free(MEM_NET, socket);
		return NULL;
	}
	socket->network = network;
	socket->auth_phase = NET_AUTH_NONE;
	socket->bn_ctx = NULL;
	socket->srp_N = NULL;
	socket->srp_g = NULL;
	socket->srp_A = NULL;
	socket->srp_a = NULL;
	socket->srp_B = NULL;
	socket->srp_K = NULL;
	return socket;
}

void net_auth_socket_delete(struct net_auth_socket *socket)
{
	if (!socket)
		return;
	bignum_ctx_free(socket->bn_ctx);
	bignum_free(socket->srp_N);
	bignum_free(socket->srp_g);
	bignum_free(socket->srp_A);
	bignum_free(socket->srp_a);
	bignum_free(socket->srp_B);
	bignum_free(socket->srp_K);
	net_socket_destroy(&socket->socket);
	mem_free(MEM_NET, socket);
}

static void memrev(void *data, size_t size)
{
	for (size_t i = 0; i < size / 2; ++i)
	{
		char tmp = ((char*)data)[i];
		((char*)data)[i] = ((char*)data)[size - i - 1];
		((char*)data)[size - i - 1] = tmp;
	}
}

static struct bignum *gen_srp_x(struct net_auth_socket *auth_socket, struct bignum *srp_s)
{
	uint8_t srp_i[SHA_DIGEST_LENGTH];
	uint8_t hash[SHA_DIGEST_LENGTH];
	struct sha1_ctx ctx;
	char i[512];
	struct bignum *srp_x = NULL;
	size_t srp_s_size;
	uint8_t *srp_s_data = NULL;
	srp_s_size = bignum_num_bytes(srp_s);
	srp_s_data = mem_malloc(MEM_NET, srp_s_size);
	if (!srp_s_data)
	{
		LOG_ERROR("malloc failed");
		goto err;
	}
	snprintf(i, sizeof(i), "%s:%s", auth_socket->network->username, auth_socket->network->password);
	if (!sha1((uint8_t*)i, strlen(i), srp_i))
	{
		LOG_ERROR("sha1 failed");
		goto err;
	}
	if (!bignum_bignum2bin(srp_s, srp_s_data))
	{
		LOG_ERROR("bignum_bignum2bin failed");
		goto err;
	}
	memrev(srp_s_data, srp_s_size);
	if (!sha1_init(&ctx))
	{
		LOG_ERROR("sha1_init failed");
		goto err;
	}
	if (!sha1_update(&ctx, srp_s_data, srp_s_size))
	{
		LOG_ERROR("sha1_update failed");
		goto err;
	}
	if (!sha1_update(&ctx, srp_i, sizeof(srp_i)))
	{
		LOG_ERROR("sha1_update failed");
		goto err;
	}
	if (!sha1_final(hash, &ctx))
	{
		LOG_ERROR("sha1_final failed");
		goto err;
	}
	memrev(hash, sizeof(hash));
	if (!(srp_x = bignum_bin2bignum(hash, sizeof(hash), NULL)))
	{
		LOG_ERROR("bignum_bin2bignum failed");
		goto err;
	}
	mem_free(MEM_NET, srp_s_data);
	return srp_x;
err:
	mem_free(MEM_NET, srp_s_data);
	bignum_free(srp_x);
	return NULL;
}

static struct bignum *gen_srp_S(struct net_auth_socket *auth_socket, struct bignum *srp_x)
{
	uint8_t srp_u_data[SHA_DIGEST_LENGTH];
	uint8_t *srp_A_data = NULL;
	size_t srp_A_size;
	uint8_t *srp_B_data = NULL;
	size_t srp_B_size;
	struct bignum *srp_bkgx = NULL;
	struct bignum *srp_aux = NULL;
	struct bignum *srp_S = NULL;
	struct sha1_ctx ctx;
	srp_A_size = bignum_num_bytes(auth_socket->srp_A);
	srp_B_size = bignum_num_bytes(auth_socket->srp_B);
	srp_A_data = mem_malloc(MEM_NET, srp_A_size);
	if (!srp_A_data)
	{
		LOG_ERROR("malloc failed");
		goto err;
	}
	srp_B_data = mem_malloc(MEM_NET, srp_B_size);
	if (!srp_B_data)
	{
		LOG_ERROR("malloc failed");
		goto err;
	}
	if (!bignum_bignum2bin(auth_socket->srp_A, srp_A_data))
	{
		LOG_ERROR("bignum_bignum2bin failed");
		goto err;
	}
	memrev(srp_A_data, srp_A_size);
	if (!bignum_bignum2bin(auth_socket->srp_B, srp_B_data))
	{
		LOG_ERROR("bignum_bignum2bin failed");
		goto err;
	}
	memrev(srp_B_data, srp_B_size);
	if (!sha1_init(&ctx))
	{
		LOG_ERROR("sha1_init failed");
		goto err;
	}
	if (!sha1_update(&ctx, srp_A_data, srp_A_size))
	{
		LOG_ERROR("sha1_update failed");
		goto err;
	}
	if (!sha1_update(&ctx, srp_B_data, srp_B_size))
	{
		LOG_ERROR("sha1_update failed");
		goto err;
	}
	if (!sha1_final(srp_u_data, &ctx))
	{
		LOG_ERROR("sha1_final failed");
		goto err;
	}
	memrev(srp_u_data, sizeof(srp_u_data));
	if (!(srp_aux = bignum_bin2bignum(srp_u_data, sizeof(srp_u_data), NULL)))
	{
		LOG_ERROR("bignum_bin2bignum failed");
		goto err;
	}
	if (!bignum_mul(srp_aux, srp_aux, srp_x, auth_socket->bn_ctx))
	{
		LOG_ERROR("bignum_mul failed");
		goto err;
	}
	if (!bignum_add(srp_aux, srp_aux, auth_socket->srp_a, auth_socket->bn_ctx))
	{
		LOG_ERROR("bignum_add failed");
		goto err;
	}
	if (!(srp_bkgx = bignum_new()))
	{
		LOG_ERROR("bignum_new failed");
		goto err;
	}
	if (!bignum_mod_exp(srp_bkgx, auth_socket->srp_g, srp_x, auth_socket->srp_N, auth_socket->bn_ctx))
	{
		LOG_ERROR("bignum_mod_exp failed");
		goto err;
	}
	if (!bignum_mul_word(srp_bkgx, srp_bkgx, 3))
	{
		LOG_ERROR("bignum_mul_word failed");
		goto err;
	}
	if (!bignum_sub(srp_bkgx, auth_socket->srp_B, srp_bkgx, auth_socket->bn_ctx))
	{
		LOG_ERROR("bignum_sub failed");
		goto err;
	}
	if (!(srp_S = bignum_new()))
	{
		LOG_ERROR("bignum_new failed");
		goto err;
	}
	if (!bignum_mod_exp(srp_S, srp_bkgx, srp_aux, auth_socket->srp_N, auth_socket->bn_ctx))
	{
		LOG_ERROR("bignum_mod_exp failed");
		goto err;
	}
	mem_free(MEM_NET, srp_A_data);
	mem_free(MEM_NET, srp_B_data);
	bignum_free(srp_aux);
	bignum_free(srp_bkgx);
	return srp_S;
err:
	mem_free(MEM_NET, srp_A_data);
	mem_free(MEM_NET, srp_B_data);
	bignum_free(srp_aux);
	bignum_free(srp_bkgx);
	bignum_free(srp_S);
	return NULL;
}

static struct bignum *gen_srp_K(struct bignum *srp_S)
{
	uint8_t srp_K_data[SHA_DIGEST_LENGTH * 2];
	uint8_t tmp_hash[SHA_DIGEST_LENGTH];
	uint8_t srp_S_data[32];
	uint8_t tmp[16];
	struct bignum *srp_K = NULL;
	if (!bignum_bignum2bin(srp_S, srp_S_data))
	{
		LOG_ERROR("bignum_bignum2bin failed");
		goto err;
	}
	memrev(srp_S_data, sizeof(srp_S_data));
	for (size_t i = 0; i < 16; ++i)
		tmp[i] = srp_S_data[i * 2];
	if (!sha1(tmp, sizeof(tmp), tmp_hash))
	{
		LOG_ERROR("sha1 failed");
		goto err;
	}
	for (size_t i = 0; i < SHA_DIGEST_LENGTH; ++i)
		srp_K_data[i * 2] = tmp_hash[i];
	for (size_t i = 0; i < 16; ++i)
		tmp[i] = srp_S_data[i * 2 + 1];
	if (!sha1(tmp, sizeof(tmp), tmp_hash))
	{
		LOG_ERROR("sha1 failed");
		goto err;
	}
	for (size_t i = 0; i < SHA_DIGEST_LENGTH; ++i)
		srp_K_data[i * 2 + 1] = tmp_hash[i];
	memrev(srp_K_data, sizeof(srp_K_data));
	if (!(srp_K = bignum_bin2bignum(srp_K_data, sizeof(srp_K_data), NULL)))
	{
		LOG_ERROR("bignum_bin2bignum failed");
		goto err;
	}
	return srp_K;
err:
	bignum_free(srp_K);
	return NULL;
}

static bool get_srp_m1(struct net_auth_socket *auth_socket, uint8_t *srp_m1, struct bignum *srp_s)
{
	bool ret = false;
	size_t srp_s_size;
	size_t srp_A_size;
	size_t srp_B_size;
	size_t srp_K_size;
	size_t srp_N_size;
	size_t srp_g_size;
	uint8_t *srp_s_data = NULL;
	uint8_t *srp_A_data = NULL;
	uint8_t *srp_B_data = NULL;
	uint8_t *srp_K_data = NULL;
	uint8_t *srp_N_data = NULL;
	uint8_t *srp_g_data = NULL;
	srp_s_size = bignum_num_bytes(srp_s);
	srp_A_size = bignum_num_bytes(auth_socket->srp_A);
	srp_B_size = bignum_num_bytes(auth_socket->srp_B);
	srp_K_size = bignum_num_bytes(auth_socket->srp_K);
	srp_N_size = bignum_num_bytes(auth_socket->srp_N);
	srp_g_size = bignum_num_bytes(auth_socket->srp_g);
	uint8_t srp_N_hash[SHA_DIGEST_LENGTH];
	uint8_t srp_g_hash[SHA_DIGEST_LENGTH];
	uint8_t srp_I_hash[SHA_DIGEST_LENGTH];
	uint8_t Nxg[SHA_DIGEST_LENGTH];
	struct sha1_ctx ctx;
	srp_s_data = mem_malloc(MEM_NET, srp_s_size);
	if (!srp_s_data)
	{
		LOG_ERROR("malloc failed");
		goto err;
	}
	if (!bignum_bignum2bin(srp_s, srp_s_data))
	{
		LOG_ERROR("bignum_bignum2bin failed");
		goto err;
	}
	memrev(srp_s_data, srp_s_size);
	srp_A_data = mem_malloc(MEM_NET, srp_A_size);
	if (!srp_A_data)
	{
		LOG_ERROR("malloc failed");
		goto err;
	}
	if (!bignum_bignum2bin(auth_socket->srp_A, srp_A_data))
	{
		LOG_ERROR("bignum_bignum2bin failed");
		goto err;
	}
	memrev(srp_A_data, srp_A_size);
	srp_B_data = mem_malloc(MEM_NET, srp_B_size);
	if (!srp_B_data)
	{
		LOG_ERROR("malloc failed");
		goto err;
	}
	if (!bignum_bignum2bin(auth_socket->srp_B, srp_B_data))
	{
		LOG_ERROR("bignum_bignum2bin failed");
		goto err;
	}
	memrev(srp_B_data, srp_B_size);
	srp_K_data = mem_malloc(MEM_NET, srp_K_size);
	if (!srp_K_data)
	{
		LOG_ERROR("malloc failed");
		goto err;
	}
	if (!bignum_bignum2bin(auth_socket->srp_K, srp_K_data))
	{
		LOG_ERROR("bignumm_bignum2bin failed");
		goto err;
	}
	memrev(srp_K_data, srp_K_size);
	if (!sha1((uint8_t*)auth_socket->network->username, strlen(auth_socket->network->username), srp_I_hash))
	{
		LOG_ERROR("sha1 failed");
		goto err;
	}
	srp_N_data = mem_malloc(MEM_NET, srp_N_size);
	if (!srp_N_data)
	{
		LOG_ERROR("malloc failed");
		goto err;
	}
	if (!bignum_bignum2bin(auth_socket->srp_N, srp_N_data))
	{
		LOG_ERROR("bignum_bignum2bin failed");
		goto err;
	}
	memrev(srp_N_data, srp_N_size);
	srp_g_data = mem_malloc(MEM_NET, srp_g_size);
	if (!srp_g_data)
	{
		LOG_ERROR("malloc failed");
		goto err;
	}
	if (!bignum_bignum2bin(auth_socket->srp_g, srp_g_data))
	{
		LOG_ERROR("bignum_bignum2bin failed");
		goto err;
	}
	memrev(srp_g_data, srp_g_size);
	if (!sha1(srp_N_data, srp_N_size, srp_N_hash))
	{
		LOG_ERROR("sha1 failed");
		goto err;
	}
	if (!sha1(srp_g_data, srp_g_size, srp_g_hash))
	{
		LOG_ERROR("sha1 failed");
		goto err;
	}
	for (size_t i = 0; i < sizeof(Nxg); ++i)
		Nxg[i] = srp_N_hash[i] ^ srp_g_hash[i];
	if (!sha1_init(&ctx))
	{
		LOG_ERROR("sha1_init failed");
		goto err;
	}
	if (!sha1_update(&ctx, Nxg, sizeof(Nxg)))
	{
		LOG_ERROR("sha1_update failed");
		goto err;
	}
	if (!sha1_update(&ctx, srp_I_hash, sizeof(srp_I_hash)))
	{
		LOG_ERROR("sha1_update failed");
		goto err;
	}
	if (!sha1_update(&ctx, srp_s_data, srp_s_size))
	{
		LOG_ERROR("sha1_update failed");
		goto err;
	}
	if (!sha1_update(&ctx, srp_A_data, srp_A_size))
	{
		LOG_ERROR("sha1_update failed");
		goto err;
	}
	if (!sha1_update(&ctx, srp_B_data, srp_B_size))
	{
		LOG_ERROR("sha1_update failed");
		goto err;
	}
	if (!sha1_update(&ctx, srp_K_data, srp_K_size))
	{
		LOG_ERROR("sha1_update failed");
		goto err;
	}
	if (!sha1_final(srp_m1, &ctx))
	{
		LOG_ERROR("sha1_final failed");
		goto err;
	}
	ret = true;
err:
	mem_free(MEM_NET, srp_s_data);
	mem_free(MEM_NET, srp_A_data);
	mem_free(MEM_NET, srp_B_data);
	mem_free(MEM_NET, srp_K_data);
	mem_free(MEM_NET, srp_N_data);
	mem_free(MEM_NET, srp_g_data);
	return ret;
}

static bool get_srp_m2(struct net_auth_socket *auth_socket, uint8_t *srp_m2, uint8_t *srp_m1)
{
	bool ret = false;
	size_t srp_A_size;
	size_t srp_K_size;
	uint8_t *srp_A_data = NULL;
	uint8_t *srp_K_data = NULL;
	srp_A_size = bignum_num_bytes(auth_socket->srp_A);
	srp_K_size = bignum_num_bytes(auth_socket->srp_K);
	struct sha1_ctx ctx;
	srp_A_data = mem_malloc(MEM_NET, srp_A_size);
	if (!srp_A_data)
	{
		LOG_ERROR("malloc failed");
		goto err;
	}
	if (!bignum_bignum2bin(auth_socket->srp_A, srp_A_data))
	{
		LOG_ERROR("bignum_bignum2bin failed");
		goto err;
	}
	memrev(srp_A_data, srp_A_size);
	srp_K_data = mem_malloc(MEM_NET, srp_K_size);
	if (!srp_K_data)
	{
		LOG_ERROR("malloc failed");
		goto err;
	}
	if (!bignum_bignum2bin(auth_socket->srp_K, srp_K_data))
	{
		LOG_ERROR("bignum_bignum2bin failed");
		goto err;
	}
	memrev(srp_K_data, srp_K_size);
	if (!sha1_init(&ctx))
	{
		LOG_ERROR("sha1_init failed");
		goto err;
	}
	if (!sha1_update(&ctx, srp_A_data, srp_A_size))
	{
		LOG_ERROR("sha1_update failed");
		goto err;
	}
	if (!sha1_update(&ctx, srp_m1, SHA_DIGEST_LENGTH))
	{
		LOG_ERROR("sha1_update failed");
		goto err;
	}
	if (!sha1_update(&ctx, srp_K_data, srp_K_size))
	{
		LOG_ERROR("sha1_update failed");
		goto err;
	}
	if (!sha1_final(srp_m2, &ctx))
	{
		LOG_ERROR("sha1_final failed");
		goto err;
	}
	ret = true;
err:
	mem_free(MEM_NET, srp_A_data);
	mem_free(MEM_NET, srp_K_data);
	return ret;
}

static bool handle_challenge(struct net_auth_socket *socket, struct net_packet_reader *packet)
{
	uint8_t srp_B_data[32];
	uint8_t srp_g_size;
	uint8_t srp_N_size;
	size_t srp_A_size;
	uint8_t *srp_g_data = NULL;
	uint8_t *srp_N_data = NULL;
	uint8_t *srp_A_data = NULL;
	uint8_t srp_s_data[32];
	uint8_t challenge[16];
	uint8_t srp_m1[SHA_DIGEST_LENGTH];
	struct bignum *srp_x = NULL;
	struct bignum *srp_s = NULL;
	struct bignum *srp_S = NULL;
	uint8_t *data = NULL;
	size_t data_size;
	size_t data_pos;
	uint8_t opcode;
	uint8_t secure_flags;
	uint8_t unk;
	uint8_t error;
	if (!net_read_u8(packet, &opcode))
		goto err;
	if (opcode != 0)
	{
		LOG_ERROR("opcode != 0");
		goto err;
	}
	if (!net_read_u8(packet, &error))
		goto err;
	if (error != 0)
	{
		LOG_ERROR("error != 0");
		goto err;
	}
	if (!net_read_u8(packet, &unk)
	 || !net_read_bytes(packet, srp_B_data, sizeof(srp_B_data)))
		goto err;
	memrev(srp_B_data, sizeof(srp_B_data));
	if (!net_read_u8(packet, &srp_g_size))
		goto err;
	srp_g_data = mem_malloc(MEM_NET, srp_g_size);
	if (!srp_g_data)
	{
		LOG_ERROR("malloc failed");
		goto err;
	}
	if (!net_read_bytes(packet, srp_g_data, srp_g_size))
		goto err;
	memrev(srp_g_data, srp_g_size);
	if (!net_read_u8(packet, &srp_N_size))
		goto err;
	srp_N_data = mem_malloc(MEM_NET, srp_N_size);
	if (!srp_N_data)
	{
		LOG_ERROR("malloc failed");
		goto err;
	}
	if (!net_read_bytes(packet, srp_N_data, srp_N_size))
		goto err;
	memrev(srp_N_data, srp_N_size);
	if (!net_read_bytes(packet, srp_s_data, sizeof(srp_s_data)))
		goto err;
	memrev(srp_s_data, sizeof(srp_s_data));
	if (!net_read_bytes(packet, challenge, sizeof(challenge)))
		goto err;
	memrev(challenge, sizeof(challenge));
	if (!net_read_u8(packet, &secure_flags))
		goto err;
	if (!(socket->bn_ctx = bignum_ctx_new()))
	{
		LOG_ERROR("bignum_ctx_new failed");
		goto err;
	}
	if (!(socket->srp_B = bignum_bin2bignum(srp_B_data, sizeof(srp_B_data), NULL)))
	{
		LOG_ERROR("bignum_bin2bignum failed");
		goto err;
	}
	if (!(socket->srp_N = bignum_bin2bignum(srp_N_data, srp_N_size, NULL)))
	{
		LOG_ERROR("bignum_bin2bignumm failed");
		goto err;
	}
	if (!(socket->srp_g = bignum_bin2bignum(srp_g_data, srp_g_size, NULL)))
	{
		LOG_ERROR("bignum_bin2bignum failed");
		goto err;
	}
	if (!(srp_s = bignum_bin2bignum(srp_s_data, sizeof(srp_s_data), NULL)))
	{
		LOG_ERROR("bignum_bin2bignum failed");
		goto err;
	}
	if (!(socket->srp_a = bignum_new()))
	{
		LOG_ERROR("bignum_new failed");
		goto err;
	}
	if (!(socket->srp_A = bignum_new()))
	{
		LOG_ERROR("bignum_new failed");
		goto err;
	}
	if (!bignum_rand(socket->srp_a, 152, BIGNUM_RAND_TOP_TWO, BIGNUM_RAND_BOT_ODD))
	{
		LOG_ERROR("bignum_rand failed");
		goto err;
	}
	if (!bignum_mod_exp(socket->srp_A, socket->srp_g, socket->srp_a, socket->srp_N, socket->bn_ctx))
	{
		LOG_ERROR("bignum_mod_exp failed");
		goto err;
	}
	if (!(srp_x = gen_srp_x(socket, srp_s)))
		goto err;
	srp_A_size = bignum_num_bytes(socket->srp_A);
	srp_A_data = mem_malloc(MEM_NET, srp_A_size);
	if (!srp_A_data)
	{
		LOG_ERROR("malloc failed");
		goto err;
	}
	if (!bignum_bignum2bin(socket->srp_A, srp_A_data))
	{
		LOG_ERROR("bignum_bignum2bin failed");
		goto err;
	}
	memrev(srp_A_data, srp_A_size);
	if (!(srp_S = gen_srp_S(socket, srp_x)))
		goto err;
	if (!(socket->srp_K = gen_srp_K(srp_S)))
		goto err;
	if (!get_srp_m1(socket, srp_m1, srp_s))
		goto err;
	if (!get_srp_m2(socket, socket->srp_m2, srp_m1))
		goto err;
	data_size = 1 + srp_A_size + sizeof(srp_m1) + 20 + 2;
	data = mem_malloc(MEM_NET, data_size);
	if (!data)
	{
		LOG_ERROR("malloc failed");
		goto err;
	}
	data_pos = 0;
	data[data_pos++] = 1;
	for (size_t i = 0; i < srp_A_size; ++i)
		data[data_pos++] = srp_A_data[i];
	for (size_t i = 0; i < sizeof(srp_m1); ++i)
		data[data_pos++] = srp_m1[i];
	for (size_t i = 0; i < 20; ++i)
		data[data_pos++] = 0; /* CRC ? */
	data[data_pos++] = 0;
	data[data_pos++] = 0;
	if (!net_buffer_write(&socket->socket.wbuffer, data, data_size))
		goto err;
	set_auth_phase(socket, NET_AUTH_PROOF);
	mem_free(MEM_NET, srp_g_data);
	mem_free(MEM_NET, srp_N_data);
	mem_free(MEM_NET, srp_A_data);
	mem_free(MEM_NET, data);
	return true;
err:
	mem_free(MEM_NET, srp_g_data);
	mem_free(MEM_NET, srp_N_data);
	mem_free(MEM_NET, srp_A_data);
	mem_free(MEM_NET, data);
	bignum_free(socket->srp_N);
	socket->srp_N = NULL;
	bignum_free(socket->srp_g);
	socket->srp_g = NULL;
	bignum_free(socket->srp_A);
	socket->srp_A = NULL;
	bignum_free(socket->srp_a);
	socket->srp_a = NULL;
	bignum_free(socket->srp_B);
	socket->srp_B = NULL;
	bignum_free(srp_x);
	bignum_free(srp_s);
	return false;
}

static bool handle_proof(struct net_auth_socket *socket, struct net_packet_reader *packet)
{
	uint8_t opcode;
	if (!net_read_u8(packet, &opcode))
		return false;
	if (opcode != 1)
	{
		LOG_ERROR("opcode 1 expected");
		return false;
	}
	uint8_t error;
	if (!net_read_u8(packet, &error))
		return false;
	if (error != 0)
	{
		switch (error)
		{
			case 0x4:
				LOG_ERROR("login failed");
				lua_pushnil(g_wow->interface->L);
				lua_getglobal(g_wow->interface->L, "LOGIN_STATE_FAILED");
				interface_execute_event(g_wow->interface, EVENT_SHOW_SERVER_ALERT, 1);
				break;
			default:
				LOG_ERROR("unknown error: %" PRIu8, error);
				break;
		}
		return false;
	}
	uint8_t m2[SHA_DIGEST_LENGTH];
	uint32_t unk1;
	uint32_t unk2;
	uint16_t unk3;
	if (!net_read_bytes(packet, m2, sizeof(m2))
	 || !net_read_u32(packet, &unk1)
	 || !net_read_u32(packet, &unk2)
	 || !net_read_u16(packet, &unk3))
		return false;
	if (memcmp(m2, socket->srp_m2, sizeof(m2)))
	{
		LOG_ERROR("invalid server proof");
		return false;
	}
	uint8_t data[5] = {0x10, 0, 0, 0, 0};
	if (!net_buffer_write(&socket->socket.wbuffer, data, sizeof(data)))
		return false;
	set_auth_phase(socket, NET_AUTH_REALMLIST);
	return true;
}

static bool handle_realm_list(struct net_auth_socket *socket, struct net_packet_reader *packet)
{
	uint8_t opcode;
	if (!net_read_u8(packet, &opcode))
		return false;
	if (opcode != 0x10)
	{
		LOG_ERROR("opcode 0x10 expected");
		return false;
	}
	uint16_t unk1;
	uint32_t unk2;
	uint16_t realms;
	if (!net_read_u16(packet, &unk1)
	 || !net_read_u32(packet, &unk2)
	 || !net_read_u16(packet, &realms))
		return false;
	for (uint16_t i = 0; i < realms; ++i)
	{
		struct world_server world_server;
		uint8_t ok;
		if (!net_read_u8(packet, &world_server.icon)
		 || !net_read_u8(packet, &world_server.locked)
		 || !net_read_u8(packet, &world_server.flags)
		 || !net_read_str(packet, (const char**)&world_server.name)
		 || !net_read_str(packet, (const char**)&world_server.host)
		 || !net_read_flt(packet, &world_server.load)
		 || !net_read_u8(packet, &world_server.characters)
		 || !net_read_u8(packet, &world_server.timezone)
		 || !net_read_u8(packet, &ok))
			return false;
		LOG_INFO("icon: %" PRIu8 ", locked: %" PRIu8 ", flags: %" PRIu8 ", name: %s, host: %s, load: %f, chars: %" PRIu8 ", timezone: %" PRIu8 ", ok: %" PRIu8, world_server.icon, world_server.locked, world_server.flags, world_server.name, world_server.host, world_server.load, world_server.characters, world_server.timezone, ok);
		world_server.name = mem_strdup(MEM_NET, world_server.name);
		world_server.host = mem_strdup(MEM_NET, world_server.host);
		if (!world_server.name || !world_server.host)
		{
			LOG_ERROR("failed to duplicate name or host");
			mem_free(MEM_NET, world_server.name);
			mem_free(MEM_NET, world_server.host);
			return false;
		}
		if (!net_add_world_server(g_wow->network, &world_server))
		{
			mem_free(MEM_NET, world_server.name);
			mem_free(MEM_NET, world_server.host);
			return false;
		}
	}
	uint16_t unk3;
	if (!net_read_u16(packet, &unk3))
		return false;
	set_auth_phase(socket, NET_AUTH_NONE);
	uint8_t srp_K_data[40];
	if (!bignum_bignum2bin(socket->srp_K, srp_K_data))
	{
		LOG_ERROR("bignum_bignum2bin failed");
		return false;
	}
	memrev(srp_K_data, sizeof(srp_K_data));
	net_set_auth_key(g_wow->network, srp_K_data);
	/* interface_execute_event(g_wow->interface, EVENT_OPEN_REALM_LIST, 0); */
	interface_execute_event(g_wow->interface, EVENT_SUGGEST_REALM, 0);
	return true;
}

bool net_auth_socket_tick(struct net_auth_socket *socket)
{
	switch (socket->auth_phase)
	{
		case NET_AUTH_NONE:
			return true;
		case NET_AUTH_CONNECTING:
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
					LOG_DEBUG("connected");
					if (!net_auth_socket_auth(socket, socket->network->username, socket->network->password))
					{
						LOG_ERROR("auth() failed");
						return false;
					}
					return true;
			}
			return true;
		}
		case NET_AUTH_CHALLENGE:
		{
			if (!net_socket_recv(&socket->socket))
			{
				LOG_ERROR("recv() failed");
				return false;
			}
			struct net_packet_reader packet;
			if (recv_packet(socket, &packet))
			{
				if (!handle_challenge(socket, &packet))
					LOG_ERROR("handle_challenge failed");
				net_packet_reader_destroy(&packet);
			}
			if (!net_socket_send(&socket->socket))
			{
				LOG_ERROR("send() failed");
				return false;
			}
			return true;
		}
		case NET_AUTH_PROOF:
		{
			if (!net_socket_recv(&socket->socket))
			{
				LOG_ERROR("recv() failed");
				return false;
			}
			struct net_packet_reader packet;
			if (recv_packet(socket, &packet))
			{
				if (!handle_proof(socket, &packet))
				{
					LOG_ERROR("handle proof failed");
					net_packet_reader_destroy(&packet);
					return false;
				}
				net_packet_reader_destroy(&packet);
			}
			if (!net_socket_send(&socket->socket))
			{
				LOG_ERROR("send() failed");
				return false;
			}
			return true;
		}
		case NET_AUTH_REALMLIST:
		{
			if (!net_socket_recv(&socket->socket))
			{
				LOG_ERROR("recv() failed");
				return false;
			}
			struct net_packet_reader packet;
			if (recv_packet(socket, &packet))
			{
				bool ret = handle_realm_list(socket, &packet);
				net_packet_reader_destroy(&packet);
				if (ret)
					return true;
			}
			if (!net_socket_send(&socket->socket))
			{
				LOG_ERROR("send() failed");
				return false;
			}
			return true;
		}
	}
	return false; /* unhandled */
}

bool net_auth_socket_auth(struct net_auth_socket *socket, const char *username, const char *password)
{
	(void)username;
	(void)password;
	if (socket->auth_phase != NET_AUTH_CONNECTING)
	{
		LOG_DEBUG("auth phase already engaged");
		return false;
	}
	set_auth_phase(socket, NET_AUTH_CHALLENGE);
	uint8_t data[300];
	size_t data_pos = 0;
	data[data_pos++] = 0; /* opcode */
	data[data_pos++] = 8; /* error */
	data[data_pos++] = 0; /* size l */
	data[data_pos++] = 0; /* size h */
	data[data_pos++] = 'W';
	data[data_pos++] = 'O';
	data[data_pos++] = 'W';
	data[data_pos++] = '\0'; /* game */
	data[data_pos++] = 2; /* version major */
	data[data_pos++] = 4; /* version minor */
	data[data_pos++] = 3; /* version patch */
	data[data_pos++] = 0x9E; /* build l */
	data[data_pos++] = 0x21; /* build h */
	data[data_pos++] = '6';
	data[data_pos++] = '8';
	data[data_pos++] = 'x';
	data[data_pos++] = '\0'; /* platform */
	data[data_pos++] = 'n';
	data[data_pos++] = 'i';
	data[data_pos++] = 'W';
	data[data_pos++] = '\0'; /* os */
	data[data_pos++] = 'R';
	data[data_pos++] = 'F';
	data[data_pos++] = 'r';
	data[data_pos++] = 'f'; /* lang */
	data[data_pos++] = 0x3C;
	data[data_pos++] = 0;
	data[data_pos++] = 0;
	data[data_pos++] = 0; /* timezone */
	data[data_pos++] = 0x7f;
	data[data_pos++] = 0;
	data[data_pos++] = 0;
	data[data_pos++] = 1; /* ip */
	data[data_pos++] = strlen(socket->network->username);
	for (char *tmp = socket->network->username; *tmp; tmp++)
		data[data_pos++] = *tmp;
	data[2] = (data_pos - 4) & 0xff;
	data[3] = (data_pos - 4) / 0x100;
	if (!net_buffer_write(&socket->socket.wbuffer, data, data_pos))
		return false;
	return true;
}

static bool recv_packet(struct net_auth_socket *socket, struct net_packet_reader *packet)
{
	if (socket->socket.rbuffer.position + 3 >= socket->socket.rbuffer.limit)
		return false;
	uint8_t *ptr = socket->socket.rbuffer.data + socket->socket.rbuffer.position;
	uint8_t opcode = *ptr;
	uint16_t len;
	LOG_DEBUG("opcode: %" PRIu8, opcode);
	switch (opcode)
	{
		case 0:
			len = 119;
			break;
		case 1:
			switch (socket->socket.rbuffer.data[socket->socket.rbuffer.position + 1])
			{
				case 0x04:
					len = 3;
					break;
				default:
					len = 32;
					break;
			}
			break;
		case 0x10:
			len = 3;
			len += *(ptr + 1);
			len += (uint16_t)*(ptr + 2) << 8;
			break;
		default:
			LOG_ERROR("invalid opcode: %" PRIu8, opcode);
			len = 1;
			break;
	}
	LOG_DEBUG("pos: %u, len: %u, limit: %u", (unsigned)socket->socket.rbuffer.position, (unsigned)len, (unsigned)socket->socket.rbuffer.limit);
	if (socket->socket.rbuffer.position + len > socket->socket.rbuffer.limit)
	{
		LOG_DEBUG("packet not full");
		return false;
	}
	if (!net_packet_reader_init(packet, opcode, ptr, len))
	{
		LOG_ERROR("failed to init packet reader");
		return false;
	}
	socket->socket.rbuffer.position = socket->socket.rbuffer.position + len;
	return true;
}

static void set_auth_phase(struct net_auth_socket *socket, enum net_auth_phase auth_phase)
{
	if (socket->auth_phase == auth_phase)
		return;
	socket->auth_phase = auth_phase;
	switch (auth_phase)
	{
		case NET_AUTH_NONE:
			break;
		case NET_AUTH_CONNECTING:
			lua_pushnil(g_wow->interface->L);
			lua_getglobal(g_wow->interface->L, "LOGIN_STATE_CONNECTING");
			interface_execute_event(g_wow->interface, EVENT_SHOW_SERVER_ALERT, 1);
			break;
		case NET_AUTH_CHALLENGE:
			lua_pushnil(g_wow->interface->L);
			lua_getglobal(g_wow->interface->L, "LOGIN_STATE_HANDSHAKING");
			interface_execute_event(g_wow->interface, EVENT_SHOW_SERVER_ALERT, 1);
			break;
		case NET_AUTH_PROOF:
			lua_pushnil(g_wow->interface->L);
			lua_getglobal(g_wow->interface->L, "LOGIN_STATE_AUTHENTICATING");
			interface_execute_event(g_wow->interface, EVENT_SHOW_SERVER_ALERT, 1);
			break;
		case NET_AUTH_REALMLIST:
			lua_pushnil(g_wow->interface->L);
			lua_getglobal(g_wow->interface->L, "LOGIN_STATE_AUTHENTICATED");
			interface_execute_event(g_wow->interface, EVENT_SHOW_SERVER_ALERT, 1);
			break;
	}
}
