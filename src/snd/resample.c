#include "snd/stream.h"

#include "memory.h"
#include "log.h"

#include <samplerate.h>
#include <string.h>

#define STREAM_RESAMPLE ((struct snd_stream_resample*)stream)

#define CHANNELS 2

struct snd_stream_resample
{
	struct snd_stream stream;
	struct snd_stream *input;
	SRC_STATE *src_state;
	float outbuf[1024];
	size_t outbuf_size;
	float inbuf[1024];
	size_t inbuf_size;
	size_t inbuf_rate;
	size_t output_rate;
	int stopped;
};

static ssize_t read_samples(struct snd_stream *stream, float *samples, size_t count, size_t *sample_rate)
{
	*sample_rate = STREAM_RESAMPLE->output_rate;
	size_t ret = 0;
	while (count)
	{
		if (STREAM_RESAMPLE->outbuf_size >= count * CHANNELS)
		{
			memcpy(samples, STREAM_RESAMPLE->outbuf, count * CHANNELS * sizeof(float));
			STREAM_RESAMPLE->outbuf_size -= count * CHANNELS;
			memmove(STREAM_RESAMPLE->outbuf, &STREAM_RESAMPLE->outbuf[count * CHANNELS], STREAM_RESAMPLE->outbuf_size * sizeof(float));
			ret += count;
			return ret;
		}
		if (STREAM_RESAMPLE->outbuf_size > 0)
		{
			memcpy(samples, STREAM_RESAMPLE->outbuf, STREAM_RESAMPLE->outbuf_size * sizeof(float));
			samples = &((float*)samples)[STREAM_RESAMPLE->outbuf_size];
			count -= STREAM_RESAMPLE->outbuf_size / CHANNELS;
			ret += STREAM_RESAMPLE->outbuf_size / CHANNELS;
			STREAM_RESAMPLE->outbuf_size = 0;
		}
		if (!STREAM_RESAMPLE->inbuf_size)
		{
			ssize_t rd = STREAM_RESAMPLE->input->vtable->read_samples(STREAM_RESAMPLE->input, STREAM_RESAMPLE->inbuf, sizeof(STREAM_RESAMPLE->inbuf) / sizeof(*STREAM_RESAMPLE->inbuf) / CHANNELS, &STREAM_RESAMPLE->inbuf_rate);
			if (rd <= 0)
			{
				if (ret)
					return ret;
				return rd;
			}
			STREAM_RESAMPLE->inbuf_size = rd * CHANNELS;
		}
		float ratio = STREAM_RESAMPLE->output_rate / (float)STREAM_RESAMPLE->inbuf_rate;
		if (src_set_ratio(STREAM_RESAMPLE->src_state, ratio))
			LOG_WARN("failed to set ratio %f", ratio);
		SRC_DATA data;
		data.data_in = STREAM_RESAMPLE->inbuf;
		data.data_out = STREAM_RESAMPLE->outbuf;
		data.input_frames = STREAM_RESAMPLE->inbuf_size / CHANNELS;
		data.output_frames = sizeof(STREAM_RESAMPLE->outbuf) / sizeof(*STREAM_RESAMPLE->outbuf) / CHANNELS;
		data.end_of_input = 0;
		data.src_ratio = ratio;
		if (src_process(STREAM_RESAMPLE->src_state, &data))
			LOG_WARN("failed to resample");
		STREAM_RESAMPLE->inbuf_size -= data.input_frames_used * CHANNELS;
		memmove(STREAM_RESAMPLE->inbuf, &STREAM_RESAMPLE->inbuf[data.input_frames_used * CHANNELS], STREAM_RESAMPLE->inbuf_size * sizeof(float));
		STREAM_RESAMPLE->outbuf_size = data.output_frames_gen * CHANNELS;
	}
	return ret;
}

static void dtr(struct snd_stream *stream)
{
	src_delete(STREAM_RESAMPLE->src_state);
}

static const struct snd_stream_vtable vtable =
{
	.dtr = dtr,
	.read_samples = read_samples,
};

struct snd_stream *snd_stream_resample_new(struct snd_stream *input, size_t output_rate)
{
	struct snd_stream_resample *stream = mem_zalloc(MEM_SND, sizeof(*stream));
	if (!stream)
		return NULL;
	stream->stream.vtable = &vtable;
	stream->input = input;
	int err;
	stream->src_state = src_new(SRC_SINC_MEDIUM_QUALITY, 2, &err);
	if (!stream->src_state)
	{
		mem_free(MEM_SND, stream);
		LOG_ERROR("failed to init resampling");
		return NULL;
	}
	stream->output_rate = output_rate;
	return &stream->stream;
}
