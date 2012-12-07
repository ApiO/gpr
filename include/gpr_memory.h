#ifndef GPR_MEMORY_H
#define GPR_MEMORY_H

#include "short_types.h"

#define DEFAULT_ALIGN 4
#define SIZE_NOT_TRACKED = 0xffffffffu

typedef struct gpr_allocator_s
{
  char debug_name[32];  // allocator's name for tracing purpose
  SZ   total_allocated; // total amount of memory allocated by this allocator

  // allocates memory with the DEFAULT_ALIGN alignment
  void *(*allocate)       (SZ size);

  // allocates memory with the specified alignment
  void *(*allocate_align) (SZ size, I32 align);

  // frees an allocation mpreviously made by allocate()
  void  (*deallocate)     (void *p);

  // returns the amount of memory allocated for p.
  // returns SIZE_NOT_TRACKED if the allocator does not support 
  // individual allocation size tracking
  SZ    (*allocated_size) (void *p);   

} gpr_allocator_t;



#endif // GPR_MEMORY_H
