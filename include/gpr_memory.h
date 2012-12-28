#ifndef GPR_MEMORY_H
#define GPR_MEMORY_H

#include "gpr_types.h"

#define GPR_DEFAULT_ALIGN 4
#define GPR_SIZE_NOT_TRACKED 0xffffffffu

typedef struct gpr_allocator_s gpr_allocator_t;

// memory allocation functions
void *gpr_allocate       (gpr_allocator_t *a, SZ size);
void *gpr_allocate_align (gpr_allocator_t *a, SZ size, SZ align);
void  gpr_deallocate     (gpr_allocator_t *a, void*p);
SZ    gpr_allocated_tot  (gpr_allocator_t *a);
SZ    gpr_allocated_for  (gpr_allocator_t *a, void*p);

// global memory allocators
extern gpr_allocator_t *gpr_default_allocator;
extern gpr_allocator_t *gpr_scratch_allocator;

// initializes/shuts down the global memory allocators
void gpr_memory_init     (SZ scratch_buffer_size);
void gpr_memory_shutdown ();

#endif // GPR_MEMORY_H