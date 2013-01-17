#ifndef GPR_ARRAY_H
#define GPR_ARRAY_H

#include <memory.h>
#include "gpr_types.h"
#include "gpr_memory.h"

#define gpr_array_t(type) \
struct { type *data; U32 size, capacity; gpr_allocator_t *allocator; }

#define gpr_array_item(a, i)   ((a)->data[(i)])
#define gpr_array_pop_back(a)  ((a)->data[--(a)->size])
#define gpr_array_remove(a, i) ((a)->data[i] = (a)->data[--(a)->size])
#define gpr_array_size(a)      ((a)->size)
#define gpr_array_empty(a)     ((a)->size == 0)
#define gpr_array_any(a)       ((a)->size >  0)
#define gpr_array_capacity(a)  ((a)->capacity)
#define gpr_array_begin(a)     ((a)->data)
#define gpr_array_end(a)       ((a)->data + (a)->size)
#define gpr_array_front(a)     ((a)->data[0])
#define gpr_array_back(a)      ((a)->data[(a)->size-1])
#define gpr_array_destroy(a)   gpr_deallocate((a)->allocator, (a)->data)

#define gpr_array_init(type, a, alct)                                \
do {                                                                 \
  (a)->allocator = alct;                                             \
  (a)->size = 0;                                                     \
  (a)->capacity = 2;                                                 \
  (a)->data = (type*)gpr_allocate(alct, sizeof(type)*2);             \
} while(0)

#define _gpr_array_realloc(type, a, c)                               \
do {                                                                 \
  type *tmp = (a)->data;                                             \
  (a)->capacity = (c);                                               \
  (a)->data = (type*)gpr_allocate((a)->allocator, sizeof(type)*(c)); \
  memcpy((a)->data, tmp, sizeof(type)*(a)->size);                    \
  gpr_deallocate((a)->allocator, tmp);                               \
} while(0)

#define gpr_array_push_back(type, a, x)                              \
do {                                                                 \
  if ((a)->size == (a)->capacity)                                    \
    _gpr_array_realloc(type, a, (a)->capacity << 1);                 \
  (a)->data[(a)->size++] = (x);                                      \
} while(0)

#define gpr_array_reserve(type, a, c)                                \
if((c) > (a)->capacity)                                              \
  _gpr_array_realloc(type, (a), gpr_next_pow2_U32(c))

#define gpr_array_resize(type, a, s)                                 \
  gpr_array_reserve(type, (a), (s));                                 \
  (a)->size = s

#define gpr_array_copy(type, dest, src)                              \
do {                                                                 \
  gpr_array_reserve(type, (dest), (src)->size);                      \
  memcpy((dest)->data, (src)->data, sizeof(type)*(src)->size);       \
  (dest)->size = (src)->size;                                        \
} while(0)

#endif // GPR_ARRAY_H