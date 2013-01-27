#include <memory.h>
#include <stdio.h>
#include <stdarg.h>

#include "gpr_assert.h"
#include "gpr_buffer.h"
#include "gpr_tmp_allocator.h"

void gpr_buffer_init(gpr_buffer_t *buf, gpr_allocator_t *a)
{
  buf->data      = NULL;
  buf->size      = 0;
  buf->capacity  = 0;
  buf->allocator = a;
}

void gpr_buffer_destroy(gpr_buffer_t *buf)
{
  if (buf->capacity == 0) return;
  gpr_deallocate(buf->allocator, buf->data);
}

void gpr_buffer_clear(gpr_buffer_t *buf)
{
  buf->size = 0;
  buf->data[0] = '\0';
}

void gpr_buffer_reserve(gpr_buffer_t *buf, U32 capacity)
{
  if (capacity > buf->capacity)
  {
    char *tmp = buf->data;
    const U32 new_capacity = gpr_next_pow2_U32(capacity);

    buf->data = (char*)gpr_allocate(buf->allocator, new_capacity);

    if (buf->capacity > 0)
    {
      memcpy(buf->data, tmp, buf->size);
      gpr_deallocate(buf->allocator, tmp);
    }
    buf->capacity = new_capacity;
  }
}

void gpr_buffer_resize(gpr_buffer_t *buf, U32 size)
{
    gpr_buffer_reserve(buf, size);
    buf->size = size;
    buf->data[size] = '\0';
}

U32 gpr_buffer_cat(gpr_buffer_t *buf, char *str)
{
  U32 size = 0;
  while (str[size] != '\0') ++size;
  return gpr_buffer_ncat(buf, str, size);
}

U32 gpr_buffer_ncat(gpr_buffer_t *buf, char *src, U32 size)
{
  U32 pos = buf->size;

  gpr_buffer_reserve(buf, buf->size + size + 1);
  memcpy(buf->data + buf->size, src, size);

  buf->size += size;
  buf->data[buf->size] = '\0';
  return pos;
}

#define XCAT_BUFFER_SIZE 256

U32 gpr_buffer_xcat(gpr_buffer_t *buf, char *format, ...)
{
  char buffer[XCAT_BUFFER_SIZE];
  U32     n;
  va_list args;

  va_start(args, format);
  n = vsnprintf(buffer, XCAT_BUFFER_SIZE, format, args);
  gpr_assert(n <= XCAT_BUFFER_SIZE);
  va_end(args);

  return gpr_buffer_ncat(buf, buffer, n);
}