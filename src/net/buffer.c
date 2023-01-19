#include "buffer.h"

#include "memory.h"
#include "log.h"

#include <inttypes.h>
#include <string.h>

bool net_buffer_init(struct net_buffer *buffer, uint32_t capacity)
{
	buffer->data = mem_malloc(MEM_NET, capacity);
	if (!buffer->data)
	{
		LOG_ERROR("malloc failed");
		return false;
	}
	buffer->position = 0;
	buffer->limit = capacity;
	buffer->size = capacity;
	return true;
}

void net_buffer_destroy(struct net_buffer *buffer)
{
	mem_free(MEM_NET, buffer->data);
}

bool net_buffer_write(struct net_buffer *buffer, const void *src, uint32_t len)
{
	if (!len)
		return true;
	if (buffer->position + len > buffer->limit)
	{
		LOG_ERROR("position = %" PRIu32 "; limit = %" PRIu32 "; len = %" PRIu32, buffer->position, buffer->limit, len);
		return false;
	}
	memmove(&buffer->data[buffer->position], src, len);
	buffer->position += len;
	return true;
}

bool net_buffer_read(struct net_buffer *buffer, void *dst, uint32_t len)
{
	if (buffer->position + len > buffer->limit)
	{
		LOG_ERROR("position = %" PRIu32 "; limit = %" PRIu32 "; len = %" PRIu32, buffer->position, buffer->limit, len);
		return false;
	}
	memmove(dst, &buffer->data[buffer->position], len);
	buffer->position = buffer->position + len;
	return true;
}

void net_buffer_clear(struct net_buffer *buffer)
{
	buffer->limit = buffer->size;
	buffer->position = 0;
}

void net_buffer_flip(struct net_buffer *buffer)
{
	buffer->limit = buffer->position;
	buffer->position = 0;
}
