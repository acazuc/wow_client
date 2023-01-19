#ifndef NET_PACKET_WRITER_H
#define NET_PACKET_WRITER_H

#include <jks/array.h>

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

struct net_packet_reader
{
	uint32_t opcode;
	uint8_t *data;
	uint16_t size;
	uint16_t pos;
};

struct net_packet_writer
{
	uint32_t opcode;
	struct jks_array data; /* uint8_t */
};

bool net_packet_reader_init(struct net_packet_reader *packet, uint32_t opcode, uint8_t *data, uint16_t size);
void net_packet_reader_destroy(struct net_packet_reader *packet);
bool net_read_i8(struct net_packet_reader *packet, int8_t *data);
bool net_read_u8(struct net_packet_reader *packet, uint8_t *data);
bool net_read_i16(struct net_packet_reader *packet, int16_t *data);
bool net_read_u16(struct net_packet_reader *packet, uint16_t *data);
bool net_read_i32(struct net_packet_reader *packet, int32_t *data);
bool net_read_u32(struct net_packet_reader *packet, uint32_t *data);
bool net_read_i64(struct net_packet_reader *packet, int64_t *data);
bool net_read_u64(struct net_packet_reader *packet, uint64_t *data);
bool net_read_flt(struct net_packet_reader *packet, float *data);
bool net_read_dbl(struct net_packet_reader *packet, double *data);
bool net_read_str(struct net_packet_reader *packet, const char **data);
bool net_read_guid(struct net_packet_reader *packet, uint64_t *data);
bool net_read_bytes(struct net_packet_reader *packet, void *dst, size_t size);

void net_packet_writer_init(struct net_packet_writer *packet, uint16_t opcode);
void net_packet_writer_destroy(struct net_packet_writer *packet);
bool net_write_i8(struct net_packet_writer *packet, int8_t data);
bool net_write_i8_off(struct net_packet_writer *packet, int8_t data, size_t position);
bool net_write_u8(struct net_packet_writer *packet, uint8_t data);
bool net_write_u8_off(struct net_packet_writer *packet, uint32_t data, size_t position);
bool net_write_i16(struct net_packet_writer *packet, int16_t data);
bool net_write_i16_off(struct net_packet_writer *packet, int16_t data, size_t position);
bool net_write_u16(struct net_packet_writer *packet, uint16_t data);
bool net_write_u16_off(struct net_packet_writer *packet, uint16_t data, size_t position);
bool net_write_i32(struct net_packet_writer *packet, int32_t data);
bool net_write_i32_off(struct net_packet_writer *packet, int32_t data, size_t position);
bool net_write_u32(struct net_packet_writer *packet, uint32_t data);
bool net_write_u32_off(struct net_packet_writer *packet, uint32_t data, size_t position);
bool net_write_i64(struct net_packet_writer *packet, int64_t data);
bool net_write_i64_off(struct net_packet_writer *packet, int64_t data, size_t position);
bool net_write_u64(struct net_packet_writer *packet, uint64_t data);
bool net_write_u64_off(struct net_packet_writer *packet, uint64_t data, size_t position);
bool net_write_flt(struct net_packet_writer *packet, float data);
bool net_write_flt_off(struct net_packet_writer *packet, float data, size_t position);
bool net_write_dbl(struct net_packet_writer *packet, double data);
bool net_write_dbl_off(struct net_packet_writer *packet, double data, size_t position);
bool net_write_str(struct net_packet_writer *packet, const char *data);
bool net_write_bytes(struct net_packet_writer *packet, const void *src, size_t size);
bool net_write_bytes_off(struct net_packet_writer *packet, const void *src, size_t size, size_t position);
bool net_write_guid(struct net_packet_writer *packet, uint64_t guid);

#endif
