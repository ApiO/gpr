#ifndef GPR_ALLOCATOR_H
#define GPR_ALLOCATOR_H

#include "gpr_memory.h"

typedef void *(*allocate_t)       (gpr_allocator_t *self, SZ size, SZ align);
typedef void  (*deallocate_t)     (gpr_allocator_t *self, void *p);
typedef SZ    (*allocated_for_t)  (gpr_allocator_t *self, void *p);
typedef SZ    (*allocated_tot_t)  (gpr_allocator_t *self);

// allocator definition
typedef struct gpr_allocator_s
{
	allocate_t       allocate;
	deallocate_t     deallocate;
	allocated_for_t  allocated_for;
	allocated_tot_t  allocated_tot;
} gpr_allocator_t;

#define gpr_set_allocator_functions(allocator,                 \
                                    allocate_func,             \
									                  deallocate_func,           \
									                  allocated_for_func,        \
									                  allocated_tot_func)        \
{                                                              \
  gpr_allocator_t *_a = (gpr_allocator_t*) allocator;          \
	_a->allocate        = (allocate_t)       allocate_func;      \
	_a->deallocate      = (deallocate_t)     deallocate_func;    \
	_a->allocated_for   = (allocated_for_t)  allocated_for_func; \
	_a->allocated_tot   = (allocated_tot_t)  allocated_tot_func; \
}

// aligns p to the specified alignment by moving it forward if necessary and 
// returns the result.
void *gpr_align_forward(void *p, SZ align) 
{
    U8 *pi = (U8*)p;
    const SZ mod = (SZ)pi % align;
    if (mod) pi += (align - mod);
    return (void *)pi;
}

// returns the result of advancing p by the specified number of bytes
void *gpr_pointer_add(void *p, SZ bytes)
{
    return (void*)((char*)p + bytes);
}

// returns the result of moving p back by the specified number of bytes
void *gpr_pointer_sub(void *p, SZ bytes)
{
    return (void*)((char*)p - bytes);
}

#endif