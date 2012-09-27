#ifndef GPR_MEMORY_H
#define GPR_MEMORY_H

#include "short_types.h"

enum gpr_ALLOC_TYPE {
  ALLOC_HEAP,  // varied sized blocks - linked list of free blocks
  ALLOC_POLL,  // fixed size pools that can be packed tight in memory
  ALLOC_FRAME, // does not deallocate blocks : releases its memory in one go
  ALLOC_PAGE,  // 
  ALLOC_PROXY  // 
};

typedef struct gpr_alloc_s{
  char            debug_name[32]; // allocator's name for tracing purpose
  gpr_ALLOC_TYPE  type;           // memory allocation type
  gpr_alloc_s    *parent;         // parent allocator if this one is a proxy
  size_t          size;           // memory allocated by this allocator
  U32             allocations;    // total number of allocations done
} gpr_alloc_t;

// initialize an allocator
void  gpr_alloc_init  (char *name, gpr_alloc_t *allocator, gpr_ALLOC_TYPE type);

// create a proxy based on an existing allocator
void  gpr_alloc_proxy (char *name, gpr_alloc_t *proxy, gpr_alloc_t *allocator);

void  gpr_alloc_clear (gpr_alloc_t *allocator);

// delete the allocator & its proxies freeing all allocated memory
void  gpr_alloc_del   (gpr_alloc_t *allocator);

void *gpr_allocate    (gpr_alloc_t *allocator, size_t size, size_t align);
void  gpr_deallocate  (gpr_alloc_t *allocator, void *p);

#endif // GPR_MEMORY_H
