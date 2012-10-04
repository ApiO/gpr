#ifndef GPR_MEMORY_H
#define GPR_MEMORY_H

#include "short_types.h"

typedef enum
{
  GPR_ALLOC_HEAP,  // general purpose allocator (using dlmalloc)
  GPR_ALLOC_POOL,  // fixed size pools that can be packed tight in memory
  GPR_ALLOC_FRAME, // does not deallocate blocks: releases its memory in one go
  GPR_ALLOC_PAGE,  // the virtual memory allocator provided by the system
  GPR_ALLOC_PROXY  // specific allocation tracking using the "parent" allocator
} GPR_ALLOC_TYPE;

typedef struct gpr_alloc_s
{
  char                debug_name[32]; // allocator's name for tracing purpose
  GPR_ALLOC_TYPE      type;           // memory allocation type
  struct gpr_alloc_s *parent;         // parent allocator if this one is a proxy
  size_t              size;           // memory allocated by this allocator
  U32                 allocations;    // total number of allocations done
} gpr_alloc_t;

// initialize an allocator
void  gpr_alloc_init     (char *name, gpr_alloc_t *allocator, 
                          GPR_ALLOC_TYPE type);

// create a proxy based on an existing allocator
void  gpr_alloc_proxy    (char *name, gpr_alloc_t *proxy, 
                          gpr_alloc_t *allocator);

// delete the allocator & its proxies freeing all allocated memory
void  gpr_alloc_del      (gpr_alloc_t *allocator);

// allocate memory using "allocator"
void *gpr_allocate       (gpr_alloc_t *allocator, size_t size);

// allocate aligned memory using "allocator"
void *gpr_allocate_align (gpr_alloc_t *allocator, size_t size, size_t align);

// free memory allocated by "allocator"
void  gpr_deallocate     (gpr_alloc_t *allocator, void *p);

#endif // GPR_MEMORY_H
