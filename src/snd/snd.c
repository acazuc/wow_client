#include "snd/stream.h"
#include "snd/snd.h"

#include "memory.h"
#include "cache.h"
#include "log.h"
#include "wow.h"

#include <libwow/mpq.h>

#include <string.h>

#define CHANNELS 2

static int callback(const void *input, void *output, unsigned long frame_count, const PaStreamCallbackTimeInfo *pa_time_info, PaStreamCallbackFlags status_flags, void *ptr)
{
	(void)input;
	(void)pa_time_info;
	(void)status_flags;

	float *dst = output;
	struct snd *snd = ptr;
	for (size_t i = 0; i < frame_count * CHANNELS; ++i)
		dst[i] = 0;
	struct snd_stream *stream, *nxt;
	float buf[1024];
	for (stream = TAILQ_FIRST(&snd->streams); stream && (nxt = TAILQ_NEXT(stream, chain), 1); stream = nxt)
	{
		dst = output;
		size_t n = frame_count;
		while (n)
		{
			size_t blk = n;
			if (blk > sizeof(buf) / sizeof(*buf) / CHANNELS)
				blk = sizeof(buf) / sizeof(*buf) / CHANNELS;
			size_t sample_rate;
			ssize_t ret = stream->vtable->read_samples(stream, buf, blk, &sample_rate);
			if (ret < 0)
			{
				TAILQ_REMOVE(&snd->streams, stream, chain);
				snd_stream_delete(stream);
				goto next_stream;
			}
			if (!ret)
			{
				TAILQ_REMOVE(&snd->streams, stream, chain);
				snd_stream_delete(stream);
				goto next_stream;
			}
			for (ssize_t j = 0; j < ret * CHANNELS; ++j)
				dst[j] += buf[j];
			dst += ret * CHANNELS;
			n -= ret;
		}
next_stream:;
	}
	return paContinue;
}

static struct snd_stream *create_file_stream(const char *path)
{
	struct wow_mpq_file *file = wow_mpq_get_file(g_wow->mpq_compound, path);
	if (!file)
	{
		LOG_ERROR("failed to open file: %s", path);
		return NULL;
	}
	size_t len = strlen(path);
	if (len < 4)
	{
		LOG_ERROR("invalid filepath length");
		wow_mpq_file_delete(file);
		return NULL;
	}
	struct snd_stream *stream;
	if (!strcmp(&path[len - 4], ".MP3"))
	{
		stream = snd_stream_mp3_new(file);
	}
	else if (!strcmp(&path[len - 4], ".WAV"))
	{
		stream = snd_stream_wav_new(file);
	}
	else
	{
		LOG_ERROR("unknown audio file: %s", path);
		wow_mpq_file_delete(file);
		return NULL;
	}
	if (!stream)
	{
		LOG_ERROR("failed to create stream");
		wow_mpq_file_delete(file);
		return NULL;
	}
	return stream;
}

static struct snd_stream *create_resampled_file_stream(struct snd *snd, const char *path)
{
	struct snd_stream *stream = create_file_stream(path);
	if (!stream)
		return NULL;
	struct snd_stream *resample_stream = snd_stream_resample_new(stream, snd->sample_rate);
	if (!resample_stream)
	{
		LOG_ERROR("failed to create resample stream");
		snd_stream_delete(stream);
		return NULL;
	}

	return resample_stream;
}

struct snd *snd_new(void)
{
	struct snd *snd = mem_zalloc(MEM_SND, sizeof(*snd));
	if (!snd)
	{
		LOG_ERROR("sound allocation failed");
		return NULL;
	}
	TAILQ_INIT(&snd->streams);
	PaStreamParameters output_parameters;
	PaDeviceIndex device = Pa_GetDefaultOutputDevice();
	const PaDeviceInfo *device_info = Pa_GetDeviceInfo(device);
	snd->sample_rate = device_info->defaultSampleRate;
	output_parameters.device = device;
	output_parameters.channelCount = CHANNELS;
	output_parameters.sampleFormat = paFloat32;
	output_parameters.suggestedLatency = device_info->defaultHighOutputLatency;
	output_parameters.hostApiSpecificStreamInfo = NULL;
	PaError error = Pa_OpenStream(&snd->stream, NULL, &output_parameters, snd->sample_rate, paFramesPerBufferUnspecified, paNoFlag, callback, snd);
	if (error)
	{
		LOG_ERROR("failed to open stream: %s", Pa_GetErrorText(error));
		return NULL;
	}
	error = Pa_StartStream(snd->stream);
	if (error)
	{
		LOG_ERROR("failed to start stream: %s", Pa_GetErrorText(error));
		return NULL;
	}

	return snd;
}

void snd_delete(struct snd *snd)
{
	if (!snd)
		return;
	Pa_CloseStream(snd->stream);
	mem_free(MEM_SND, snd);
}

void snd_set_glue_music(struct snd *snd, const char *path)
{
	LOG_ERROR("set glue %s", path);
	if (!path)
	{
		if (snd->glue_music)
		{
			TAILQ_REMOVE(&snd->streams, snd->glue_music, chain);
			snd_stream_delete(snd->glue_music);
			snd->glue_music = NULL;
		}
		return;
	}
	struct snd_stream *stream = create_resampled_file_stream(snd, path);
	if (!stream)
		return;
	//stream = snd_stream_highpass_new(stream, snd->sample_rate, 1000, 1);
	if (snd->glue_music)
	{
		TAILQ_REMOVE(&snd->streams, snd->glue_music, chain);
		snd_stream_delete(snd->glue_music);
	}
	TAILQ_INSERT_HEAD(&snd->streams, stream, chain);
	snd->glue_music = stream;
}

void snd_play_sound(struct snd *snd, const char *path)
{
	LOG_ERROR("play sound %s", path);
	struct snd_stream *stream = create_resampled_file_stream(snd, path);
	if (!stream)
		return;
	TAILQ_INSERT_HEAD(&snd->streams, stream, chain);
}

size_t snd_get_output_dev_count(struct snd *snd)
{
	int dev_count = Pa_GetDeviceCount();
	if (dev_count < 0)
		return 0;
	size_t count = 0;
	for (int i = 0; i < dev_count; ++i)
	{
		const PaDeviceInfo *dev = Pa_GetDeviceInfo(i);
		if (!dev)
			break;
		if (dev->maxOutputChannels < 1)
			continue;
		count++;
	}
	return count;
}

const PaDeviceInfo *snd_get_output_dev(struct snd *snd, size_t n)
{
	int dev_count = Pa_GetDeviceCount();
	if (dev_count < 0)
		return 0;
	size_t count = 0;
	for (int i = 0; i < dev_count; ++i)
	{
		const PaDeviceInfo *dev = Pa_GetDeviceInfo(i);
		if (!dev)
			break;
		if (dev->maxOutputChannels < 1)
			continue;
		if (count == n)
			return dev;
		count++;
	}
	return NULL;
}

size_t snd_get_input_dev_count(struct snd *snd)
{
	int dev_count = Pa_GetDeviceCount();
	if (dev_count < 0)
		return 0;
	size_t count = 0;
	for (int i = 0; i < dev_count; ++i)
	{
		const PaDeviceInfo *dev = Pa_GetDeviceInfo(i);
		if (!dev)
			break;
		if (dev->maxInputChannels < 1)
			continue;
		count++;
	}
	return count;
}

const PaDeviceInfo *snd_get_input_dev(struct snd *snd, size_t n)
{
	int dev_count = Pa_GetDeviceCount();
	if (dev_count < 0)
		return 0;
	size_t count = 0;
	for (int i = 0; i < dev_count; ++i)
	{
		const PaDeviceInfo *dev = Pa_GetDeviceInfo(i);
		if (!dev)
			break;
		if (dev->maxInputChannels < 1)
			continue;
		if (count == n)
			return dev;
		count++;
	}
	return NULL;
}
