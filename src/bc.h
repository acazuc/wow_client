#ifndef BC_H
#define BC_H

#include <stdint.h>

void unpack_bc1(uint32_t width, uint32_t height, const uint8_t *in, uint8_t *out);
void unpack_bc2(uint32_t width, uint32_t height, const uint8_t *in, uint8_t *out);
void unpack_bc3(uint32_t width, uint32_t height, const uint8_t *in, uint8_t *out);
void unpack_bc4(uint32_t width, uint32_t height, const uint8_t *in, uint8_t *out);
void unpack_bc5(uint32_t width, uint32_t height, const uint8_t *in, uint8_t *out);

#endif
