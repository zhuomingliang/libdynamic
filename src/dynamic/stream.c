#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "buffer.h"
#include "stream.h"

static int stream_need(stream *stream, size_t size)
{
  if (stream_valid(stream) && size <= stream_size(stream))
    return 1;

  stream_destruct(stream);
  return 0;
}

static int stream_writable(stream *stream)
{
  if (stream_valid(stream) && stream->buffer)
    return 1;

  stream_destruct(stream);
  return 0;
}

void stream_construct(stream *stream, void *data, size_t size)
{
  *stream = (struct stream) {0};
  stream->data = data;
  stream->end = size;
}

void stream_construct_buffer(stream *stream, buffer *buffer)
{
  *stream = (struct stream) {0};
  stream->buffer = buffer;
  stream->data = buffer_data(stream->buffer);
  stream->begin = 0;
  stream->end = 0;
}

void stream_destruct(stream *stream)
{
  *stream = (struct stream) {0};
}

int stream_valid(stream *stream)
{
  return stream->buffer != NULL || stream->data != NULL;
}

void *stream_data(stream *stream)
{
  return stream->data + stream->begin;
}

size_t stream_size(stream *stream)
{
  return stream->end - stream->begin;
}

void stream_read(stream *stream, void *data, size_t size)
{
  if (!stream_need(stream, size))
    return;

  if (data)
    memcpy(data, stream->data + stream->begin, size);
  stream->begin += size;
}

uint8_t stream_read8(stream *stream)
{
  uint8_t value;

  stream_read(stream, &value, sizeof value);
  if (!stream_valid(stream))
    return 0;

  return value;
}

uint16_t stream_read16(stream *stream)
{
  uint16_t value;

  stream_read(stream, &value, sizeof value);
  if (!stream_valid(stream))
    return 0;

  return be16toh(value);
}

uint32_t stream_read32(stream *stream)
{
  uint32_t value;

  stream_read(stream, &value, sizeof value);
  if (!stream_valid(stream))
    return 0;

  return be32toh(value);
}

uint64_t stream_read64(stream *stream)
{
  uint64_t value;

  stream_read(stream, &value, sizeof value);
  if (!stream_valid(stream))
    return 0;

  return be64toh(value);
}

uint64_t stream_read_bits(uint64_t value, int size, int offset, int bits)
{
  value >>= (size - offset - bits);
  value &= (1 << bits) - 1;
  return value;
}

void stream_write(stream *stream, void *data, size_t size)
{
  if (!stream_writable(stream))
    return;

  buffer_insert(stream->buffer, buffer_size(stream->buffer), data, size);
  stream->data = buffer_data(stream->buffer);
  stream->end += size;
}

void stream_write8(stream *stream, uint8_t value)
{
  stream_write(stream, &value, sizeof value);
}

void stream_write16(stream *stream, uint16_t value)
{
  value = htobe16(value);
  stream_write(stream, &value, sizeof value);
}

void stream_write32(stream *stream, uint32_t value)
{
  value = htobe32(value);
  stream_write(stream, &value, sizeof value);
}

void stream_write64(stream *stream, uint64_t value)
{
  value = htobe64(value);
  stream_write(stream, &value, sizeof value);
}

uint64_t stream_write_bits(uint64_t value, int size, int offset, int bits)
{
  value &= (1 << bits) - 1;
  value <<= (size - offset - bits);
  return value;
}
