#include "snd/stream.h"

#include "memory.h"

void snd_stream_delete(struct snd_stream *stream)
{
	if (!stream)
		return;
	if (stream->vtable->dtr)
		stream->vtable->dtr(stream);
	mem_free(MEM_SND, stream);
}
