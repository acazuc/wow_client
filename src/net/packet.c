#include "packet.h"

#include "memory.h"
#include "log.h"

#include <string.h>

MEMORY_DECL(NET);

bool net_packet_reader_init(struct net_packet_reader *packet, uint32_t opcode, uint8_t *data, uint16_t size)
{
	packet->opcode = opcode;
	packet->data = mem_malloc(MEM_NET, size);
	if (!packet->data)
	{
		LOG_ERROR("failed to allocate buffer");
		return false;
	}
	memcpy(packet->data, data, size);
	packet->size = size;
	packet->pos = 0;
	return true;
}

void net_packet_reader_destroy(struct net_packet_reader *packet)
{
	mem_free(MEM_NET, packet->data);
}

bool net_read_i8(struct net_packet_reader *packet, int8_t *data)
{
	return net_read_bytes(packet, data, 1);
}

bool net_read_u8(struct net_packet_reader *packet, uint8_t *data)
{
	return net_read_bytes(packet, data, 1);
}

bool net_read_i16(struct net_packet_reader *packet, int16_t *data)
{
	return net_read_bytes(packet, data, 2);
}

bool net_read_u16(struct net_packet_reader *packet, uint16_t *data)
{
	return net_read_bytes(packet, data, 2);
}

bool net_read_i32(struct net_packet_reader *packet, int32_t *data)
{
	return net_read_bytes(packet, data, 4);
}

bool net_read_u32(struct net_packet_reader *packet, uint32_t *data)
{
	return net_read_bytes(packet, data, 4);
}

bool net_read_i64(struct net_packet_reader *packet, int64_t *data)
{
	return net_read_bytes(packet, data, 8);
}

bool net_read_u64(struct net_packet_reader *packet, uint64_t *data)
{
	return net_read_bytes(packet, data, 8);
}

bool net_read_flt(struct net_packet_reader *packet, float *data)
{
	return net_read_bytes(packet, data, 4);
}

bool net_read_dbl(struct net_packet_reader *packet, double *data)
{
	return net_read_bytes(packet, data, 8);
}

bool net_read_str(struct net_packet_reader *packet, const char **data)
{
	*data = (const char*)&packet->data[packet->pos];
	uint8_t osef;
	while (net_read_u8(packet, &osef))
	{
		if (!osef)
			return true;
	}
	return true;
}

bool net_read_guid(struct net_packet_reader *packet, uint64_t *guid)
{
	uint8_t length;
	if (!net_read_u8(packet, &length))
		return false;
	*guid = 0;
	for (uint8_t i = 0; i < 8; ++i)
	{
		if (length & (1 << i))
		{
			uint8_t tmp;
			if (!net_read_u8(packet, &tmp))
				return false;
			*guid |= (uint64_t)tmp << (8 * i);
		}
	}
	return true;
}

bool net_read_bytes(struct net_packet_reader *packet, void *dst, size_t size)
{
	if (packet->pos + size > packet->size)
	{
		LOG_ERROR("read_bytes underflow: %u + %u > %u", (unsigned)packet->pos, (unsigned)size, (unsigned)packet->size);
		return false;
	}
	memcpy(dst, packet->data + packet->pos, size);
	packet->pos += size;
	return true;
}

void net_packet_writer_init(struct net_packet_writer *packet, uint16_t opcode)
{
	packet->opcode = opcode;
	jks_array_init(&packet->data, sizeof(uint8_t), NULL, &jks_array_memory_fn_NET);
}

void net_packet_writer_destroy(struct net_packet_writer *packet)
{
	jks_array_destroy(&packet->data);
}

bool net_write_i8(struct net_packet_writer *packet, int8_t data)
{
	return net_write_bytes(packet, &data, 1);
}

bool net_write_i8_off(struct net_packet_writer *packet, int8_t data, size_t position)
{
	return net_write_bytes_off(packet, &data, 1, position);
}

bool net_write_u8(struct net_packet_writer *packet, uint8_t data)
{
	return net_write_bytes(packet, &data, 1);
}

bool net_write_u8_off(struct net_packet_writer *packet, uint32_t data, size_t position)
{
	return net_write_bytes_off(packet, &data, 1, position);
}

bool net_write_i16(struct net_packet_writer *packet, int16_t data)
{
	return net_write_bytes(packet, &data, 2);
}

bool net_write_i16_off(struct net_packet_writer *packet, int16_t data, size_t position)
{
	return net_write_bytes_off(packet, &data, 2, position);
}

bool net_write_u16(struct net_packet_writer *packet, uint16_t data)
{
	return net_write_bytes(packet, &data, 2);
}

bool net_write_u16_off(struct net_packet_writer *packet, uint16_t data, size_t position)
{
	return net_write_bytes_off(packet, &data, 2, position);
}

bool net_write_i32(struct net_packet_writer *packet, int32_t data)
{
	return net_write_bytes(packet, &data, 4);
}

bool net_write_i32_off(struct net_packet_writer *packet, int32_t data, size_t position)
{
	return net_write_bytes_off(packet, &data, 4, position);
}

bool net_write_u32(struct net_packet_writer *packet, uint32_t data)
{
	return net_write_bytes(packet, &data, 4);
}

bool net_write_u32_off(struct net_packet_writer *packet, uint32_t data, size_t position)
{
	return net_write_bytes_off(packet, &data, 4, position);
}

bool net_write_i64(struct net_packet_writer *packet, int64_t data)
{
	return net_write_bytes(packet, &data, 8);
}

bool net_write_i64_off(struct net_packet_writer *packet, int64_t data, size_t position)
{
	return net_write_bytes_off(packet, &data, 8, position);
}

bool net_write_u64(struct net_packet_writer *packet, uint64_t data)
{
	return net_write_bytes(packet, &data, 8);
}

bool net_write_u64_off(struct net_packet_writer *packet, uint64_t data, size_t position)
{
	return net_write_bytes_off(packet, &data, 8, position);
}

bool net_write_flt(struct net_packet_writer *packet, float data)
{
	return net_write_bytes(packet, &data, 4);
}

bool net_write_flt_off(struct net_packet_writer *packet, float data, size_t position)
{
	return net_write_bytes_off(packet, &data, 4, position);
}

bool net_write_dbl(struct net_packet_writer *packet, double data)
{
	return net_write_bytes(packet, &data, 8);
}

bool net_write_dbl_off(struct net_packet_writer *packet, double data, size_t position)
{
	return net_write_bytes_off(packet, &data, 8, position);
}

bool net_write_str(struct net_packet_writer *packet, const char *data)
{
	if (data)
	{
		if (!net_write_bytes(packet, data, strlen(data)))
			return false;
	}
	return net_write_i8(packet, 0);
}

bool net_write_guid(struct net_packet_writer *packet, uint64_t guid)
{
	size_t pos = packet->data.size;
	if (!net_write_u8(packet, 0))
		return false;
	uint16_t length = 0;
	for (uint8_t i = 0; guid != 0; ++i)
	{
		if (guid & 0xFF)
		{
			length |= (uint8_t)(1 << i);
			if (!net_write_u8(packet, guid & 0xFF))
				return false;
		}
		guid >>= 8;
	}
	return net_write_u8_off(packet, length, pos);
}

bool net_write_bytes(struct net_packet_writer *packet, const void *src, size_t size)
{
	if (!size)
		return true;
	void *dst = jks_array_grow(&packet->data, size);
	if (!dst)
	{
		LOG_ERROR("failed to grow write buffer for opcode %u (%u + %u)", (unsigned)packet->opcode, (unsigned)packet->data.size, (unsigned)size);
		return false;
	}
	memcpy(dst, src, size);
	return true;
}

bool net_write_bytes_off(struct net_packet_writer *packet, const void *src, size_t size, size_t position)
{
	if (position + size > packet->data.size)
	{
		if (!jks_array_grow(&packet->data, position + size - packet->data.size))
		{
			LOG_ERROR("failed to grow write buffer for opcode %u (%u + %u)", (unsigned)packet->opcode, (unsigned)packet->data.size, (unsigned)(position + size - packet->data.size));
			return false;
		}
	}
	uint8_t *dst = JKS_ARRAY_GET(&packet->data, position, uint8_t);
	memcpy(dst, src, size);
	return true;
}
