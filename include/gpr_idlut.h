#ifndef GPR_IDLUT_H
#define GPR_IDLUT_H

//#include "memory.h"
#include <stdlib.h> 
#include "gpr_assert.h"
#include "gpr_types.h"
#include "gpr_memory.h"

// -------------------------------------------------------------------------
// Id Lookup Table Container 
// -------------------------------------------------------------------------
// Design & Implementation inspired from
// http://www.altdevblogaday.com/2011/09/23/managing-decoupling-part-4-the-id-lookup-table/
// -------------------------------------------------------------------------

typedef struct
{
  U32  id;    // external id
  U16  index; // index of the item
  U16  next;  // index of the next gpr_index_t
} gpr_index_t;

#define GPR_INDEX_MASK 0xffff
#define GPR_ID_ADD 0x10000

#define GPR_IDLUT_INIT(type)                                                \
typedef struct                                                              \
{                                                                           \
  U32  id;                                                                  \
  type value;                                                               \
} gpr_##type##_idlut_item;                                                  \
                                                                            \
typedef struct                                                              \
{                                                                           \
  gpr_allocator_t *allocator;                                               \
  U16 max_items;        /* maximum number of items available in the table */\
  U16 num_items;        /* number of items in the id lookup table */        \
  gpr_##type##_idlut_item                                                   \
    *items;             /* items of the table */                            \
  gpr_index_t *indices; /* table indices used to avoid item fragmentation*/ \
  U16 freelist_enqueue; /* index of the previous available table index */   \
  U16 freelist_dequeue; /* index of the next available table index */       \
} gpr_##type##_idlut_t;                                                     \
                                                                            \
static void gpr_##type##_idlut_init(gpr_##type##_idlut_t *table,            \
                             gpr_allocator_t *allocator, U16 max_items)     \
{                                                                           \
  max_items = gpr_next_pow2_U16(max_items);                                 \
  table->allocator = allocator;                                             \
  table->items = (gpr_##type##_idlut_item*)gpr_allocate(table->allocator,   \
                  max_items * sizeof(gpr_##type##_idlut_item));             \
                                                                            \
  table->indices = (gpr_index_t*)gpr_allocate(table->allocator,             \
                    max_items * sizeof(gpr_index_t));                       \
                                                                            \
  table->max_items        = max_items;                                      \
  table->num_items        = 0;                                              \
  table->freelist_dequeue = 0;                                              \
  table->freelist_enqueue = max_items;                                      \
                                                                            \
  {                                                                         \
    U16 i;                                                                  \
    for(i = 0; i < table->max_items; ++i)                                   \
    {                                                                       \
      table->indices[i].id = i;                                             \
      table->indices[i].next = i+1;                                         \
    }                                                                       \
  }                                                                         \
}                                                                           \
                                                                            \
static void gpr_##type##_idlut_destroy(gpr_##type##_idlut_t *table)         \
{                                                                           \
  gpr_deallocate(table->allocator, table->items);                           \
  gpr_deallocate(table->allocator, table->indices);                         \
}                                                                           \
                                                                            \
static U32 gpr_##type##_idlut_add(gpr_##type##_idlut_t *table,              \
                           const type *item_value)                          \
{                                                                           \
  gpr_##type##_idlut_item *item;                                            \
  gpr_index_t *in = &table->indices[table->freelist_dequeue];               \
                                                                            \
  gpr_assert(table->num_items + 1 <= table->max_items);                     \
                                                                            \
  in->id    += GPR_ID_ADD ;                                                 \
  in->index  = table->num_items;                                            \
                                                                            \
  item = &table->items[in->index];                                          \
  item->value = *item_value;                                                \
  item->id    =  in->id;                                                    \
                                                                            \
  ++table->num_items;                                                       \
  table->freelist_dequeue = in->next;                                       \
                                                                            \
  return in->id;                                                            \
}                                                                           \
                                                                            \
static I32 gpr_##type##_idlut_has(gpr_##type##_idlut_t *table, U32 id)      \
{                                                                           \
  gpr_index_t *in = &table->indices[id & GPR_INDEX_MASK];                   \
  return in->id == id && in->index != USHRT_MAX;                            \
}                                                                           \
                                                                            \
static type *gpr_##type##_idlut_lookup(gpr_##type##_idlut_t *table, U32 id) \
{                                                                           \
  return &table->items[table->indices[id & GPR_INDEX_MASK].index].value;    \
}                                                                           \
                                                                            \
static void gpr_##type##_idlut_remove(gpr_##type##_idlut_t *table, U32 id)  \
{                                                                           \
  gpr_##type##_idlut_item *item;                                            \
  gpr_index_t *in = &table->indices[id & GPR_INDEX_MASK];                   \
                                                                            \
  /* copy the last item at position of the item to delete to avoid holes */ \
  item = &table->items[in->index];                                          \
  *item = table->items[--table->num_items];                                 \
  table->indices[item->id & GPR_INDEX_MASK].index = in->index;              \
                                                                            \
  in->index = USHRT_MAX;                                                    \
	table->freelist_enqueue = id & GPR_INDEX_MASK;                            \
  if(table->freelist_dequeue == table->max_items)                           \
    table->freelist_dequeue = table->freelist_enqueue;                      \
}                                                                           \
                                                                            \
static gpr_##type##_idlut_item *                                            \
gpr_##type##_idlut_items(gpr_##type##_idlut_t *table)                       \
{                                                                           \
  return table->items;                                                      \
}                                                                           \
                                                                            \
static void gpr_##type##_idlut_swap                                         \
              (gpr_##type##_idlut_t *table, U32 src_id, U32 dest_id)        \
{                                                                           \
  U16 swap_i;                                                               \
  gpr_##type##_idlut_item swap;                                             \
  gpr_index_t *src_in = &table->indices[src_id & GPR_INDEX_MASK];           \
  gpr_index_t *dest_in = &table->indices[dest_id & GPR_INDEX_MASK];         \
                                                                            \
  swap = table->items[dest_in->index];                                      \
  table->items[dest_in->index] = table->items[src_in->index];               \
  table->items[src_in->index] = swap;                                       \
                                                                            \
  swap_i = dest_in->index;                                                  \
  dest_in->index = src_in->index;                                           \
  src_in->index = swap_i;                                                   \
                                                                            \
  swap_i = src_in->next;                                                    \
  src_in->next = dest_in->next;                                             \
  dest_in->next = swap_i;                                                   \
}                                                                           \
                                                                            \
static void gpr_##type##_idlut_swap_to                                      \
              (gpr_##type##_idlut_t *table, U32 id, int index)              \
{                                                                           \
  int i;                                                                    \
  U16 dest_id;                                                              \
                                                                            \
  for(i = 0; table->num_items; i++)                                         \
    if(table->indices[i].index == index)                                    \
    {                                                                       \
      dest_id = table->indices[i].id;                                       \
      break;                                                                \
    }                                                                       \
  gpr_##type##_idlut_swap(table, id, dest_id);                              \
}                                                                           \

#define gpr_idlut_item(type) gpr_##type##_idlut_item
#define gpr_idlut_t(type) gpr_##type##_idlut_t

// create an id lookup table allocating memory for "max_items"
#define gpr_idlut_init(type, table, allocator, max_items) \
  gpr_##type##_idlut_init(table, allocator, max_items)

// free the id lookup table and all its memory
#define gpr_idlut_destroy(type, table) \
  gpr_##type##_idlut_destroy(table)

// add the specified item to the table and return its id
#define gpr_idlut_add(type, table, item_value) \
  gpr_##type##_idlut_add(table, item_value)

// return 1 or 0 if the table contains or not the item
#define gpr_idlut_has(type, table, id) \
  gpr_##type##_idlut_has(table, id)

// return a pointer to the item with the "id" identifier
#define gpr_idlut_lookup(type, table, id) \
  gpr_##type##_idlut_lookup(table, id)

// remove the item with the "id" identifier (does not free memory)
#define gpr_idlut_remove(type, table, id) \
  gpr_##type##_idlut_remove(table, id)

// return the table item list
#define gpr_idlut_items(type, table) \
  gpr_##type##_idlut_items(table)

#define gpr_idlut_swap(type, table, src_id, dest_id) \
  gpr_##type##_idlut_swap(table, src_id, dest_id)
  
#define gpr_idlut_swap_to(type, table, id, index) \
  gpr_##type##_idlut_swap_to(table, id, index)

#endif // GPR_IDLUT_H