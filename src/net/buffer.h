#ifndef NET_BUFFER_H
#define NET_BUFFER_H

#include <stdbool.h>
#include <stdint.h>

struct net_buffer
{
	uint8_t *data;
	uint32_t position;
	uint32_t limit;
	uint32_t size;
};

bool net_buffer_init(struct net_buffer *buffer, uint32_t capacity);
void net_buffer_destroy(struct net_buffer *buffer);
bool net_buffer_write(struct net_buffer *buffer, const void *src, uint32_t len);
bool net_buffer_read(struct net_buffer *buffer, void *dst, uint32_t len);
void net_buffer_clear(struct net_buffer *buffer);
void net_buffer_flip(struct net_buffer *buffer);

#endif
