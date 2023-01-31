#ifndef SIMPLEX_NOISE_H
#define SIMPLEX_NOISE_H

#include <jks/array.h>

struct simplex_noise
{
	struct jks_array frequencies; /* float */
	struct jks_array amplitudes; /* float */
	struct jks_array octaves; /* struct simplex_noise_octave */
	float persistance;
	int32_t seed;
};

bool simplex_noise_init(struct simplex_noise *simplex_noise, uint32_t octaves_number, float persistance, int32_t seed);
void simplex_noise_destroy(struct simplex_noise *simplex_noise);
float simplex_noise_get1(struct simplex_noise *simplex_noise, float x);
float simplex_noise_get2(struct simplex_noise *simplex_noise, float x, float y);
float simplex_noise_get3(struct simplex_noise *simplex_noise, float x, float y, float z);
float simplex_noise_get4(struct simplex_noise *simplex_noise, float x, float y, float z, float w);

#endif
