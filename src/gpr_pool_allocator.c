#include "gpr_pool_allocator.h"

typedef gpr_pool_allocator_page_t  page_t;
typedef gpr_pool_allocator_entry_t entry_t;

static U32 create_page(gpr_pool_allocator_t *a)
{
  page_t p;
  p.blocks = (char*)gpr_allocate(a->backing, a->block_size*a->page_size);
  p.used_blocks = 0;
  gpr_array_init(char*, &p.freelist, a->backing);
  gpr_array_reserve(char*, &p.freelist, a->page_size);

  { 
    U32 i; char *last_block = p.blocks + a->block_size*(a->page_size-1);
    for(i = 0; i < a->page_size; ++i)
      gpr_array_push_back(char*, &p.freelist, last_block - i*a->block_size);
  }

  gpr_array_push_back(page_t, &a->pages, p);
  return gpr_array_size(&a->pages) - 1;
}

static I32 page_full(gpr_pool_allocator_t *a, U32 page_index)
{
  return gpr_array_item(&a->pages, page_index).used_blocks == a->page_size;
}

static void *dequeue_block(gpr_pool_allocator_t *a, 
                           U32 size, U32 align, U32 page_index)
{
  page_t *page  = &gpr_array_item(&a->pages, page_index);
  char   *block =  gpr_array_back(&page->freelist);
  void   *p     =  gpr_align_forward(block, align);

  // if the requested memory is too big to fit in a block,
  // return memory from the backing allocator
  if((char*)p - block + size > a->block_size)
    return gpr_allocate_align(a->backing, size, align);

  ++page->used_blocks;
  gpr_array_pop_back(&page->freelist);
  {
    entry_t e;
    e.page  = page_index;
    e.block = block;
    gpr_hash_set(entry_t, &a->pointer_block, (U64)p, &e);
  }

  return p;
}

static void *allocate(gpr_pool_allocator_t *a, U32 size, U32 align)
{
  // if the current page is not fully used, return a block from it
  if(!page_full(a, a->current_page)) 
    return dequeue_block(a, size, align, a->current_page);

  {
    // find the most used page that is not full
    U32 num_pages = gpr_array_size(&a->pages);
    U32 max_used =  gpr_array_item(&a->pages, a->current_page).used_blocks;
    U32 i = 0; U32 new_page = a->current_page;

    while( i < num_pages)
    {
      page_t *tmp = &gpr_array_item(&a->pages, i);
      if (!page_full(a, i) 
        && gpr_array_item(&a->pages, i).used_blocks > max_used)
      {
        max_used = tmp->used_blocks;
        new_page = i;
      } 
      ++i;
    }

    if(new_page != a->current_page)
      a->current_page = i;
    else
      a->current_page = create_page(a);
  }
  return dequeue_block(a, size, align, a->current_page);
}

static void deallocate(gpr_pool_allocator_t *a, void *p) 
{
  entry_t *e = gpr_hash_get(entry_t, &a->pointer_block, (U64)p);

  if(e != NULL)
  {
    page_t *page = &gpr_array_item(&a->pages, e->page);
    gpr_array_push_back(char*, &page->freelist, e->block);
  } else {
    gpr_deallocate(a->backing, p);
  }
}

static U32 allocated_for(gpr_pool_allocator_t *a, void *p) 
{ return GPR_SIZE_NOT_TRACKED; }

static U32 allocated_tot(gpr_pool_allocator_t *a)
{ 
  return gpr_array_size(&a->pages) * a->page_size * a->block_size;
}

void gpr_pool_allocator_init(gpr_pool_allocator_t *a, U32 block_size, U32 page_size, 
                             gpr_allocator_t *backing)
{
  a->backing    = backing;
  a->block_size = block_size;
  a->page_size  = page_size;
  a->current_page  = 0;

  gpr_array_init(page_t, &a->pages, backing);
  gpr_hash_init (entry_t, &a->pointer_block, backing);
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
    gpr_array_destroy(&p->freelist);
    gpr_deallocate(a->backing, p->blocks);
    ++p;
  }
  gpr_array_destroy(&a->pages);
  gpr_hash_destroy(entry_t, &a->pointer_block);
}