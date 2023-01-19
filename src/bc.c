#include "bc.h"

#define RGB5TO8(v) ((((v) * 527) + 23) >> 6)
#define RGB6TO8(v) ((((v) * 259) + 33) >> 6)

static void unpack_bc1_block(uint32_t bx, uint32_t by, uint32_t width, uint32_t height, const uint8_t *in, uint8_t *out)
{
	(void)height;
	uint32_t idx = (by * width + bx) * 4;
	uint32_t idx_inc = width * 4 - 16;
	uint32_t color_bits = in[4] | (in[5] << 8) | (in[6] << 16) | (in[7] << 24);
	uint16_t color1 = (in[1] << 8) | in[0];
	uint8_t r1 = RGB5TO8(in[1] >> 3);
	uint8_t g1 = RGB6TO8((color1 >> 5) & 0x3F);
	uint8_t b1 = RGB5TO8(in[0] & 0x1F);
	uint16_t color2 = (in[3] << 8) | in[2];
	uint8_t r2 = RGB5TO8(in[3] >> 3);
	uint8_t g2 = RGB6TO8((color2 >> 5) & 0x3F);
	uint8_t b2 = RGB5TO8(in[2] & 0x1F);
	for (uint32_t y = 0; y < 4; ++y)
	{
		for (uint32_t x = 0; x < 4; ++x)
		{
			switch (color_bits & 3)
			{
				case 0:
					out[idx++] = r1;
					out[idx++] = g1;
					out[idx++] = b1;
					out[idx++] = 0xff;
					break;
				case 1:
					out[idx++] = r2;
					out[idx++] = g2;
					out[idx++] = b2;
					out[idx++] = 0xff;
					break;
				case 2:
					if (color1 > color2)
					{
						out[idx++] = (2 * r1 + r2) / 3;
						out[idx++] = (2 * g1 + g2) / 3;
						out[idx++] = (2 * b1 + b2) / 3;
						out[idx++] = 0xff;
					}
					else
					{
						out[idx++] = (r1 + r2) / 2;
						out[idx++] = (g1 + g2) / 2;
						out[idx++] = (b1 + b2) / 2;
						out[idx++] = 0xff;
					}
					break;
				case 3:
					if (color1 > color2)
					{
						out[idx++] = (2 * r2 + r1) / 3;
						out[idx++] = (2 * g2 + g1) / 3;
						out[idx++] = (2 * b2 + b1) / 3;
						out[idx++] = 0xff;
					}
					else
					{
						out[idx++] = 0;
						out[idx++] = 0;
						out[idx++] = 0;
						out[idx++] = 0;
					}
					break;
			}
			color_bits >>= 2;
		}
		idx += idx_inc;
	}
}

void unpack_bc1(uint32_t width, uint32_t height, const uint8_t *in, uint8_t *out)
{
	uint32_t bx = (width + 3) & (~0x3);
	uint32_t by = (height + 3) & (~0x3);
	for (uint32_t y = 0; y < by; y += 4)
	{
		for (uint32_t x = 0; x < bx; x += 4)
		{
			unpack_bc1_block(x, y, width, height, in, out);
			in += 8;
		}
	}
}

static void unpack_bc2_block(uint32_t bx, uint32_t by, uint32_t width, uint32_t height, const uint8_t *in, uint8_t *out)
{
	(void)height;
	uint32_t idx = (by * width + bx) * 4;
	uint32_t idx_inc = width * 4 - 16;
	uint32_t color_bits = in[12] | (in[13] << 8) | (in[14] << 16) | (in[15] << 24);
	uint16_t color1 = (in[9] << 8) | in[8];
	uint8_t r1 = RGB5TO8(in[9] >> 3);
	uint8_t g1 = RGB6TO8((color1 >> 5) & 0x3F);
	uint8_t b1 = RGB5TO8(in[8] & 0x1F);
	uint16_t color2 = (in[11] << 8) | in[10];
	uint8_t r2 = RGB5TO8(in[11] >> 3);
	uint8_t g2 = RGB6TO8((color2 >> 5) & 0x3F);
	uint8_t b2 = RGB5TO8(in[10] & 0x1F);
	uint32_t alpha_idx = 0;
	for (uint32_t y = 0; y < 4; ++y)
	{
		for (uint32_t x = 0; x < 4; ++x)
		{
			switch (color_bits & 3)
			{
				case 0:
					out[idx++] = r1;
					out[idx++] = g1;
					out[idx++] = b1;
					break;
				case 1:
					out[idx++] = r2;
					out[idx++] = g2;
					out[idx++] = b2;
					break;
				case 2:
					out[idx++] = (2 * r1 + r2) / 3;
					out[idx++] = (2 * g1 + g2) / 3;
					out[idx++] = (2 * b1 + b2) / 3;
					break;
				case 3:
					out[idx++] = (2 * r2 + r1) / 3;
					out[idx++] = (2 * g2 + g1) / 3;
					out[idx++] = (2 * b2 + b1) / 3;
					break;
			}
			color_bits >>= 2;
			if (x & 1)
			{
				out[idx] = in[alpha_idx++] & 0xF0;
				out[idx] |= out[idx] >> 4;
			}
			else
			{
				out[idx] = in[alpha_idx] & 0x0F;
				out[idx] |= out[idx] << 4;
			}
			idx++;
		}
		idx += idx_inc;
	}
}

void unpack_bc2(uint32_t width, uint32_t height, const uint8_t *in, uint8_t *out)
{
	uint32_t bx = (width + 3) & (~0x3);
	uint32_t by = (height + 3) & (~0x3);
	for (uint32_t y = 0; y < by; y += 4)
	{
		for (uint32_t x = 0; x < bx; x += 4)
		{
			unpack_bc2_block(x, y, width, height, in, out);
			in += 16;
		}
	}
}

static void unpack_bc3_block(uint32_t bx, uint32_t by, uint32_t width, uint32_t height, const uint8_t *in, uint8_t *out)
{
	(void)height;
	uint32_t idx = (by * width + bx) * 4;
	uint32_t idx_inc = width * 4 - 16;
	uint32_t color_bits = in[12] | (in[13] << 8) | (in[14] << 16) | (in[15] << 24);
	uint16_t color1 = (in[9] << 8) | in[8];
	uint8_t r1 = RGB5TO8(in[9] >> 3);
	uint8_t g1 = RGB6TO8((color1 >> 5) & 0x3F);
	uint8_t b1 = RGB5TO8(in[8] & 0x1F);
	uint16_t color2 = (in[11] << 8) | in[10];
	uint8_t r2 = RGB5TO8(in[11] >> 3);
	uint8_t g2 = RGB6TO8((color2 >> 5) & 0x3F);
	uint8_t b2 = RGB5TO8(in[10] & 0x1F);
	uint32_t alpha_bits1 = (in[4] << 16) | (in[3] << 8) | in[2];
	uint32_t alpha_bits2 = (in[7] << 16) | (in[6] << 8) | in[5];
	uint8_t alphas[8];
	alphas[0] = in[0];
	alphas[1] = in[1];
	if (alphas[0] > alphas[1])
	{
		alphas[2] = (6 * alphas[0] + 1 * alphas[1]) / 7;
		alphas[3] = (5 * alphas[0] + 2 * alphas[1]) / 7;
		alphas[4] = (4 * alphas[0] + 3 * alphas[1]) / 7;
		alphas[5] = (3 * alphas[0] + 4 * alphas[1]) / 7;
		alphas[6] = (2 * alphas[0] + 5 * alphas[1]) / 7;
		alphas[7] = (1 * alphas[0] + 6 * alphas[1]) / 7;
	}
	else
	{
		alphas[2] = (4 * alphas[0] + 1 * alphas[1]) / 5;
		alphas[3] = (3 * alphas[0] + 2 * alphas[1]) / 5;
		alphas[4] = (2 * alphas[0] + 3 * alphas[1]) / 5;
		alphas[5] = (1 * alphas[0] + 4 * alphas[1]) / 5;
		alphas[6] = 0;
		alphas[7] = 0xFF;
	}
	for (uint32_t y = 0; y < 4; ++y)
	{
		for (uint32_t x = 0; x < 4; ++x)
		{
			switch (color_bits & 3)
			{
				case 0:
					out[idx++] = r1;
					out[idx++] = g1;
					out[idx++] = b1;
					break;
				case 1:
					out[idx++] = r2;
					out[idx++] = g2;
					out[idx++] = b2;
					break;
				case 2:
					out[idx++] = (2 * r1 + r2) / 3;
					out[idx++] = (2 * g1 + g2) / 3;
					out[idx++] = (2 * b1 + b2) / 3;
					break;
				case 3:
					out[idx++] = (2 * r2 + r1) / 3;
					out[idx++] = (2 * g2 + g1) / 3;
					out[idx++] = (2 * b2 + b1) / 3;
					break;
			}
			color_bits >>= 2;
			if (y < 2)
			{
				out[idx++] = alphas[alpha_bits1 & 7];
				alpha_bits1 >>= 3;
			}
			else
			{
				out[idx++] = alphas[alpha_bits2 & 7];
				alpha_bits2 >>= 3;
			}
		}
		idx += idx_inc;
	}
}

void unpack_bc3(uint32_t width, uint32_t height, const uint8_t *in, uint8_t *out)
{
	uint32_t bx = (width + 3) & (~0x3);
	uint32_t by = (height + 3) & (~0x3);
	for (uint32_t y = 0; y < by; y += 4)
	{
		for (uint32_t x = 0; x < bx; x += 4)
		{
			unpack_bc3_block(x, y, width, height, in, out);
			in += 16;
		}
	}
}

static void unpack_bc4_block(uint32_t bx, uint32_t by, uint32_t width, uint32_t height, const uint8_t *in, uint8_t *out)
{
	(void)height;
	uint32_t idx = by * width + bx;
	uint32_t idx_inc = width - 4;
	uint32_t red_bits1 = (in[4] << 16) | (in[3] << 8) | in[2];
	uint32_t red_bits2 = (in[7] << 16) | (in[6] << 8) | in[5];
	uint8_t reds[8];
	reds[0] = in[0];
	reds[1] = in[1];
	if (reds[0] > reds[1])
	{
		reds[2] = (6 * reds[0] + 1 * reds[1]) / 7;
		reds[3] = (5 * reds[0] + 2 * reds[1]) / 7;
		reds[4] = (4 * reds[0] + 3 * reds[1]) / 7;
		reds[5] = (3 * reds[0] + 4 * reds[1]) / 7;
		reds[6] = (2 * reds[0] + 5 * reds[1]) / 7;
		reds[7] = (1 * reds[0] + 6 * reds[1]) / 7;
	}
	else
	{
		reds[2] = (4 * reds[0] + 1 * reds[1]) / 5;
		reds[3] = (3 * reds[0] + 2 * reds[1]) / 5;
		reds[4] = (2 * reds[0] + 3 * reds[1]) / 5;
		reds[5] = (1 * reds[0] + 4 * reds[1]) / 5;
		reds[6] = 0;
		reds[7] = 0xFF;
	}
	for (uint32_t y = 0; y < 4; ++y)
	{
		for (uint32_t x = 0; x < 4; ++x)
		{
			if (y < 2)
			{
				out[idx++] = reds[red_bits1 & 7];
				red_bits1 >>= 3;
			}
			else
			{
				out[idx++] = reds[red_bits2 & 7];
				red_bits2 >>= 3;
			}
		}
		idx += idx_inc;
	}
}

void unpack_bc4(uint32_t width, uint32_t height, const uint8_t *in, uint8_t *out)
{
	uint32_t bx = (width + 3) & (~0x3);
	uint32_t by = (height + 3) & (~0x3);
	for (uint32_t y = 0; y < by; y += 4)
	{
		for (uint32_t x = 0; x < bx; x += 4)
		{
			unpack_bc4_block(x, y, width, height, in, out);
			in += 8;
		}
	}
}

static void unpack_bc5_block(uint32_t bx, uint32_t by, uint32_t width, uint32_t height, const uint8_t *in, uint8_t *out)
{
	(void)height;
	uint32_t idx = (by * width + bx) * 2;
	uint32_t idx_inc = width * 2 - 8;
	uint32_t red_bits1 = (in[4] << 16) | (in[3] << 8) | in[2];
	uint32_t red_bits2 = (in[7] << 16) | (in[6] << 8) | in[5];
	uint8_t reds[8];
	reds[0] = in[0];
	reds[1] = in[1];
	if (reds[0] > reds[1])
	{
		reds[2] = (6 * reds[0] + 1 * reds[1]) / 7;
		reds[3] = (5 * reds[0] + 2 * reds[1]) / 7;
		reds[4] = (4 * reds[0] + 3 * reds[1]) / 7;
		reds[5] = (3 * reds[0] + 4 * reds[1]) / 7;
		reds[6] = (2 * reds[0] + 5 * reds[1]) / 7;
		reds[7] = (1 * reds[0] + 6 * reds[1]) / 7;
	}
	else
	{
		reds[2] = (4 * reds[0] + 1 * reds[1]) / 5;
		reds[3] = (3 * reds[0] + 2 * reds[1]) / 5;
		reds[4] = (2 * reds[0] + 3 * reds[1]) / 5;
		reds[5] = (1 * reds[0] + 4 * reds[1]) / 5;
		reds[6] = 0;
		reds[7] = 0xFF;
	}
	uint32_t green_bits1 = (in[4] << 16) | (in[3] << 8) | in[2];
	uint32_t green_bits2 = (in[7] << 16) | (in[6] << 8) | in[5];
	uint8_t greens[8];
	greens[0] = in[0];
	greens[1] = in[1];
	if (greens[0] > greens[1])
	{
		greens[2] = (6 * greens[0] + 1 * greens[1]) / 7;
		greens[3] = (5 * greens[0] + 2 * greens[1]) / 7;
		greens[4] = (4 * greens[0] + 3 * greens[1]) / 7;
		greens[5] = (3 * greens[0] + 4 * greens[1]) / 7;
		greens[6] = (2 * greens[0] + 5 * greens[1]) / 7;
		greens[7] = (1 * greens[0] + 6 * greens[1]) / 7;
	}
	else
	{
		greens[2] = (4 * greens[0] + 1 * greens[1]) / 5;
		greens[3] = (3 * greens[0] + 2 * greens[1]) / 5;
		greens[4] = (2 * greens[0] + 3 * greens[1]) / 5;
		greens[5] = (1 * greens[0] + 4 * greens[1]) / 5;
		greens[6] = 0;
		greens[7] = 0xFF;
	}
	for (uint32_t y = 0; y < 4; ++y)
	{
		for (uint32_t x = 0; x < 4; ++x)
		{
			if (y < 2)
			{
				out[idx++] = reds[red_bits1 & 7];
				red_bits1 >>= 3;
				out[idx++] = greens[green_bits1 & 7];
				green_bits1 >>= 3;
			}
			else
			{
				out[idx++] = reds[red_bits2 & 7];
				red_bits2 >>= 3;
				out[idx++] = greens[green_bits2 & 7];
				green_bits2 >>= 3;
			}
		}
		idx += idx_inc;
	}
}

void unpack_bc5(uint32_t width, uint32_t height, const uint8_t *in, uint8_t *out)
{
	uint32_t bx = (width + 3) & (~0x3);
	uint32_t by = (height + 3) & (~0x3);
	for (uint32_t y = 0; y < by; y += 4)
	{
		for (uint32_t x = 0; x < bx; x += 4)
		{
			unpack_bc5_block(x, y, width, height, in, out);
			in += 16;
		}
	}
}
