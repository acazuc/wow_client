#include "snd/stream.h"

#include "memory.h"

#include <math.h>

struct snd_stream_bilinear
{
	struct snd_stream stream;
	struct snd_stream *input;
	float back_samples[2][3];
	float prev_samples[2][2];
	float a0;
	float a1;
	float a2;
	float b0;
	float b1;
	float b2;
};

static ssize_t bilinear_read_samples(struct snd_stream *stream, float *samples, size_t count, size_t *sample_rate)
{
	struct snd_stream_bilinear *bilinear = (struct snd_stream_bilinear*)stream;
	ssize_t ret = bilinear->input->vtable->read_samples(bilinear->input, samples, count, sample_rate);
	if (ret <= 0)
		return ret;
	float *dst = samples;
	for (ssize_t i = 0; i < ret; ++i)
	{
		for (size_t j = 0; j < 2; ++j)
		{
			bilinear->back_samples[j][2] = bilinear->back_samples[j][1];
			bilinear->back_samples[j][1] = bilinear->back_samples[j][0];
			bilinear->back_samples[j][0] = *dst;
			float pcm =
				(bilinear->b0 * bilinear->back_samples[j][0]) +
				(bilinear->b1 * bilinear->back_samples[j][1]) +
				(bilinear->b2 * bilinear->back_samples[j][2]) -
				(bilinear->a1 * bilinear->prev_samples[j][0]) -
				(bilinear->a2 * bilinear->prev_samples[j][1]);
			*dst = pcm;
			bilinear->prev_samples[j][1] = bilinear->prev_samples[j][0];
			bilinear->prev_samples[j][0] = pcm;
			++dst;
		}
	}
	return ret;
}

static const struct snd_stream_vtable bilinear_vtable =
{
	.read_samples = bilinear_read_samples,
};

static struct snd_stream_bilinear *snd_stream_bilinear_new(struct snd_stream *input)
{
	struct snd_stream_bilinear *bilinear = mem_zalloc(MEM_SND, sizeof(*bilinear));
	if (!bilinear)
		return NULL;
	bilinear->stream.vtable = &bilinear_vtable;
	bilinear->input = input;
	return bilinear;
}

struct snd_stream *snd_stream_lowpass_new(struct snd_stream *input, uint32_t sampling, float cutoff, float resonance)
{
	struct snd_stream_bilinear *stream = snd_stream_bilinear_new(input);
	if (!stream)
		return NULL;
	float w0 = 2 * M_PI * cutoff / sampling;
	float alpha = sinf(w0) / (2 * resonance);
	float cw0 = cosf(w0);
	stream->a0 = 1 + alpha;
	stream->a1 = (-2 * cw0) / stream->a0;
	stream->a2 = (1 - alpha) / stream->a0;
	stream->b0 = ((1 - cw0) / 2) / stream->a0;
	stream->b1 = (1 - cw0) / stream->a0;
	stream->b2 = ((1 - cw0) / 2) / stream->a0;
	return &stream->stream;
}

struct snd_stream *snd_stream_highpass_new(struct snd_stream *input, uint32_t sampling, float cutoff, float resonance)
{
	struct snd_stream_bilinear *stream = snd_stream_bilinear_new(input);
	if (!stream)
		return NULL;
	float w0 = 2 * M_PI * cutoff / sampling;
	float alpha = sinf(w0) / (2 * resonance);
	float cw0 = cosf(w0);
	stream->a0 = 1 + alpha;
	stream->a1 = (-2 * cw0) / stream->a0;
	stream->a2 = (1 - alpha) / stream->a0;
	stream->b0 = ((1 + cw0) / 2) / stream->a0;
	stream->b1 = (-1 - cw0) / stream->a0;
	stream->b2 = ((1 + cw0) / 2) / stream->a0;
	return &stream->stream;
}

struct snd_stream *snd_stream_bandpass_new(struct snd_stream *input, uint32_t sampling, float cutoff, float resonance)
{
	struct snd_stream_bilinear *stream = snd_stream_bilinear_new(input);
	if (!stream)
		return NULL;
	float w0 = 2 * M_PI * cutoff / sampling;
	float alpha = sinf(w0) / (2 * resonance);
	float cw0 = cosf(w0);
	stream->a0 = 1 + alpha;
	stream->a1 = (-2 * cw0) / stream->a0;
	stream->a2 = (1 - alpha) / stream->a0;
	stream->b0 = alpha / stream->a0;
	stream->b1 = 0;
	stream->b2 = alpha / stream->a0;
	return &stream->stream;
}

struct snd_stream_echo
{
	struct snd_stream stream;
	struct snd_stream *input;
	size_t buffer_size;
	float *buffers[2];
	float delay;
	float decay;
	size_t pos;
};

static ssize_t echo_read_samples(struct snd_stream *stream, float *samples, size_t count, size_t *sample_rate)
{
	struct snd_stream_echo *echo = (struct snd_stream_echo*)stream;
	ssize_t ret = echo->input->vtable->read_samples(echo->input, samples, count, sample_rate);
	if (ret <= 0)
		return ret;
	float *dst = samples;
	for (ssize_t i = 0; i < ret; ++i)
	{
		uint32_t next = (echo->pos + 1) % echo->buffer_size;
		for (size_t j = 0; j < 2; ++j)
		{
			dst[j] += echo->buffers[j][next] * echo->decay;
			echo->buffers[j][echo->pos] = dst[j];
		}
		dst += 2;
		echo->pos = next;
	}
	return ret;
}

static void echo_dtr(struct snd_stream *stream)
{
	struct snd_stream_echo *echo = (struct snd_stream_echo*)stream;
	mem_free(MEM_SND, echo->buffers[0]);
	mem_free(MEM_SND, echo->buffers[1]);
}

static const struct snd_stream_vtable echo_vtable =
{
	.dtr = echo_dtr,
	.read_samples = echo_read_samples,
};

struct snd_stream *snd_stream_echo_new(struct snd_stream *input, uint32_t sampling, float delay, float decay)
{
	struct snd_stream_echo *echo = mem_zalloc(MEM_SND, sizeof(*echo));
	if (!echo)
		return NULL;
	echo->buffer_size = delay * sampling;
	echo->buffers[0] = mem_malloc(MEM_SND, sizeof(*echo->buffers[0]) * echo->buffer_size);
	echo->buffers[1] = mem_malloc(MEM_SND, sizeof(*echo->buffers[1]) * echo->buffer_size);
	if (!echo->buffers[0] || !echo->buffers[1])
	{
		mem_free(MEM_SND, echo->buffers[0]);
		mem_free(MEM_SND, echo->buffers[1]);
		mem_free(MEM_SND, echo);
		return NULL;
	}
	for (size_t i = 0; i < echo->buffer_size; ++i)
	{
		echo->buffers[0][i] = 0;
		echo->buffers[1][i] = 0;
	}
	echo->input = input;
	echo->delay = delay;
	echo->decay = decay;
	echo->pos = 0;
	echo->stream.vtable = &echo_vtable;
	return &echo->stream;
}
