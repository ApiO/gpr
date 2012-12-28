#include <malloc.h>
#include "gpr_assert.h"
#include "gpr_memory.h"
#include "gpr_allocator.h"

// ---------------------------------------------------------------
// Global memory functions
// ---------------------------------------------------------------

void *gpr_allocate(gpr_allocator_t *a, SZ size) 
{
  return a->allocate(a, size, GPR_DEFAULT_ALIGN);
}

void *gpr_allocate_align(gpr_allocator_t *a, SZ size, SZ align) 
{
  return a->allocate(a, size, align);
}

void gpr_deallocate(gpr_allocator_t *a, void*p)  
{
  a->deallocate(a, p);
}

SZ gpr_allocated_tot(gpr_allocator_t *a)          
{
  return a->allocated_tot(a);
}

SZ gpr_allocated_for(gpr_allocator_t *a, void *p)  
{
  return a->allocated_for(a,p);
}

typedef struct
{
  SZ size;
} header_t;

// in case of aligned allocation, use this value as padding
U32 HEADER_PAD_VALUE = 0xffffffffu;

// ---------------------------------------------------------------
// Default allocator implementations
// ---------------------------------------------------------------

// given a pointer to the header, returns a pointer to the data that follows it
void *data_pointer(header_t *header, SZ align) {
  void *p = header + 1;
  return gpr_align_forward(p, align);
}

// given a pointer to the data, returns a pointer to the header before it
header_t *header(void *data)
{
  char *p = (char*)data;
  while (p[-1] == HEADER_PAD_VALUE) --p;
  return (header_t *)p - 1;
}

// stores the size in the header and pads with HEADER_PAD_VALUE up to the
// data pointer
void fill(header_t *header, void *data, SZ size)
{
  char *d = (char*)data;
  char *p = (char*)(header + 1);
  header->size = size;
  while (p < d) *p++ = HEADER_PAD_VALUE;
}

// -------------------------------------------------------------------------
// An allocator that uses the system malloc
// -------------------------------------------------------------------------
// Allocations are padded so that we can store the size of each allocation 
// and align them to the desired alignment.
// -------------------------------------------------------------------------

typedef struct
{
  gpr_allocator_t base;
  SZ              total_allocated;
} malloc_t;

// returns the size to allocate from malloc() for a given size and align
SZ size_with_padding(SZ size, SZ align) {
  return size + align + sizeof(header_t);
}

void *malloc_allocate(malloc_t *a, SZ size, SZ align)
{
  const SZ ts = size_with_padding(size, align);
  header_t *h = (header_t*)malloc(ts);
  void     *p = data_pointer(h, align);

  assert(align % 4 == 0);

  fill(h, p, ts);
  a->total_allocated += ts;
  return p;
}

void malloc_deallocate(malloc_t *a, void *p)
{
  header_t *h;
  if (!p) return;

  h = header(p);
  a->total_allocated -= h->size;
  free(h);
}

SZ malloc_allocated_for(malloc_t *a, void *p)
{
  return header(p)->size;
}

SZ malloc_allocated_tot(malloc_t *a)
{
  return a->total_allocated;
}

void malloc_init(malloc_t *a)
{
  a->total_allocated = 0;
  gpr_set_allocator_functions(a, 
    malloc_allocate, 
    malloc_deallocate, 
    malloc_allocated_for, 
    malloc_allocated_tot);
}

void malloc_shutdown(malloc_t *a)
{
  gpr_assert(a->total_allocated == 0);
}

// -------------------------------------------------------------------------
// An allocator that used to allocate temporary scratch memory
// -------------------------------------------------------------------------
// This allocatur uses a fixed size ring buffer.
// An allocation pointer wraps around the ring buffer, and a free pointer
// is advanced when memory is freed.
// The malloc_allocator is used as backing when the ring buffer is exhausted
// -------------------------------------------------------------------------

typedef struct
{
  gpr_allocator_t  base;
  gpr_allocator_t *backing;
  char            *begin;
  char            *end;
  char            *allocate;
  char            *free;
} scratch_t;

int in_use(scratch_t *a, void *p)
{
  char *pc = (char*)p;
  if (a->free == a->allocate)
    return 0;
  if (a->allocate > a->free)
    return pc >= a->free && pc < a->allocate;
  return pc >= a->free || pc < a->allocate;
}

void *scratch_allocate(scratch_t *a, SZ size, SZ align)
{
  char *p = a->allocate;
  header_t *h = (header_t*)p;
  char *data = (char*)data_pointer(h, align);

  gpr_assert(align % 4 == 0);
  size = ((size + 3)/4)*4;

  p = data + size;

  // Reached the end of the buffer, wrap around to the beginning.
  if (p > a->end) {
    h->size = (a->end - (char *)h) | 0x80000000u;

    p = a->begin;
    h = (header_t*)p;
    data = (char *)data_pointer(h, align);
    p = data + size;
  }

  // If the buffer is exhausted use the backing allocator instead.
  if (in_use(a, p))
    return gpr_allocate_align(a->backing, size, align);

  fill(h, data, p - (char *)h);
  a->allocate = p;
  return data;
}

void scratch_deallocate(scratch_t *a, void *p)
{
  char     *pc = (char*)p;
  header_t *h  = header(p);

  if (!pc) return;

  if (pc < a->begin || pc >= a->end) {
    gpr_deallocate(a->backing, p);
    return;
  }

  // Mark this slot as free
  gpr_assert((h->size & 0x80000000u) == 0);
  h->size = h->size | 0x80000000u;

  // Advance the free pointer past all free slots.
  while (a->free != a->allocate) {
    header_t *h = (header_t*)a->free;
    if ((h->size & 0x80000000u) == 0) break;

    a->free += h->size & 0x7fffffffu;
    if (a->free == a->end) a->free = a->begin;
  }
}

SZ scratch_allocated_for(scratch_t *a, void *p)
{
  header_t *h = header(p);
  return h->size - ((char*)p - (char*)h);
}

SZ scratch_allocated_tot(scratch_t *a)
{
  return a->end - a->begin;
}

void scratch_init(scratch_t *a, SZ size, gpr_allocator_t *backing)
{
  a->begin    = (char*)gpr_allocate(a->backing, size);
  a->end      = a->begin + size;
  a->allocate = a->begin;
  a->free     = a->begin;

  gpr_set_allocator_functions(a, 
    scratch_allocate, 
    scratch_deallocate, 
    scratch_allocated_for, 
    scratch_allocated_tot);
}

void scratch_shutdown(scratch_t *a)
{
  gpr_assert(a->free = a->allocate);
  gpr_deallocate(a->backing, a->begin);
}

// -------------------------------------------------------------------------
// Memory globals
// -------------------------------------------------------------------------

char buffer[sizeof(malloc_t) + sizeof(scratch_t)];

gpr_allocator_t *gpr_default_allocator;
gpr_allocator_t *gpr_scratch_allocator;

void gpr_memory_init(SZ scratch_buffer_size)
{
  // default allocator initialization
  char *p = buffer;
  gpr_default_allocator = (gpr_allocator_t*)p;
  malloc_init((malloc_t*)gpr_default_allocator);

  // scratch allocator initialization
  p += sizeof(malloc_t);
  gpr_scratch_allocator = (gpr_allocator_t*)p;
  scratch_init((scratch_t*)gpr_scratch_allocator, scratch_buffer_size,
    gpr_default_allocator);
}

void gpr_memory_shutdown()
{
  // default allocator shutdown
  malloc_shutdown((malloc_t*)gpr_default_allocator);

  // scratch allocator shutdown
  scratch_shutdown((scratch_t*)gpr_scratch_allocator);
}