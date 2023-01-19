#ifndef SND_STREAM_H
#define SND_STREAM_H

#include <sys/types.h>
#include <sys/queue.h>
#include <stdint.h>
#include <stddef.h>

struct wow_mpq_file;

struct snd_stream;

struct snd_stream_vtable
{
	void (*ctr)(struct snd_stream *stream);
	void (*dtr)(struct snd_stream *stream);
	ssize_t (*read_samples)(struct snd_stream *stream, float *samples, size_t count, size_t *sample_rate);
};

struct snd_stream
{
	const struct snd_stream_vtable *vtable;
	TAILQ_ENTRY(snd_stream) chain;
};

struct snd_stream *snd_stream_mp3_new(struct wow_mpq_file *mpq);
struct snd_stream *snd_stream_wav_new(struct wow_mpq_file *mpq);
struct snd_stream *snd_stream_resample_new(struct snd_stream *stream, size_t output_rate);
struct snd_stream *snd_stream_lowpass_new(struct snd_stream *input, uint32_t sampling, float cutoff, float resonance);
struct snd_stream *snd_stream_highpass_new(struct snd_stream *input, uint32_t sampling, float cutoff, float resonance);
struct snd_stream *snd_stream_bandpass_new(struct snd_stream *input, uint32_t sampling, float cutoff, float resonance);
struct snd_stream *snd_stream_echo_new(struct snd_stream *input, uint32_t sampling, float delay, float decay);

void snd_stream_delete(struct snd_stream *stream);

#endif
