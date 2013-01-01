#ifndef GPR_TMP_ALLOCATOR_H
#define GPR_TMP_ALLOCATOR_H

#include "gpr_allocator.h"

// -------------------------------------------------------------------------
// A temporary memory allocator that primarily allocates memory from a
// local stack buffer.
// -------------------------------------------------------------------------
// If the stack memory is exhausted it will use the backing scratch allocator.
// Memory allocated with a TempAllocator does not have to be deallocated, but 
// the destroy function must be called before the end of the scope to free 
// the memory that may be allocated by the backing allocator.
// -------------------------------------------------------------------------


#define GPR_TMP_ALLOCATOR_INIT(s) \
typedef struct                    \
{                                 \
  gpr_allocator_t  base;          \
  gpr_allocator_t *backing;       \
  char            *start;         \
  char            *p;             \
  char            *end;           \
  SZ               chunk_size;    \
  char             buffer[s];     \
} gpr_tmp_allocator_##s##_t;

GPR_TMP_ALLOCATOR_INIT(64)
GPR_TMP_ALLOCATOR_INIT(128)
GPR_TMP_ALLOCATOR_INIT(256)
GPR_TMP_ALLOCATOR_INIT(512)
GPR_TMP_ALLOCATOR_INIT(1024)
GPR_TMP_ALLOCATOR_INIT(2048)
GPR_TMP_ALLOCATOR_INIT(4096)

void gpr_tmp_allocator_init    (void *a, SZ size);
void gpr_tmp_allocator_destroy (void *a);

#endif // GPR_TMP_ALLOCATOR_H