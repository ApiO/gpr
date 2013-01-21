#include "gpr_idlut.h"
#include <time.h>
#include <stdio.h>

typedef gpr_idlut_index_t index_t;

#define INDEX_FREE 0xffffffffu
#define INDEX_MASK 0xffffffffu
#define ID_ADD 0x100000000

static I32 full(gpr_idlut_t *t)
{
  return t->num_items >= gpr_array_size(&t->indices) - 1;
}

static void grow(gpr_idlut_t *t, const U32 s, U32 capacity)
{
  U32 i = gpr_array_size(&t->indices);

  gpr_array_item(&t->indices, t->freelist_enqueue).next = i;
  t->freelist_enqueue  = capacity-1;

  gpr_array_resize(index_t, &t->indices, capacity);
  for(; i < capacity; ++i)
  {
    gpr_array_item(&t->indices, i).id   = i;
    gpr_array_item(&t->indices, i).next = i+1;
  }
  gpr_array_reserve(U64, &t->ids, capacity);
  gpr_buffer_reserve(&t->items, capacity*s);
}

void _gpr_idlut_reserve(gpr_idlut_t *t, const U32 s, U32 capacity)
{
  grow(t, s, gpr_next_pow2_U32(capacity));
}

void _gpr_idlut_init(gpr_idlut_t *t, const U32 s, gpr_allocator_t *a)
{
  gpr_array_init(index_t, &t->indices, a);
  gpr_array_init(U64,     &t->ids,     a);
  gpr_buffer_init(&t->items, a);

  t->num_items = 0;
  t->freelist_dequeue = 0;
  t->freelist_enqueue = 0;
}

void _gpr_idlut_destroy(gpr_idlut_t *t)
{
  gpr_array_destroy(&t->indices);
  gpr_array_destroy(&t->ids);
  gpr_buffer_destroy(&t->items);
}

U64 _gpr_idlut_add(gpr_idlut_t *t, const U32 s, void *item)
{
  index_t *index; U64 id;
  if(gpr_array_size(&t->indices) == 0) grow(t, s, 4);

  index = &gpr_array_item(&t->indices, t->freelist_dequeue);
  id = index->id += ID_ADD ;
  index->item_pos = t->num_items*s;

  gpr_array_push_back(U64, &t->ids, index->id);
  gpr_buffer_ncat(&t->items, (char*)item, s);

  ++t->num_items;
  t->freelist_dequeue = index->next;

  if (full(t)) grow(t, s, gpr_array_size(&t->indices) << 1);

  return id; 
}

I32 _gpr_idlut_has(gpr_idlut_t *t, U64 id)
{
  index_t *index = &gpr_array_item(&t->indices, id & INDEX_MASK);
  return index->id == id && index->item_pos != INDEX_FREE;  
}

void *_gpr_idlut_lookup(gpr_idlut_t *t, U64 id)
{
  return t->items.data + gpr_array_item(&t->indices, id & INDEX_MASK).item_pos;
}

void _gpr_idlut_remove(gpr_idlut_t *t, const U32 s, U64 id)
{
  index_t   *index = &gpr_array_item(&t->indices, id & INDEX_MASK);
  const U32  item_pos = index->item_pos;
  const U32  item_i   = item_pos/s;

  // copy the last item at position of the item to delete to avoid holes
  gpr_array_item(&t->ids, item_i) = gpr_array_pop_back(&t->ids);
  memcpy(t->items.data + index->item_pos, t->items.data + (--t->num_items*s), s);
  gpr_buffer_resize(&t->items, t->num_items*s);

  // update the index
  gpr_array_item(&t->indices, 
    gpr_array_item(&t->ids, item_i) & INDEX_MASK).item_pos = index->item_pos;

  index->item_pos = INDEX_FREE;
  gpr_array_item(&t->indices, t->freelist_enqueue).next = id & INDEX_MASK;
  t->freelist_enqueue = id & INDEX_MASK;
}

void *_gpr_idlut_begin(gpr_idlut_t *t)
{
  return t->items.data;
}

void *_gpr_idlut_end(gpr_idlut_t *t, const U32 s)
{
  return t->items.data + t->num_items*s;
}