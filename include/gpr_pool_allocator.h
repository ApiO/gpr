#ifndef GPR_POOL_ALLOCATOR_H
#define GPR_POOL_ALLOCATOR_H

#include "gpr_allocator.h"
#include "gpr_array.h"
#include "gpr_hash.h"

// -------------------------------------------------------------------------
// An allocator that uses preallocated fixed size blocks
// -------------------------------------------------------------------------
// A pool preallocates a page with n blocks using the backing allocator.
// If all blocks of a page are used, a new page is allocated with the same 
// number of blocks. When all blocks of a page are not used, the page memory
// is freed, except if this is the last one remaining
// -------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  char *blocks;
  U32   used_blocks;
  gpr_array_t(char*) freelist;
} gpr_pool_allocator_page_t;

typedef struct
{
  U32   page;
  char *block;
} gpr_pool_allocator_entry_t;

typedef struct
{
  gpr_allocator_t  base;
  gpr_allocator_t *backing;
  U32 block_size, page_size;
  gpr_array_t(gpr_pool_allocator_page_t) pages;
  gpr_hash_t pointer_block;
  U32 current_page;
} gpr_pool_allocator_t;

void gpr_pool_allocator_init    (gpr_pool_allocator_t *a, U32 block_size, 
                                 U32 page_size, gpr_allocator_t *backing);

void gpr_pool_allocator_destroy (void *a);

#ifdef __cplusplus
}
#endif

#endif // GPR_POOL_ALLOCATOR_H