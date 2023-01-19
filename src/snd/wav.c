#include "snd/stream.h"

#include "memory.h"
#include "cache.h"
#include "log.h"
#include "wow.h"

#include <libwow/mpq.h>

#include <inttypes.h>
#include <string.h>

#define WAV_STREAM ((struct snd_stream_wav*)stream)

#define CHANNELS 2

struct snd_stream_wav
{
	struct snd_stream stream;
	wow_mpq_file_t *file;
	size_t file_begin;
	struct
	{
		uint32_t data_size;
		uint16_t fmt_tag;
		uint16_t channels;
		uint32_t samplerate;
		uint32_t byte_rate;
		uint16_t align;
		uint16_t bps;
	} fmt;
	uint32_t chunk_size;
	uint32_t chunk_pos;
};

static int decode_next_chunk(struct snd_stream_wav *stream)
{
	uint8_t sig[4];
	if (!wow_mpq_read(stream->file, sig, 4))
	{
		LOG_ERROR("failed to read file");
		return 0;
	}
	if (!memcmp(sig, "fmt ", 4))
	{
		if (stream->file->size - stream->file->pos < 20)
		{
			LOG_ERROR("fmt block too short");
			return 0;
		}
		if (!wow_mpq_read(stream->file, &stream->fmt, sizeof(stream->fmt)))
		{
			LOG_ERROR("failed to read fmt chunk");
			return 0;
		}
#if 0
		LOG_DEBUG("format: %" PRIu32, stream->fmt.data_size);
		LOG_DEBUG("fmt_tag: %" PRIu16, stream->fmt.fmt_tag);
		LOG_DEBUG("channels: %" PRIu16, stream->fmt.channels);
		LOG_DEBUG("samplerate: %" PRIu32, stream->fmt.samplerate);
		LOG_DEBUG("byte_rate: %" PRIu32, stream->fmt.byte_rate);
		LOG_DEBUG("bps: %" PRIu16, stream->fmt.bps);
#endif
		stream->chunk_pos = 0;
		stream->chunk_size = 0;
		return 1;
	}
	if (!memcmp(sig, "data", 4))
	{
		if (!wow_mpq_read(stream->file, &stream->chunk_size, 4))
		{
			LOG_ERROR("failed to stream data chunk size");
			return 0;
		}
		return 1;
	}
	LOG_ERROR("unknown chunk type: %.4s", (char*)&sig[0]);
	return 0;
}

static ssize_t decode_samples(struct snd_stream *stream, float *samples, size_t count, size_t *sample_rate)
{
	*sample_rate = WAV_STREAM->fmt.samplerate;
	if (WAV_STREAM->fmt.fmt_tag != 1)
	{
		LOG_ERROR("unknown fmt tag: %" PRIu16, WAV_STREAM->fmt.fmt_tag);
		return -1;
	}
	size_t avail_bytes = WAV_STREAM->chunk_size - WAV_STREAM->chunk_pos;
	size_t bytes_per_sample;
	switch (WAV_STREAM->fmt.bps)
	{
		case 8:
			bytes_per_sample = WAV_STREAM->fmt.channels;
			break;
		case 16:
			bytes_per_sample = WAV_STREAM->fmt.channels * 2;
			break;
		case 32:
			bytes_per_sample = WAV_STREAM->fmt.channels * 4;
			break;
		default:
			LOG_ERROR("invalid bps: %" PRIu16, WAV_STREAM->fmt.bps);
			return -1;
	}
	size_t avail_samples = avail_bytes / bytes_per_sample;
	if (count > avail_samples)
		count = avail_samples;
	switch (WAV_STREAM->fmt.bps)
	{
		case 8:
		{
			int8_t *org = (int8_t*)&WAV_STREAM->file->data[WAV_STREAM->file->pos];
			int8_t *ptr = org;
			if (WAV_STREAM->fmt.channels == 1)
			{
				for (size_t i = 0; i < count; ++i)
				{
					samples[0] = *(ptr++) / 128.f;
					samples[1] = samples[0];
					samples += 2;
				}
			}
			else
			{
				for (size_t i = 0; i < count; ++i)
				{
					samples[0] = *(ptr + 0) / 128.f;
					samples[1] = *(ptr + 1) / 128.f;
					samples += WAV_STREAM->fmt.channels;
					ptr += WAV_STREAM->fmt.channels;
				}
			}
			WAV_STREAM->file->pos += ptr - org;
			WAV_STREAM->chunk_pos += ptr - org;
			break;
		}
		case 16:
		{
			int16_t *org = (int16_t*)&WAV_STREAM->file->data[WAV_STREAM->file->pos];
			int16_t *ptr = org;
			if (WAV_STREAM->fmt.channels == 1)
			{
				for (size_t i = 0; i < count; ++i)
				{
					samples[0] = *(ptr++) / 32768.f;
					samples[1] = samples[0];
					samples += 2;
				}
			}
			else
			{
				for (size_t i = 0; i < count; ++i)
				{
					samples[0] = *(ptr + 0) / 32768.f;
					samples[1] = *(ptr + 1) / 32768.f;
					samples += WAV_STREAM->fmt.channels;
					ptr += WAV_STREAM->fmt.channels;
				}
			}
			WAV_STREAM->file->pos += (ptr - org) * 2;
			WAV_STREAM->chunk_pos += (ptr - org) * 2;
			break;
		}
		case 32:
		{
			int32_t *org = (int32_t*)&WAV_STREAM->file->data[WAV_STREAM->file->pos];
			int32_t *ptr = org;
			if (WAV_STREAM->fmt.channels == 1)
			{
				for (size_t i = 0; i < count; ++i)
				{
					samples[0] = *(ptr++) / 2147483648.f;
					samples[1] = samples[0];
					samples += 2;
				}
			}
			else
			{
				for (size_t i = 0; i < count; ++i)
				{
					samples[0] = *(ptr + 0) / 2147483648.f;
					samples[1] = *(ptr + 1) / 2147483648.f;
					samples += WAV_STREAM->fmt.channels;
					ptr += WAV_STREAM->fmt.channels;
				}
			}
			WAV_STREAM->file->pos += (ptr - org) * 4;
			WAV_STREAM->chunk_pos += (ptr - org) * 4;
			break;
		}
	}
	return count;
}

static ssize_t read_samples(struct snd_stream *stream, float *samples, size_t count, size_t *sample_rate)
{
	while (count)
	{
		if (WAV_STREAM->chunk_pos != WAV_STREAM->chunk_size)
			return decode_samples(stream, samples, count, sample_rate);
		if (WAV_STREAM->file->pos == WAV_STREAM->file->size)
		{
			if (1) /* stop on end */
				return 0;
			WAV_STREAM->file->pos = WAV_STREAM->file_begin;
		}
		if (!decode_next_chunk(WAV_STREAM))
			return -1;
	}
	return 0;
}

static void dtr(struct snd_stream *stream)
{
	wow_mpq_file_delete(WAV_STREAM->file);
}

static const struct snd_stream_vtable vtable =
{
	.dtr = dtr,
	.read_samples = read_samples,
};

struct snd_stream *snd_stream_wav_new(wow_mpq_file_t *file)
{
	char riff_sig[4];
	if (!wow_mpq_read(file, riff_sig, 4))
	{
		LOG_ERROR("wave file too short");
		return NULL;
	}
	if (memcmp(riff_sig, "RIFF", 4))
	{
		LOG_ERROR("invalid riff magic");
		return NULL;
	}
	uint32_t length;
	if (!wow_mpq_read(file, &length, 4))
	{
		LOG_ERROR("wave file too short");
		return NULL;
	}
	if (length + 8 != file->size)
	{
		LOG_ERROR("invalid wave file length");
		return NULL;
	}
	char wave_sig[4];
	if (!wow_mpq_read(file, wave_sig, 4))
	{
		LOG_ERROR("wave file too short");
		return NULL;
	}
	if (memcmp(wave_sig, "WAVE", 4))
	{
		LOG_ERROR("invalid wave magic");
		return NULL;
	}
	struct snd_stream_wav *stream = mem_zalloc(MEM_SND, sizeof(*stream));
	if (!stream)
		return NULL;
	stream->stream.vtable = &vtable;
	stream->file_begin = file->pos;
	stream->file = file;
	return &stream->stream;
}
