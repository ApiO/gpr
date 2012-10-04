#include "gpr_assert.h"
#include "gpr_memory.h"
#include "malloc.h"

#define ALIGNOF(type) offsetof (struct { char c; type member; }, member)

// initialize an allocator
void gpr_alloc_init(char *name, gpr_alloc_t *allocator, GPR_ALLOC_TYPE type)
{
}

// create a proxy based on an existing allocator
void gpr_alloc_proxy(char *name, gpr_alloc_t *proxy, gpr_alloc_t *allocator)
{
}

// delete the allocator & its proxies freeing all allocated memory
void gpr_alloc_del(gpr_alloc_t *allocator)
{
}

// allocate memory using "allocator"
void *gpr_allocate(gpr_alloc_t *allocator, size_t size)
{
  return gpr_allocate_align(allocator, size, -1);
}

// allocate aligned memory using "allocator"
void *gpr_allocate_align(gpr_alloc_t *allocator, size_t size, size_t align)
{
  void *mem;

  switch (allocator->type)
  {
  case GPR_ALLOC_HEAP : 
    mem = align < 0 ? dlmemalign(align, size) : dlmalloc(size);
    break;
  case GPR_ALLOC_POOL :
    GPR_ASSERT_FAILURE_MSG("unimplemented allocator");
    break;
  case GPR_ALLOC_FRAME :
    GPR_ASSERT_FAILURE_MSG("unimplemented allocator");
    break;
  case GPR_ALLOC_PAGE :
    GPR_ASSERT_FAILURE_MSG("unimplemented allocator");
    break;
  case GPR_ALLOC_PROXY :
    GPR_ASSERT_FAILURE_MSG("unimplemented allocator");
    break;
  default:
    GPR_ASSERT_FAILURE_MSG("unimplemented allocator");
    break;
  }

  if (mem == NULL) return mem;

  allocator->allocations++;
  allocator->size += size;

  return mem;
}

// free memory allocated by "allocator"
void gpr_deallocate  (gpr_alloc_t *allocator, void *p)
{
  switch (allocator->type)
  {
  case GPR_ALLOC_HEAP : 
    break;
  case GPR_ALLOC_POOL :
    GPR_ASSERT_FAILURE_MSG("unimplemented allocator");
    break;
  case GPR_ALLOC_FRAME :
    GPR_ASSERT_FAILURE_MSG("unimplemented allocator");
    break;
  case GPR_ALLOC_PAGE :
    GPR_ASSERT_FAILURE_MSG("unimplemented allocator");
    break;
  case GPR_ALLOC_PROXY :
    GPR_ASSERT_FAILURE_MSG("unimplemented allocator");
    break;
  default:
    GPR_ASSERT_FAILURE_MSG("unimplemented allocator");
    break;
  }
}