#ifndef GPCORE_MEMORY_H
#define GPCORE_MEMORY_H

#include "short_types.h"

enum ALLOC_TYPE {
  ALLOC_HEAP,
  ALLOC_POLL,
  ALLOC_FRAME,
  ALLOC_PAGE
};

typedef struct gp_alloc_s{
  char        debug_name[32];
  gp_alloc_s *backing_allocator;
  ALLOC_TYPE  type;
  size_t      size;
  U32         allocations;
} gp_alloc_t;

void  gp_alloc_init  (gp_alloc_t *allocator);
void  gp_alloc_proxy (char *name, gp_alloc_t *proxy, gp_alloc_t *allocator);
void  gp_alloc_free  (gp_alloc_t *allocator);

void *gp_allocate    (gp_alloc_t *allocator, size_t size, size_t align);
void  gp_deallocate  (void *p);

#endif // GPCORE_MEMORY_H