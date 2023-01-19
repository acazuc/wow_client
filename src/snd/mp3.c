#include "snd/stream.h"

#include "memory.h"
#include "cache.h"
#include "log.h"
#include "wow.h"

#include <libwow/mpq.h>

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_FLOAT_OUTPUT
#include "minimp3.h"

#define MP3_STREAM ((struct snd_stream_mp3*)stream)

#define CHANNELS 2

struct snd_stream_mp3
{
	struct snd_stream stream;
	wow_mpq_file_t *file;
	mp3dec_frame_info_t info;
	mp3dec_t ctx;
	mp3d_sample_t buf[MINIMP3_MAX_SAMPLES_PER_FRAME];
	size_t buf_size;
	size_t file_pos;
};

static ssize_t read_samples(struct snd_stream *stream, float *samples, size_t count, size_t *sample_rate)
{
	size_t ret = 0;
	while (count)
	{
		if (MP3_STREAM->buf_size >= count * CHANNELS)
		{
			memcpy(samples, MP3_STREAM->buf, count * CHANNELS * sizeof(float));
			MP3_STREAM->buf_size -= count * CHANNELS;
			memmove(MP3_STREAM->buf, &MP3_STREAM->buf[count * CHANNELS], MP3_STREAM->buf_size * sizeof(float));
			ret += count;
			*sample_rate = MP3_STREAM->info.hz;
			return ret;
		}
		if (MP3_STREAM->buf_size > 0)
		{
			memcpy(samples, MP3_STREAM->buf, MP3_STREAM->buf_size * sizeof(float));
			samples = &((float*)samples)[MP3_STREAM->buf_size];
			count -= MP3_STREAM->buf_size / CHANNELS;
			ret += MP3_STREAM->buf_size / CHANNELS;
			MP3_STREAM->buf_size = 0;
			*sample_rate = MP3_STREAM->info.hz;
			return ret;
		}
		if (MP3_STREAM->file_pos == MP3_STREAM->file->size)
		{
			if (1) /* stop on end */
				return 0;
			MP3_STREAM->file_pos = 0;
		}
		MP3_STREAM->buf_size = mp3dec_decode_frame(&MP3_STREAM->ctx, &MP3_STREAM->file->data[MP3_STREAM->file_pos], MP3_STREAM->file->size - MP3_STREAM->file_pos, MP3_STREAM->buf, &MP3_STREAM->info) * 2;
		MP3_STREAM->file_pos += MP3_STREAM->info.frame_bytes;
	}
	return ret;
}

static void dtr(struct snd_stream *stream)
{
	wow_mpq_file_delete(MP3_STREAM->file);
}

static const struct snd_stream_vtable vtable =
{
	.dtr = dtr,
	.read_samples = read_samples,
};

struct snd_stream *snd_stream_mp3_new(wow_mpq_file_t *file)
{
	struct snd_stream_mp3 *stream = mem_zalloc(MEM_SND, sizeof(*stream));
	if (!stream)
		return NULL;
	stream->stream.vtable = &vtable;
	mp3dec_init(&stream->ctx);
	stream->file = file;
	return &stream->stream;
}
