#include "gpr_pool_allocator.h"

typedef gpr_pool_page_t      page_t;
typedef gpr_pool_freeblock_t freeblock_t;

static U32 create_page(gpr_pool_allocator_t *a)
{
  page_t p;
  p.blocks = (char*)gpr_allocate_align(a->backing, a->block_size*a->page_size, 
                                       a->block_align);
  p.used_blocks = 0;
  gpr_array_reserve(freeblock_t, &a->freelist, 
    (gpr_array_size(&a->pages)+1) * (a->page_size));

  { 
    U32 i; 
    char *last_block = p.blocks + a->block_size*(a->page_size-1);
    freeblock_t fb;
    for(i = 0; i < a->page_size; ++i)
    {
      fb.page_i = gpr_array_size(&a->pages);
      fb.block  = last_block - i*a->block_size;
      gpr_array_push_back(freeblock_t, &a->freelist, fb);
    }
  }

  gpr_array_push_back(page_t, &a->pages, p);
  return gpr_array_size(&a->pages) - 1;
}

static I32 full(gpr_pool_allocator_t *a)
{
  return gpr_array_size(&a->freelist) == 0;
}

static void *allocate(gpr_pool_allocator_t *a, U32 size, U32 align)
{
  freeblock_t *fb; void *p;

  if (full(a)) create_page(a);

  fb = &gpr_array_back(&a->freelist);
  p  = gpr_align_forward(fb->block, align);

  // if the requested memory is too big to fit in a block,
  // return memory from the backing allocator
  if((char*)p - fb->block + size > a->block_size)
    return gpr_allocate_align(a->backing, size, align);

  ++gpr_array_item(&a->pages, fb->page_i).used_blocks;
  gpr_array_pop_back(&a->freelist);
  return p;
}

#define NO_PAGE 0xffffffffu

static void deallocate(gpr_pool_allocator_t *a, void *p) 
{
  U32 i = 0;
  freeblock_t fb;
  const U32 page_bytes = (a->page_size)*a->block_size;

  if(p == NULL) return;

  fb.page_i = NO_PAGE;
  while(i < gpr_array_size(&a->pages) && fb.page_i == NO_PAGE)
  {
    page_t *page = &gpr_array_item(&a->pages, i);
    if(p >= page->blocks && p < page->blocks + page_bytes)
    {
      --page->used_blocks;
      fb.page_i = i;
    }
    ++i;
  }

  if(fb.page_i == NO_PAGE) 
  {
    gpr_deallocate(a->backing, p);
    return;
  }

  fb.block = (char*)p - ((U32)p % a->block_size);
  gpr_array_push_back(freeblock_t, &a->freelist, fb);
}

static U32 allocated_for(gpr_pool_allocator_t *a, void *p) 
{ return a->block_size; }

static U32 allocated_tot(gpr_pool_allocator_t *a)
{ 
  return (gpr_array_size(&a->pages) * a->page_size 
    - gpr_array_size(&a->freelist)) * a->block_size;
}

void gpr_pool_allocator_init(gpr_pool_allocator_t *a, U32 block_size,
                             U32 page_size, gpr_allocator_t *backing)
{
  return gpr_pool_allocator_init_align(a, block_size, GPR_DEFAULT_ALIGN,
                                       page_size, backing);
}

void gpr_pool_allocator_init_align(gpr_pool_allocator_t *a, U32 block_size, 
                                   U32 block_align, U32 page_size, 
                                   gpr_allocator_t *backing)
{
  a->backing    = backing;
  // roundup the block size to the next multiple of the default alignment
  // in order to have blocks already aligned
  a->block_size = gpr_next_multiple(block_size, block_align);
  a->block_align = block_align;
  a->page_size  = page_size;

  gpr_array_init(freeblock_t, &a->freelist, backing);
  gpr_array_init(page_t, &a->pages, backing);

  create_page(a);

  gpr_set_allocator_functions(a,
    allocate,
    deallocate,
    allocated_for,
    allocated_tot);
}

void gpr_pool_allocator_destroy(gpr_pool_allocator_t *a)
{
  page_t *p   = gpr_array_begin(&a->pages);
  page_t *end = gpr_array_end  (&a->pages);
  while(p < end)
  {
    gpr_deallocate(a->backing, p->blocks);
    ++p;
  }
  gpr_array_destroy(&a->pages);
  gpr_array_destroy(&a->freelist);
}