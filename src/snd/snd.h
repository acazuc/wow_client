#ifndef SND_H
#define SND_H

#include <portaudio.h>

#include <sys/queue.h>

struct snd_stream;

struct snd
{
	PaStream *stream;
	size_t sample_rate;
	TAILQ_HEAD(, snd_stream) streams;
	struct snd_stream *glue_music;
};

struct snd *snd_new(void);
void snd_delete(struct snd *snd);

void snd_set_glue_music(struct snd *snd, const char *path);
void snd_play_sound(struct snd *snd, const char *path);

size_t snd_get_output_dev_count(struct snd *snd);
const PaDeviceInfo *snd_get_output_dev(struct snd *snd, size_t i);
size_t snd_get_input_dev_count(struct snd *snd);
const PaDeviceInfo *snd_get_input_dev(struct snd *snd, size_t i);

#endif
