#ifndef GPR_POOL_ALLOCATOR_H
#define GPR_POOL_ALLOCATOR_H

#include "gpr_allocator.h"
#include "gpr_array.h"

// -------------------------------------------------------------------------
// An allocator that uses preallocated fixed size blocks
// -------------------------------------------------------------------------
// A pool preallocates a page with n blocks using the backing allocator.
// If all blocks of a page are used, a new page is allocated with the same 
// number of blocks.
// if an allocation does not fit a block, the backing allocator is used
// -------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  char *blocks;
  U32   used_blocks;
} gpr_pool_page_t;

typedef struct
{
  char *block;
  U32   page_i;
} gpr_pool_freeblock_t;

typedef struct
{
  gpr_allocator_t  base;
  gpr_allocator_t *backing;
  U32 block_size, block_align, page_size;
  gpr_array_t(gpr_pool_freeblock_t) freelist;
  gpr_array_t(gpr_pool_page_t)      pages;
} gpr_pool_allocator_t;

// initialize & preallocate one page of blocks
// block size may be roundup to keep blocks aligned on the default alignment
void gpr_pool_allocator_init       (gpr_pool_allocator_t *a, U32 block_size,
                                    U32 page_size, gpr_allocator_t *backing);

// initialize & preallocate one page of blocks
// block size may be roundup to keep blocks aligned on the specified alignment
void gpr_pool_allocator_init_align (gpr_pool_allocator_t *a, U32 block_size, 
                                    U32 block_align, U32 page_size,
                                    gpr_allocator_t *backing);

void gpr_pool_allocator_destroy    (void *a);

#ifdef __cplusplus
}
#endif

#endif // GPR_POOL_ALLOCATOR_H