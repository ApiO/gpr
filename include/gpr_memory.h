#ifndef GPR_MEMORY_H
#define GPR_MEMORY_H

#include "gpr_types.h"

#define GPR_DEFAULT_ALIGN 4
#define GPR_SIZE_NOT_TRACKED 0xffffffffu

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gpr_allocator_s gpr_allocator_t;

// memory allocation functions
void *gpr_allocate       (gpr_allocator_t *a, U32 size);
void *gpr_allocate_align (gpr_allocator_t *a, U32 size, U32 align);
void  gpr_deallocate     (gpr_allocator_t *a, void*p);
U32   gpr_allocated_tot  (gpr_allocator_t *a);
U32   gpr_allocated_for  (gpr_allocator_t *a, void*p);

// allocate memory to store the string str and returns its pointer
// must be freed with gpr_deallocate
char *gpr_strdup(gpr_allocator_t *a, const char *str);

// global memory allocators
extern gpr_allocator_t *gpr_default_allocator;
extern gpr_allocator_t *gpr_scratch_allocator;

// initializes/shuts down the global memory allocators
void gpr_memory_init     (U32 scratch_buffer_size);
void gpr_memory_shutdown ();

#ifdef __cplusplus
}
#endif

#endif // GPR_MEMORY_H