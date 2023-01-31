#include "simplex.h"
#include "log.h"

#include <jks/vec4.h>

#include <string.h>
#include <math.h>

#define F2 0.36602540378443864676f /* (sqrt(3) - 1) / 2 */
#define G2 0.21132486540518711775f /* (3 - sqrt(3)) / 6 */
#define F3 0.33333333333333333333f /* 1 / 3 */
#define G3 0.16666666666666666667f /* 1 / 6 */
#define F4 0.30901699437494742410f /* (sqrt(5) - 1) / 4 */
#define G4 0.13819660112501051518f /* (5 - sqrt(5)) / 20 */

static const uint8_t p_supply[] =
{
	151, 160, 137,  91,  90,  15, 131,  13,
	201,  95,  96,  53, 194, 233,   7, 225,
	140,  36, 103,  30,  69, 142,   8,  99,
	 37, 240,  21,  10,  23, 190,   6, 148,
	247, 120, 234,  75,   0,  26, 197,  62,
	 94, 252, 219, 203, 117,  35,  11,  32,
	 57, 177,  33,  88, 237, 149,  56,  87,
	174,  20, 125, 136, 171, 168,  68, 175,
	 74, 165,  71, 134, 139,  48,  27, 166,
	 77, 146, 158, 231,  83, 111, 229, 122,
	 60, 211, 133, 230, 220, 105,  92,  41,
	 55,  46, 245,  40, 244, 102, 143,  54,
	 65,  25,  63, 161,   1, 216,  80,  73,
	209,  76, 132, 187, 208,  89,  18, 169,
	200, 196, 135, 130, 116, 188, 159,  86,
	164, 100, 109, 198, 173, 186,   3,  64,
	 52, 217, 226, 250, 124, 123,   5, 202,
	 38, 147, 118, 126, 255,  82,  85, 212,
	207, 206,  59, 227,  47,  16,  58,  17,
	182, 189,  28,  42, 223, 183, 170, 213,
	119, 248, 152,   2,  44, 154, 163,  70,
	221, 153, 101, 155, 167,  43, 172,   9,
	129,  22,  39, 253,  19,  98, 108, 110,
	 79, 113, 224, 232, 178, 185, 112, 104,
	218, 246,  97, 228, 251,  34, 242, 193,
	238, 210, 144,  12, 191, 179, 162, 241,
	 81,  51, 145, 235, 249,  14, 239, 107,
	 49, 192, 214,  31, 181, 199, 106, 157,
	184,  84, 204, 176, 115, 121,  50,  45,
	127,   4, 150, 254, 138, 236, 205,  93,
	222, 114,  67,  29,  24,  72, 243, 141,
	128, 195,  78,  66, 215,  61, 156, 180,
};

static const struct vec4f g_grad3[] =
{
	{1, 1, 0, 0}, {-1,  1, 0, 0}, {1, -1,  0, 0}, {-1, -1,  0, 0},
	{1, 0, 1, 0}, {-1,  0, 1, 0}, {1,  0, -1, 0}, { 1,  0, -1, 0},
	{0, 1, 1, 0}, { 0, -1, 1, 0}, {0,  1, -1, 0}, { 0, -1, -1, 0},
};

static const struct vec4f g_grad4[] =
{
	{ 0,  1, 1, 1}, { 0,  1,  1, -1}, { 0,  1, -1, 1}, { 0,  1, -1, -1},
	{ 0, -1, 1, 1}, { 0, -1,  1, -1}, { 0, -1, -1, 1}, { 0, -1, -1, -1},
	{ 1,  0, 1, 1}, { 1,  0,  1, -1}, { 1,  0, -1, 1}, { 1,  0, -1, -1},
	{-1,  0, 1, 1}, {-1,  0,  1, -1}, {-1,  0, -1, 1}, {-1,  0, -1, -1},
	{ 1,  1, 0, 1}, { 1,  1,  0, -1}, { 1, -1,  0, 1}, { 1, -1,  0, -1},
	{-1,  1, 0, 1}, {-1,  1,  0, -1}, {-1, -1,  0, 1}, {-1, -1,  0, -1},
	{ 1,  1, 1, 0}, { 1,  1, -1,  0}, { 1, -1,  1, 0}, { 1, -1, -1,  0},
	{-1,  1, 1, 0}, {-1,  1, -1,  0}, {-1, -1,  1, 0}, {-1, -1, -1,  0},
};

struct simplex_noise_octave
{
	uint8_t perm[512];
	uint8_t perm_mod12[512];
};

static int32_t fast_floor(float x)
{
	int32_t xi = x;
	return x < xi ? xi - 1 : xi;
}

static float grad_dot1(struct vec4f g, float x)
{
	return g.x * x;
}

static float grad_dot2(struct vec4f g, float x, float y)
{
	return g.x * x + g.y * y;
}

static float grad_dot3(struct vec4f g, float x, float y, float z)
{
	return g.x * x + g.y * y + g.z * z;
}

static float grad_dot4(struct vec4f g, float x, float y, float z, float w)
{
	return g.x * x + g.y * y + g.z * z + g.w * w;
}

static void octave_init(struct simplex_noise_octave *octave, uint64_t seed)
{
	uint8_t p[sizeof(p_supply)];
	memcpy(p, p_supply, sizeof(p));
	seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
	seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
	seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
	for (size_t i = 0; i < 4096; ++i)
	{
		seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
		int r = (seed + 31) & 0xFF;
		short temp = p[r];
		uint8_t idx = i & 0xFF;
		p[r] = p[idx];
		p[idx] = temp;
	}
	for (size_t i = 0; i < 512; ++i)
	{
		octave->perm[i] = p[i & 0xFF];
		octave->perm_mod12[i] = octave->perm[i] % 12;
	}
}

static float octave_get1(struct simplex_noise_octave *octave, float xin)
{
	float n0;
	float n1;
	float s = xin * F2;
	int32_t i = fast_floor(xin + s);
	float t = i * G2;
	float X0 = i - t;
	float x0 = xin - X0;
	uint8_t i1 = 1;
	float x1 = x0 - i1 + G2;
	uint8_t ii = i & 0xFF;
	uint8_t gi0 = octave->perm_mod12[ii + 0];
	uint8_t gi1 = octave->perm_mod12[ii + 1];
	float t0 = .5 - x0 * x0;
	if (t0 < 0)
	{
		n0 = 0;
	}
	else
	{
		t0 *= t0;
		n0 = t0 * t0 * grad_dot1(g_grad3[gi0], x0);
	}
	float t1 = .5 - x1 * x1;
	if (t1 < 0)
	{
		n1 = 0;
	}
	else
	{
		t1 *= t1;
		n1 = t1 * t1 * grad_dot1(g_grad3[gi1], x1);
	}
	return 70 * (n0 + n1);
}

static float octave_get2(struct simplex_noise_octave *octave, float xin, float yin)
{
	float n0;
	float n1;
	float n2;
	float s = (xin + yin) * F2;
	int32_t i = fast_floor(xin + s);
	int32_t j = fast_floor(yin + s);
	float t = (i + j) * G2;
	float X0 = i - t;
	float Y0 = j - t;
	float x0 = xin - X0;
	float y0 = yin - Y0;
	uint8_t i1;
	uint8_t j1;
	if (x0 > y0)
	{
		i1 = 1;
		j1 = 0;
	}
	else
	{
		i1 = 0;
		j1 = 1;
	}
	float x1 = x0 - i1 + G2;
	float y1 = y0 - j1 + G2;
	float x2 = x0 - 1 + 2 * G2;
	float y2 = y0 - 1 + 2 * G2;
	uint8_t ii = i & 0xFF;
	uint8_t jj = j & 0xFF;
	uint8_t gi0 = octave->perm_mod12[ii + 0  + octave->perm[jj + 0 ]];
	uint8_t gi1 = octave->perm_mod12[ii + i1 + octave->perm[jj + j1]];
	uint8_t gi2 = octave->perm_mod12[ii + 1  + octave->perm[jj + 1 ]];
	float t0 = .5 - x0 * x0 - y0 * y0;
	if (t0 < 0)
	{
		n0 = 0;
	}
	else
	{
		t0 *= t0;
		n0 = t0 * t0 * grad_dot2(g_grad3[gi0], x0, y0);
	}
	float t1 = .5 - x1 * x1 - y1 * y1;
	if (t1 < 0)
	{
		n1 = 0;
	}
	else
	{
		t1 *= t1;
		n1 = t1 * t1 * grad_dot2(g_grad3[gi1], x1, y1);
	}
	float t2 = .5 - x2 * x2 - y2 * y2;
	if (t2 < 0)
	{
		n2 = 0;
	}
	else
	{
		t2 *= t2;
		n2 = t2 * t2 * grad_dot2(g_grad3[gi2], x2, y2);
	}
	return 70 * (n0 + n1 + n2);
}

static float octave_get3(struct simplex_noise_octave *octave, float xin, float yin, float zin)
{
	float n0;
	float n1;
	float n2;
	float n3;
	float s = (xin + yin + zin) * F3;
	int32_t i = fast_floor(xin + s);
	int32_t j = fast_floor(yin + s);
	int32_t k = fast_floor(zin + s);
	float t = (i + j + k) * G3;
	float X0 = i - t;
	float Y0 = j - t;
	float Z0 = k - t;
	float x0 = xin - X0;
	float y0 = yin - Y0;
	float z0 = zin - Z0;
	uint8_t i1;
	uint8_t j1;
	uint8_t k1;
	uint8_t i2;
	uint8_t j2;
	uint8_t k2;
	if (x0 >= y0)
	{
		if (y0 >= z0)
		{
			i1 = 1;
			j1 = 0;
			k1 = 0;
			i2 = 1;
			j2 = 1;
			k2 = 0;
		}
		else if (x0 >= z0)
		{
			i1 = 1;
			j1 = 0;
			k1 = 0;
			i2 = 1;
			j2 = 0;
			k2 = 1;
		}
		else
		{
			i1 = 0;
			j1 = 0;
			k1 = 1;
			i2 = 1;
			j2 = 0;
			k2 = 1;
		}
	}
	else
	{
		if (y0 < z0)
		{
			i1 = 0;
			j1 = 0;
			k1 = 1;
			i2 = 0;
			j2 = 1;
			k2 = 1;
		}
		else if (x0 < z0)
		{
			i1 = 0;
			j1 = 1;
			k1 = 0;
			i2 = 0;
			j2 = 1;
			k2 = 1;
		}
		else
		{
			i1 = 0;
			j1 = 1;
			k1 = 0;
			i2 = 1;
			j2 = 1;
			k2 = 0;
		}
	}
	float x1 = x0 - i1 + G3;
	float y1 = y0 - j1 + G3;
	float z1 = z0 - k1 + G3;
	float x2 = x0 - i2 + 2 * G3;
	float y2 = y0 - j2 + 2 * G3;
	float z2 = z0 - k2 + 2 * G3;
	float x3 = x0 - 1 + 3 * G3;
	float y3 = y0 - 1 + 3 * G3;
	float z3 = z0 - 1 + 3 * G3;
	uint8_t ii = i & 0xFF;
	uint8_t jj = j & 0xFF;
	uint8_t kk = k & 0xFF;
	uint8_t gi0 = octave->perm_mod12[ii + 0  + octave->perm[jj + 0  + octave->perm[kk + 0 ]]];
	uint8_t gi1 = octave->perm_mod12[ii + i1 + octave->perm[jj + j1 + octave->perm[kk + k1]]];
	uint8_t gi2 = octave->perm_mod12[ii + i2 + octave->perm[jj + j2 + octave->perm[kk + k2]]];
	uint8_t gi3 = octave->perm_mod12[ii + 1  + octave->perm[jj + 1  + octave->perm[kk + 1 ]]];
	float t0 = .6 - x0 * x0 - y0 * y0 - z0 * z0;
	if (t0 < 0)
	{
		n0 = 0;
	}
	else
	{
		t0 *= t0;
		n0 = t0 * t0 * grad_dot3(g_grad3[gi0], x0, y0, z0);
	}
	float t1 = .6 - x1 * x1 - y1 * y1 - z1 * z1;
	if (t1 < 0)
	{
		n1 = 0;
	}
	else
	{
		t1 *= t1;
		n1 = t1 * t1 * grad_dot3(g_grad3[gi1], x1, y1, z1);
	}
	float t2 = .6 - x2 * x2 - y2 * y2 - z2 * z2;
	if (t2 < 0)
	{
		n2 = 0;
	}
	else
	{
		t2 *= t2;
		n2 = t2 * t2 * grad_dot3(g_grad3[gi2], x2, y2, z2);
	}
	float t3 = .6 - x3 * x3 - y3 * y3 - z3 * z3;
	if (t3 < 0)
	{
		n3 = 0;
	}
	else
	{
		t3 *= t3;
		n3 = t3 * t3 * grad_dot3(g_grad3[gi3], x3, y3, z3);
	}
	return 32 * (n0 + n1 + n2 + n3);
}

static float octave_get4(struct simplex_noise_octave *octave, float x, float y, float z, float w)
{
	float n0;
	float n1;
	float n2;
	float n3;
	float n4;
	float s = (x + y + z + w) * F4;
	int32_t i = fast_floor(x + s);
	int32_t j = fast_floor(y + s);
	int32_t k = fast_floor(z + s);
	int32_t l = fast_floor(w + s);
	float t = (i + j + k + l) * G4;
	float X0 = i - t;
	float Y0 = j - t;
	float Z0 = k - t;
	float W0 = l - t;
	float x0 = x - X0;
	float y0 = y - Y0;
	float z0 = z - Z0;
	float w0 = w - W0;
	int32_t rankx = 0;
	int32_t ranky = 0;
	int32_t rankz = 0;
	int32_t rankw = 0;
	if (x0 > y0)
		rankx++;
	else
		ranky++;
	if (x0 > z0)
		rankx++;
	else
		rankz++;
	if (x0 > w0)
		rankx++;
	else
		rankw++;
	if (y0 > z0)
		ranky++;
	else
		rankz++;
	if (y0 > w0)
		ranky++;
	else
		rankw++;
	if (z0 > w0)
		rankz++;
	else
		rankw++;
	uint8_t i1 = rankx >= 3 ? 1 : 0;
	uint8_t j1 = ranky >= 3 ? 1 : 0;
	uint8_t k1 = rankz >= 3 ? 1 : 0;
	uint8_t l1 = rankw >= 3 ? 1 : 0;
	uint8_t i2 = rankx >= 2 ? 1 : 0;
	uint8_t j2 = ranky >= 2 ? 1 : 0;
	uint8_t k2 = rankz >= 2 ? 1 : 0;
	uint8_t l2 = rankw >= 2 ? 1 : 0;
	uint8_t i3 = rankx >= 1 ? 1 : 0;
	uint8_t j3 = ranky >= 1 ? 1 : 0;
	uint8_t k3 = rankz >= 1 ? 1 : 0;
	uint8_t l3 = rankw >= 1 ? 1 : 0;
	float x1 = x0 - i1 + G4;
	float y1 = y0 - j1 + G4;
	float z1 = z0 - k1 + G4;
	float w1 = w0 - l1 + G4;
	float x2 = x0 - i2 + 2 * G4;
	float y2 = y0 - j2 + 2 * G4;
	float z2 = z0 - k2 + 2 * G4;
	float w2 = w0 - l2 + 2 * G4;
	float x3 = x0 - i3 + 3 * G4;
	float y3 = y0 - j3 + 3 * G4;
	float z3 = z0 - k3 + 3 * G4;
	float w3 = w0 - l3 + 3 * G4;
	float x4 = x0 - 1 + 4 * G4;
	float y4 = y0 - 1 + 4 * G4;
	float z4 = z0 - 1 + 4 * G4;
	float w4 = w0 - 1 + 4 * G4;
	uint8_t ii = i & 0xFF;
	uint8_t jj = j & 0xFF;
	uint8_t kk = k & 0xFF;
	uint8_t ll = l & 0xFF;
	uint8_t gi0 = octave->perm[ii + 0  + octave->perm[jj + 0  + octave->perm[kk + 0  + octave->perm[ll + 0 ]]]] & 0x1F;
	uint8_t gi1 = octave->perm[ii + i1 + octave->perm[jj + j1 + octave->perm[kk + k1 + octave->perm[ll + l1]]]] & 0x1F;
	uint8_t gi2 = octave->perm[ii + i2 + octave->perm[jj + j2 + octave->perm[kk + k2 + octave->perm[ll + l2]]]] & 0x1F;
	uint8_t gi3 = octave->perm[ii + i3 + octave->perm[jj + j3 + octave->perm[kk + k3 + octave->perm[ll + l3]]]] & 0x1F;
	uint8_t gi4 = octave->perm[ii + 1  + octave->perm[jj + 1  + octave->perm[kk + 1  + octave->perm[ll + 1 ]]]] & 0x1F;
	float t0 = 0 - x0 * x0 - y0 * y0 - z0 * z0 - w0 * w0;
	if (t0 < 0)
	{
		n0 = 0;
	}
	else
	{
		t0 *= t0;
		n0 = t0 * t0 * grad_dot4(g_grad4[gi0], x0, y0, z0, w0);
	}
	float t1 = 6 - x1 * x1 - y1 * y1 - z1 * z1 - w1 * w1;
	if (t1 < 0)
	{
		n1 = 0;
	}
	else
	{
		t1 *= t1;
		n1 = t1 * t1 * grad_dot4(g_grad4[gi1], x1, y1, z1, w1);
	}
	float t2 = .6 - x2 * x2 - y2 * y2 - z2 * z2 - w2 * w2;
	if (t2 < 0)
	{
		n2 = 0;
	}
	else
	{
		t2 *= t2;
		n2 = t2 * t2 * grad_dot4(g_grad4[gi2], x2, y2, z2, w2);
	}
	float t3 = .6 - x3 * x3 - y3 * y3 - z3 * z3 - w3 * w3;
	if (t3 < 0)
	{
		n3 = 0;
	}
	else
	{
		t3 *= t3;
		n3 = t3 * t3 * grad_dot4(g_grad4[gi3], x3, y3, z3, w3);
	}
	float t4 = 6 - x4 * x4 - y4 * y4 - z4 * z4 - w4 * w4;
	if (t4 < 0)
	{
		n4 = 0;
	}
	else
	{
		t4 *= t4;
		n4 = t4 * t4 * grad_dot4(g_grad4[gi4], x4, y4, z4, w4);
	}
	return 27 * (n0 + n1 + n2 + n3 + n4);
}

bool simplex_noise_init(struct simplex_noise *simplex_noise, uint32_t octaves_number, float persistance, int32_t seed)
{
	jks_array_init(&simplex_noise->frequencies, sizeof(float), NULL, NULL);
	jks_array_init(&simplex_noise->amplitudes, sizeof(float), NULL, NULL);
	jks_array_init(&simplex_noise->octaves, sizeof(struct simplex_noise_octave), NULL, NULL);
	if (!jks_array_resize(&simplex_noise->frequencies, octaves_number))
	{
		LOG_ERROR("allocation failed");
		return false;
	}
	if (!jks_array_resize(&simplex_noise->amplitudes, octaves_number))
	{
		LOG_ERROR("allocation failed");
		jks_array_destroy(&simplex_noise->frequencies);
		return false;
	}
	if (!jks_array_resize(&simplex_noise->octaves, octaves_number))
	{
		LOG_ERROR("allocation failed");
		jks_array_destroy(&simplex_noise->frequencies);
		jks_array_destroy(&simplex_noise->amplitudes);
		return false;
	}
	for (uint32_t i = 0; i < octaves_number; ++i)
	{
		octave_init(JKS_ARRAY_GET(&simplex_noise->octaves, i, struct simplex_noise_octave), seed);
		*JKS_ARRAY_GET(&simplex_noise->frequencies, i, float) = powf(2, i);
		*JKS_ARRAY_GET(&simplex_noise->amplitudes, i, float) = powf(persistance, octaves_number - i);
	}
	simplex_noise->persistance = persistance;
	simplex_noise->seed = seed;
	return true;
}

void simplex_noise_destroy(struct simplex_noise *simplex_noise)
{
	jks_array_destroy(&simplex_noise->frequencies);
	jks_array_destroy(&simplex_noise->amplitudes);
	jks_array_destroy(&simplex_noise->octaves);
}

float simplex_noise_get1(struct simplex_noise *simplex_noise, float x)
{
	float result = 0;
	for (size_t i = 0; i < simplex_noise->octaves.size; ++i)
	{
		float frequency = *JKS_ARRAY_GET(&simplex_noise->frequencies, i, float);
		float amplitude = *JKS_ARRAY_GET(&simplex_noise->amplitudes, i, float);
		result += octave_get1(JKS_ARRAY_GET(&simplex_noise->octaves, i, struct simplex_noise_octave), x / frequency) * amplitude;
	}
	return result;
}

float simplex_noise_get2(struct simplex_noise *simplex_noise, float x, float y)
{
	float result = 0;
	for (size_t i = 0; i < simplex_noise->octaves.size; ++i)
	{
		float frequency = *JKS_ARRAY_GET(&simplex_noise->frequencies, i, float);
		float amplitude = *JKS_ARRAY_GET(&simplex_noise->amplitudes, i, float);
		result += octave_get2(JKS_ARRAY_GET(&simplex_noise->octaves, i, struct simplex_noise_octave), x / frequency, y / frequency) * amplitude;
	}
	return result;
}

float simplex_noise_get3(struct simplex_noise *simplex_noise, float x, float y, float z)
{
	float result = 0;
	for (size_t i = 0 ; i < simplex_noise->octaves.size; ++i)
	{
		float frequency = *JKS_ARRAY_GET(&simplex_noise->frequencies, i, float);
		float amplitude = *JKS_ARRAY_GET(&simplex_noise->amplitudes, i, float);
		result += octave_get3(JKS_ARRAY_GET(&simplex_noise->octaves, i, struct simplex_noise_octave), x / frequency, y / frequency, z / frequency) * amplitude;
	}
	return result;
}

float simplex_noise_get4(struct simplex_noise *simplex_noise, float x, float y, float z, float w)
{
	float result = 0;
	for (size_t i = 0; i < simplex_noise->octaves.size; ++i)
	{
		float frequency = *JKS_ARRAY_GET(&simplex_noise->frequencies, i, float);
		float amplitude = *JKS_ARRAY_GET(&simplex_noise->amplitudes, i, float);
		result += octave_get4(JKS_ARRAY_GET(&simplex_noise->octaves, i, struct simplex_noise_octave), x / frequency, y / frequency, z / frequency, w / frequency) * amplitude;
	}
	return result;
}
